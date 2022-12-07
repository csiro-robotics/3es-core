//
// Author: Kazys Stepanas
//
#include "3estext2d.h"

#include "3esmagnumcolour.h"

#include <3esconnection.h>
#include <3eslog.h>
#include <shapes/3estext2d.h>

#include <Magnum/Math/Matrix3.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/DistanceFieldGlyphCache.h>

#include <cctype>
#include <limits>

namespace tes::viewer::handler
{
Text2D::Text2D(Magnum::Text::AbstractFont *font, std::shared_ptr<Magnum::Text::DistanceFieldGlyphCache> cache)
  : Message(SIdText2D, "text 2D")
  , _font(font)
  , _cache(std::move(cache))
{
  if (_font && _cache)
  {
    _renderer = std::make_unique<Magnum::Text::Renderer2D>(*_font, *_cache, 32.0f, Magnum::Text::Alignment::MiddleLeft);
    _renderer->reserve(255, Magnum::GL::BufferUsage::DynamicDraw, Magnum::GL::BufferUsage::StaticDraw);
  }
  else
  {
    log::error("Text 2D not given a valid font and cache. Text 2D rendering will be disabled.");
  }
}


void Text2D::initialise()
{}


void Text2D::reset()
{
  std::lock_guard guard(_mutex);
  _pending.clear();
  _transient.clear();
  _remove.clear();
  _text.clear();
}


void Text2D::beginFrame(const FrameStamp &stamp)
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


void Text2D::endFrame(const FrameStamp &stamp)
{}


void Text2D::draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params)
{
  if (pass != DrawPass::Overlay)
  {
    return;
  }

  if (!_font || !_cache || !_renderer)
  {
    return;
  }

  _shader.bindVectorTexture(_cache->texture());

  for (const auto &text : _transient)
  {
    draw(text, params);
  }

  for (const auto &[id, text] : _text)
  {
    draw(text, params);
  }
}


void Text2D::readMessage(PacketReader &reader)
{
  switch (reader.messageId())
  {
  case OIdCreate: {
    tes::Text2D shape;
    if (!shape.readCreate(reader))
    {
      log::error("Failed to create text 2D create.");
      return;
    }

    TextEntry text;
    text.id = shape.id();
    text.text = std::string(shape.text(), shape.textLength());
    text.position.x() = shape.position().x;
    text.position.y() = shape.position().y;
    text.position.z() = shape.position().z;
    text.colour = convert(shape.colour());
    text.world_projected = shape.inWorldSpace();

    _pending.emplace_back(text);
    break;
  }
  case OIdDestroy: {
    DestroyMessage msg;
    if (!msg.read(reader))
    {
      return log::error("Failed to read text 2D destroy.");
    }
    _remove.emplace_back(msg.id);
    break;
  }
  default:
    log::error("Unsupported text 2D message ID: ", reader.messageId());
    break;
  }
}


void Text2D::serialise(Connection &out, ServerInfoMessage &info)
{
  tes::Text2D shape;

  const auto write_shape = [&out, &shape](const TextEntry &text) {
    shape.setId(text.id);
    shape.setText(text.text.c_str(), uint16_t(text.text.length()));
    shape.setPosition(tes::Vector3f(text.position.x(), text.position.y(), text.position.z()));
    shape.setInWorldSpace(text.world_projected);
    if (out.create(shape) < 0)
    {
      log::error("Error writing text 2D shape.");
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


void Text2D::draw(const TextEntry &text, const DrawParams &params)
{
  using namespace Magnum::Math::Literals;

  // Adjust the position from [0, 1] to [-0.5, 0.5] with Y inverted so that +y is downs.
  Magnum::Vector2 norm_position = {};
  if (!text.world_projected)
  {
    norm_position = Magnum::Vector2(text.position.x() - 0.5f, 1 - text.position.y() - 0.5f);
  }
  else
  {
    auto projected_pos = params.projection_matrix.transformPoint(text.position);
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

  // Expand buffers as required.
  if (_renderer->capacity() < text.text.length())
  {
    _renderer->reserve(text.text.length(), Magnum::GL::BufferUsage::DynamicDraw, Magnum::GL::BufferUsage::StaticDraw);
  }
  _renderer->render(text.text);

  auto transform =
    Magnum::Matrix3::projection(params.view_size) * Magnum::Matrix3::translation(norm_position * params.view_size);
  _shader.setTransformationProjectionMatrix(transform)
    .setColor(text.colour)
    // .setOutlineColor(0xdcdcdc_rgbf)
    // .setOutlineRange(0.25f, 0.15f)
    // .setSmoothness(0.025f / _transformationRotatingText.uniformScaling())
    .draw(_renderer->mesh());
}  // namespace tes::viewer::handler
}  // namespace tes::viewer::handler
