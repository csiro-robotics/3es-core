//
// Author: Kazys Stepanas
//
#include "3estext3d.h"

#include "3esmagnumcolour.h"

#include <3esconnection.h>
#include <3eslog.h>
#include <shapes/3estext3d.h>

#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Quaternion.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/DistanceFieldGlyphCache.h>

#include <cctype>
#include <limits>

namespace tes::viewer::handler
{
Text3D::Text3D(Magnum::Text::AbstractFont *font, std::shared_ptr<Magnum::Text::DistanceFieldGlyphCache> cache)
  : Message(SIdText3D, "text 3D")
  , _font(font)
  , _cache(std::move(cache))
{
  if (_font && _cache)
  {
    _renderer =
      std::make_unique<Magnum::Text::Renderer3D>(*_font, *_cache, 0.1f, Magnum::Text::Alignment::MiddleCenter);
    _renderer->reserve(255, Magnum::GL::BufferUsage::DynamicDraw, Magnum::GL::BufferUsage::StaticDraw);
  }
  else
  {
    log::error("Text 3D not given a valid font and cache. Text 3D rendering will be disabled.");
  }
}


void Text3D::initialise()
{}


void Text3D::reset()
{
  std::lock_guard guard(_mutex);
  _pending.clear();
  _transient.clear();
  _remove.clear();
  _text.clear();
}


void Text3D::updateServerInfo(const ServerInfoMessage &info)
{
  Message::updateServerInfo(info);
  // The constructor we call is to construct from *column* vectors, but for readability we layout *rows* then transpose.
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


void Text3D::beginFrame(const FrameStamp &stamp)
{
  std::lock_guard guard(_mutex);
  _transient.clear();

  for (const auto id : _remove)
  {
    auto iter = _text.find(id);
    if (iter != _text.end())
    {
      _text.erase(iter);
    }
  }
  _remove.clear();

  for (const auto &text : _pending)
  {
    const tes::Id id(text.id);

    if (id.isTransient())
    {
      _transient.emplace_back(text);
    }
    else
    {
      _text[text.id] = text;
    }
  }
  _pending.clear();
}


void Text3D::endFrame(const FrameStamp &stamp)
{}


void Text3D::draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params)
{
  if (pass != DrawPass::Opaque)
  {
    return;
  }

  if (!_font || !_cache || !_renderer)
  {
    return;
  }

  _shader.bindVectorTexture(_cache->texture());

  // Draw double sided.
  Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::FaceCulling);
  for (const auto &text : _transient)
  {
    draw(text, params);
  }

  for (const auto &[id, text] : _text)
  {
    draw(text, params);
  }
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::FaceCulling);
}


void Text3D::readMessage(PacketReader &reader)
{
  switch (reader.messageId())
  {
  case OIdCreate: {
    tes::Text3D shape;
    if (!shape.readCreate(reader))
    {
      log::error("Failed to create text 3D create.");
      return;
    }

    TextEntry text;
    text.id = shape.id();
    text.text = std::string(shape.text(), shape.textLength());
    // Read font size which comes from scale.
    text.font_size = shape.fontSize();
    // Remove the font size scaling before we compose the transform.
    shape.setFontSize(1.0);
    text.transform = Message::composeTransform(shape.attributes());
    text.colour = convert(shape.colour());
    text.screen_facing = shape.screenFacing();

    _pending.emplace_back(text);
    break;
  }
  case OIdDestroy: {
    DestroyMessage msg;
    if (!msg.read(reader))
    {
      return log::error("Failed to read text 3D destroy.");
    }
    _remove.emplace_back(msg.id);
    break;
  }
  default:
    log::error("Unsupported text 3D message ID: ", reader.messageId());
    break;
  }
}


void Text3D::serialise(Connection &out, ServerInfoMessage &info)
{
  tes::Text3D shape;

  const auto write_shape = [&out, &shape](const TextEntry &text) {
    shape.setId(text.id);
    shape.setText(text.text.c_str(), uint16_t(text.text.length()));
    ObjectAttributes attrs = {};
    Message::decomposeTransform(text.transform, attrs);
    shape.setPosition(tes::Vector3f(attrs.position[0], attrs.position[1], attrs.position[2]));
    shape.setRotation(tes::Quaternionf(attrs.rotation[0], attrs.rotation[1], attrs.rotation[2], attrs.rotation[3]));
    shape.setScale(tes::Vector3f(attrs.scale[0], attrs.scale[1], attrs.scale[2]));
    // Set the font size, which will adjust the scale.
    shape.setFontSize(text.font_size);
    shape.setScreenFacing(text.screen_facing);
    if (out.create(shape) < 0)
    {
      log::error("Error writing text 3D shape.");
    }
  };

  for (const auto &text : _transient)
  {
    write_shape(text);
  }

  for (const auto &[id, text] : _text)
  {
    write_shape(text);
  }
}


void Text3D::draw(const TextEntry &text, const DrawParams &params)
{
  using namespace Magnum::Math::Literals;

  // TODO(KS): culling

  // Expand buffers as required.
  if (_renderer->capacity() < text.text.length())
  {
    _renderer->reserve(text.text.length(), Magnum::GL::BufferUsage::DynamicDraw, Magnum::GL::BufferUsage::StaticDraw);
  }
  _renderer->render(text.text);

  auto text_transform = text.transform;
  const auto text_position = text.transform[3].xyz();
  if (text.screen_facing)
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
  else
  {
    // text_transform = Magnum::Matrix4::translation(text_position);
  }

  // Apply scaling for font size.
  if (text.font_size)
  {
    text_transform = text_transform * Magnum::Matrix4::scaling(Magnum::Vector3(text.font_size));
  }

  // _shader.setTransformationProjectionMatrix(projection_matrix * text.transform)
  _shader.setTransformationProjectionMatrix(params.projection_matrix * text_transform * _default_transform)
    .setColor(text.colour)
    // .setOutlineColor(0xdcdcdc_rgbf)
    // .setOutlineRange(0.25f, 0.15f)
    // .setSmoothness(0.025f / _transformationRotatingText.uniformScaling())
    .draw(_renderer->mesh());
}
}  // namespace tes::viewer::handler
