//
// Author: Kazys Stepanas
//
#include "3estext2d.h"

#include "3esmagnumcolour.h"

#include <3esconnection.h>
#include <3eslog.h>
#include <shapes/3estext2d.h>

#include <Corrade/Utility/Resource.h>
#include <Magnum/Math/Matrix3.h>

#include <cctype>
#include <limits>

namespace tes::viewer::handler
{
Text2D::Text2D(const std::string &font_name, const std::string &fonts_resources)
  : Message(SIdText2D, "text 2D")
  , _cache(Magnum::Vector2i(2048), Magnum::Vector2i(512), 22)
{
  Corrade::Utility::Resource rs(fonts_resources);
  _font = _manager.loadAndInstantiate("TrueTypeFont");
  if (!_font || !_font->openData(rs.getRaw(font_name), 180.0f))
  {
    log::error("Unable to initialise font ", font_name);
    _font = nullptr;
  }
  else
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

    _font->fillGlyphCache(_cache, printable_characters.c_str());

    _renderer = std::make_unique<Magnum::Text::Renderer2D>(*_font, _cache, 32.0f, Magnum::Text::Alignment::MiddleLeft);
    _renderer->reserve(255, Magnum::GL::BufferUsage::DynamicDraw, Magnum::GL::BufferUsage::StaticDraw);
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


void Text2D::updateServerInfo(const ServerInfoMessage &info)
{}


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

  if (!_font)
  {
    return;
  }

  _shader.bindVectorTexture(_cache.texture());

  for (const auto &text : _transient)
  {
    draw(text, params.view_size);
  }

  for (const auto &[id, text] : _text)
  {
    draw(text, params.view_size);
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
    text.colour = convert(shape.colour());

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
    shape.setPosition(tes::Vector3f(text.position.x(), text.position.y(), 0.0f));
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


void Text2D::draw(const TextEntry &text, const Magnum::Vector2 &view_size)
{
  using namespace Magnum::Math::Literals;

  // Expand buffers as required.
  if (_renderer->capacity() < text.text.length())
  {
    _renderer->reserve(text.text.length(), Magnum::GL::BufferUsage::DynamicDraw, Magnum::GL::BufferUsage::StaticDraw);
  }
  _renderer->render(text.text);

  // Adjust the position from [0, 1] to [-0.5, 0.5] with Y inverted so that +y is downs.
  const auto norm_position = Magnum::Vector2(text.position.x() - 0.5f, 1 - text.position.y() - 0.5f);
  auto transform = Magnum::Matrix3::projection(view_size) * Magnum::Matrix3::translation(norm_position * view_size);
  _shader.setTransformationProjectionMatrix(transform)
    .setColor(text.colour)
    // .setOutlineColor(0xdcdcdc_rgbf)
    // .setOutlineRange(0.25f, 0.15f)
    // .setSmoothness(0.025f / _transformationRotatingText.uniformScaling())
    .draw(_renderer->mesh());
}
}  // namespace tes::viewer::handler
