//
// author: Kazys Stepanas
//
#include "Text3D.h"

#include <3escore/CoreUtil.h>

#include <utility>

namespace tes
{
bool Text3D::writeCreate(PacketWriter &stream) const
{
  bool ok = true;
  stream.reset(routingId(), CreateMessage::MessageId);
  ok = _data.write(stream, _attributes) && ok;

  // Write line count and lines.
  const auto text_length = int_cast<uint16_t>(_text.size());
  ok = stream.writeElement(text_length) == sizeof(text_length) && ok;

  if (text_length)
  {
    // Don't write null terminator.
    ok = stream.writeArray(_text.data(), text_length) == sizeof(*_text.data()) * text_length && ok;
  }

  return ok;
}


bool Text3D::readCreate(PacketReader &stream)
{
  if (!Shape::readCreate(stream))
  {
    return false;
  }

  bool ok = true;
  uint16_t text_length = 0;
  ok = ok && stream.readElement(text_length) == sizeof(text_length);

  if (text_length == 0)
  {
    _text = std::string();
    return ok;
  }

  _text.resize(text_length);
  ok = ok && stream.readArray(_text.data(), text_length) == sizeof(*_text.data()) * text_length;

  return ok;
}


std::shared_ptr<Shape> Text3D::clone() const
{
  auto copy = std::make_shared<Text3D>(std::string(), Id());
  onClone(*copy);
  return copy;
}


void Text3D::onClone(Text3D &copy) const
{
  Shape::onClone(copy);
  copy._text = _text;
}
}  // namespace tes
