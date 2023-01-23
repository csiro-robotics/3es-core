//
// Author: Kazys Stepanas
//
#include "Text3d.h"

#include <3esview/MagnumColour.h>

#include <3escore/Connection.h>
#include <3escore/Log.h>
#include <3escore/shapes/Text3d.h>

#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Quaternion.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/DistanceFieldGlyphCache.h>

#include <cctype>
#include <limits>

namespace tes::view::handler
{
Text3D::Text3D(std::shared_ptr<painter::Text> painter)
  : Message(SIdText3D, "text 3D")
  , _painter(std::move(painter))
{}


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


void Text3D::beginFrame(const FrameStamp &stamp)
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


void Text3D::endFrame(const FrameStamp &stamp)
{
  (void)stamp;
}


void Text3D::draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params)
{
  (void)stamp;
  (void)params;
  if (pass != DrawPass::Opaque)
  {
    return;
  }

  _painter->draw3D(
    _transient.begin(), _transient.end(), [](const std::vector<TextEntry>::iterator &iter) { return *iter; }, params);
  _painter->draw3D(
    _text.begin(), _text.end(),
    [](const std::unordered_map<uint32_t, TextEntry>::iterator &iter) { return iter->second; }, params);
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
    text.text = std::string(shape.text(), shape.textLength());
    // Read font size which comes from scale.
    text.font_size = Magnum::Float(shape.fontSize());
    // Remove the font size scaling before we compose the transform.
    shape.setFontSize(1.0);
    text.transform = Message::composeTransform(shape.attributes());
    text.colour = convert(shape.colour());
    if (shape.screenFacing())
    {
      text.flags |= painter::Text::TextFlag::ScreenFacing;
    }

    _pending.emplace_back(shape.id(), text);
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
  (void)info;
  tes::Text3D shape;

  const auto write_shape = [&out, &shape](uint32_t id, const TextEntry &text) {
    shape.setId(id);
    shape.setText(text.text.c_str(), uint16_t(text.text.length()));
    ObjectAttributes attrs = {};
    Message::decomposeTransform(text.transform, attrs);
    shape.setPosition(tes::Vector3f(attrs.position[0], attrs.position[1], attrs.position[2]));
    shape.setRotation(tes::Quaternionf(attrs.rotation[0], attrs.rotation[1], attrs.rotation[2], attrs.rotation[3]));
    shape.setScale(tes::Vector3f(attrs.scale[0], attrs.scale[1], attrs.scale[2]));
    // Set the font size, which will adjust the scale.
    shape.setFontSize(text.font_size);
    shape.setScreenFacing((text.flags & painter::Text::TextFlag::ScreenFacing) != painter::Text::TextFlag::Zero);
    if (out.create(shape) < 0)
    {
      log::error("Error writing text 3D shape.");
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
