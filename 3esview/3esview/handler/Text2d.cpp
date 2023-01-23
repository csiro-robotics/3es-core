//
// Author: Kazys Stepanas
//
#include "Text2d.h"

#include <3esview/MagnumColour.h>
#include <3esview/MagnumV3.h>

#include <3escore/Connection.h>
#include <3escore/Log.h>
#include <3escore/shapes/Text2d.h>

#include <Magnum/Math/Matrix3.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/DistanceFieldGlyphCache.h>

#include <cctype>
#include <limits>

namespace tes::view::handler
{
Text2D::Text2D(std::shared_ptr<painter::Text> painter)
  : Message(SIdText2D, "text 2D")
  , _painter(std::move(painter))
{}


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
  (void)stamp;
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

  for (const auto &[id, text] : _pending)
  {
    if (tes::Id(id).isTransient())
    {
      _transient.emplace_back(text);
    }
    else
    {
      _text[id] = text;
    }
  }
  _pending.clear();
}


void Text2D::endFrame(const FrameStamp &stamp)
{
  (void)stamp;
}


void Text2D::draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params)
{
  (void)stamp;
  (void)params;
  if (pass != DrawPass::Overlay)
  {
    return;
  }

  _painter->draw2D(
    _transient.begin(), _transient.end(), [](const std::vector<TextEntry>::iterator &iter) { return *iter; }, params);
  _painter->draw2D(
    _text.begin(), _text.end(),
    [](const std::unordered_map<uint32_t, TextEntry>::iterator &iter) { return iter->second; }, params);
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
    text.text = std::string(shape.text(), shape.textLength());
    text.transform = Magnum::Matrix4::translation(convert(shape.position()));
    text.colour = convert(shape.colour());
    if (shape.inWorldSpace())
    {
      text.flags |= painter::Text::TextFlag::ScreenProjected;
    }

    _pending.emplace_back(shape.id(), text);
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
  (void)info;
  tes::Text2D shape;

  const auto write_shape = [&out, &shape](uint32_t id, const TextEntry &text) {
    shape.setId(id);
    shape.setText(text.text.c_str(), uint16_t(text.text.length()));
    const auto position = text.transform[3].xyz();
    shape.setPosition(tes::Vector3f(position.x(), position.y(), position.z()));
    shape.setInWorldSpace((text.flags & painter::Text::TextFlag::ScreenProjected) != painter::Text::TextFlag::Zero);
    if (out.create(shape) < 0)
    {
      log::error("Error writing text 2D shape.");
    }
  };

  for (const auto &text : _transient)
  {
    write_shape(0, text);
  }

  for (const auto &[id, text] : _text)
  {
    write_shape(id, text);
  }
}
}  // namespace tes::view::handler
