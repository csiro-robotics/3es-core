//
// Author: Kazys Stepanas
//
#include "3estext.h"

#include "3esdrawparams.h"
#include "3esmagnumcolour.h"

#include <3eslog.h>

#include <Corrade/Utility/Resource.h>

#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Quaternion.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/DistanceFieldGlyphCache.h>

#include <cctype>
#include <limits>

namespace tes::viewer::painter
{
constexpr unsigned Text::kMaxTextLength;

Text::Text(Corrade::PluginManager::Manager<Magnum::Text::AbstractFont> &font_manager,
           const std::string &font_resource_name, const std::string &fonts_resource_section,
           const std::string &font_plugin)
{
  // TODO(KS): get resources strings passed in as it's the exe which must include the resources.
  Corrade::Utility::Resource rs(fonts_resource_section);
  _cache = std::make_unique<Magnum::Text::DistanceFieldGlyphCache>(Magnum::Vector2i(2048), Magnum::Vector2i(512), 22);
  _font = font_manager.loadAndInstantiate(font_plugin);
  if (_font && _font->openData(rs.getRaw(font_resource_name), 180.0f))
  {
    std::string printable_characters;
    printable_characters.reserve(std::numeric_limits<char>::max());
    for (int c = 0; c < std::numeric_limits<char>::max(); ++c)
    {
      if (std::isprint(c))
      {
        printable_characters.append(1, c);
      }
    }

    _font->fillGlyphCache(*_cache, printable_characters.c_str());

    _renderer_2d =
      std::make_unique<Magnum::Text::Renderer2D>(*_font, *_cache, 32.0f, Magnum::Text::Alignment::MiddleLeft);
    _renderer_2d->reserve(kMaxTextLength, Magnum::GL::BufferUsage::DynamicDraw, Magnum::GL::BufferUsage::StaticDraw);
    _renderer_3d =
      std::make_unique<Magnum::Text::Renderer3D>(*_font, *_cache, 0.1f, Magnum::Text::Alignment::MiddleCenter);
    _renderer_3d->reserve(kMaxTextLength, Magnum::GL::BufferUsage::DynamicDraw, Magnum::GL::BufferUsage::StaticDraw);
  }
  else
  {
    log::error("Unable to initialise font ", font_resource_name, ". Text rendering will be unavailable.");
    _font = nullptr;
    _cache = nullptr;
  }

  // The constructor we call is to construct from *column* vectors, but for readability we layout *rows*
  // then transpose.
  // TODO(KS): this is only set for CoordiateFrame::XYZ. Do we need anything else? really adding the CoordinateFrame
  // transform to the projection matrix should be enough. We just need to line up the matrix so it defaults to -Y
  // facing, Z up.
  _default_transform = Magnum::Matrix4({
                                         Magnum::Vector4{ 1, 0, 0, 0 },  //
                                         Magnum::Vector4{ 0, 0, 1, 0 },  //
                                         Magnum::Vector4{ 0, 1, 0, 0 },  //
                                         Magnum::Vector4{ 0, 0, 0, 1 }   //
                                       })
                         .transposed();
}


bool Text::isAvailable() const
{
  return _font && _cache;
}


void Text::beginDraw()
{
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::Blending);
  // From magnum text examples: set up premultiplied alpha blending to avoid overlapping text characters to cut into
  // each other
  Magnum::GL::Renderer::setBlendFunction(Magnum::GL::Renderer::BlendFunction::One,
                                         Magnum::GL::Renderer::BlendFunction::OneMinusSourceAlpha);
  Magnum::GL::Renderer::setBlendEquation(Magnum::GL::Renderer::BlendEquation::Add,
                                         Magnum::GL::Renderer::BlendEquation::Add);
}


void Text::endDraw()
{
  Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::Blending);
}


void Text::beginDraw2D()
{
  beginDraw();
}


void Text::endDraw2D()
{
  endDraw();
}


void Text::beginDraw3D()
{
  beginDraw();
  // Draw double sided.
  Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::FaceCulling);
}


void Text::endDraw3D()
{
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::FaceCulling);
  endDraw();
}


void Text::draw2DText(const TextEntry &text, const DrawParams &params)
{
  // Adjust the position from [0, 1] to [-0.5, 0.5] with Y inverted so that +y is downs.
  auto text_position = text.transform[3].xyz();
  Magnum::Vector2 norm_position = {};
  if ((text.flags & TextFlag::ScreenProjected) == TextFlag::Zero)
  {
    norm_position = Magnum::Vector2(text_position.x() - 0.5f, 1 - text_position.y() - 0.5f);
  }
  else
  {
    auto projected_pos = params.projection_matrix.transformPoint(text_position);
    norm_position = { projected_pos.x(), projected_pos.y() };
    norm_position *= 0.5f;
  }

  // We try render text out range for a bit to allow long text to start offscreen. The right solution is to clip
  // properly, but this is enough for now.
  // TODO(KS): clip text correctly.
  if (norm_position.x() < -1 || norm_position.x() > 1 || norm_position.y() < -1 && norm_position.y() > 1)
  {
    return;
  }

  const auto view_size = Magnum::Vector2(params.view_size);
  const auto text_transform =
    Magnum::Matrix3::projection(view_size) * Magnum::Matrix3::translation(norm_position * view_size);
  draw(text, text_transform, *_renderer_2d, _shader_2d);
}


void Text::draw3DText(const TextEntry &text, const DrawParams &params)
{
  auto text_transform = text.transform;
  const auto text_position = text.transform[3].xyz();
  if ((text.flags & TextFlag::ScreenFacing) != TextFlag::Zero)
  {
    // Use the forward vector of the camera to orient the text.
    auto camera_fwd = params.camera_matrix[1].xyz();
    // Remove any height component from the camera.
    // FIXME(KS): make it work for alternative CoordinateFrame values to XYZ.
    camera_fwd[2] = 0;
    const float epsilon = 1e-3f;
    if (camera_fwd.dot() <= epsilon)
    {
      // Direction to too aligned with up. Try using the positions.
      camera_fwd = text_position - params.camera.position;
    }

    if (camera_fwd.dot() > epsilon)
    {
      camera_fwd = camera_fwd.normalized();
      const auto up = Magnum::Vector3::zAxis();
      auto side = Magnum::Math::cross(camera_fwd, up);
      // Build new rotation axes using camera_fwd for forward and a new Up axis.
      text_transform[0] = Magnum::Vector4(side.x(), side.y(), side.z(), 0);
      text_transform[1] = Magnum::Vector4(camera_fwd.x(), camera_fwd.y(), camera_fwd.z(), 0);
      text_transform[2] = Magnum::Vector4(up.x(), up.y(), up.z(), 0);
      text_transform[3] = Magnum::Vector4(text_position.x(), text_position.y(), text_position.z(), 1.0f);
    }
    // else cannot resolve.
  }

  // Apply scaling for font size.
  if (text.font_size)
  {
    text_transform = text_transform * Magnum::Matrix4::scaling(Magnum::Vector3(text.font_size));
  }

  draw(text, params.pv_transform * text_transform * _default_transform, *_renderer_3d, _shader_3d);
}


template <typename Matrix, typename Renderer, typename Shader>
void Text::draw(const TextEntry &text, const Matrix &full_projection_matrix, Renderer &renderer, Shader &shader)
{
  using namespace Magnum::Math::Literals;

  if (text.text.length() <= renderer.capacity())
  {
    renderer.render(text.text);
  }
  else
  {
    std::string truncated = text.text.substr(0, renderer.capacity());
    renderer.render(truncated);
  }

  shader.setTransformationProjectionMatrix(full_projection_matrix)
    .setColor(text.colour)
    .setOutlineColor(0x7f7f7f_rgbf)
    .setOutlineRange(0.45f, 0.35f)
    // .setSmoothness(0.025f / _transformationRotatingText.uniformScaling())
    .draw(renderer.mesh());
}
}  // namespace tes::viewer::painter
