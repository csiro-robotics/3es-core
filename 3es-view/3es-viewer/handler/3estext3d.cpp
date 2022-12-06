//
// Author: Kazys Stepanas
//
#include "3estext3d.h"

#include "3esmagnumcolour.h"

#include <3esconnection.h>
#include <3eslog.h>
#include <shapes/3estext3d.h>

#include <Magnum/Math/Matrix4.h>
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
    _renderer = std::make_unique<Magnum::Text::Renderer3D>(*_font, *_cache, 32.0f, Magnum::Text::Alignment::MiddleLeft);
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
{}


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
    draw(text, params.projection_matrix);
  }

  for (const auto &[id, text] : _text)
  {
    draw(text, params.projection_matrix);
  }
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
    text.transform = Message::composeTransform(shape.attributes());
    text.colour = convert(shape.colour());

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


void Text3D::draw(const TextEntry &text, const Magnum::Matrix4 &projection_matrix)
{
  using namespace Magnum::Math::Literals;

  // Expand buffers as required.
  if (_renderer->capacity() < text.text.length())
  {
    _renderer->reserve(text.text.length(), Magnum::GL::BufferUsage::DynamicDraw, Magnum::GL::BufferUsage::StaticDraw);
  }
  _renderer->render(text.text);

  _shader.setTransformationProjectionMatrix(projection_matrix * text.transform)
    .setColor(text.colour)
    // .setOutlineColor(0xdcdcdc_rgbf)
    // .setOutlineRange(0.25f, 0.15f)
    // .setSmoothness(0.025f / _transformationRotatingText.uniformScaling())
    .draw(_renderer->mesh());
}
}  // namespace tes::viewer::handler
