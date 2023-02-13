//
// author: Kazys Stepanas
//
#include "Text2d.h"

#include <utility>

namespace tes
{
Text2D::Text2D(const Text2D &other) = default;

Text2D::Text2D(Text2D &&other) noexcept = default;

Text2D::~Text2D() = default;


bool Text2D::writeCreate(PacketWriter &stream) const
{
  bool ok = true;
  stream.reset(routingId(), CreateMessage::MessageId);
  ok = _data.write(stream, _attributes) && ok;

  // Write line count and lines.
  const uint16_t text_length = textLength();
  ok = stream.writeElement(text_length) == sizeof(text_length) && ok;

  if (text_length)
  {
    // Don't write null terminator.
    ok = stream.writeArray(_text.data(), text_length) == sizeof(*_text.data()) * text_length && ok;
  }

  return ok;
}


bool Text2D::readCreate(PacketReader &stream)
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


Text2D &Text2D::operator=(const Text2D &other) = default;


Text2D &Text2D::operator=(Text2D &&other) noexcept = default;

std::shared_ptr<Shape> Text2D::clone() const
{
  auto copy = std::make_shared<Text2D>(std::string(), Id());
  onClone(*copy);
  return copy;
}


void Text2D::onClone(Text2D &copy) const
{
  Shape::onClone(copy);
  copy._text = _text;
}
}  // namespace tes
