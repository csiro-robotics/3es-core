//
// author: Kazys Stepanas
//
#include "Text3d.h"

#include <3escore/CoreUtil.h>

#include <utility>

namespace tes
{
Text3D::Text3D(const Text3D &other) = default;

Text3D::Text3D(Text3D &&other) noexcept = default;

Text3D::~Text3D() = default;


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

  _text.resize(text_length + 1);
  _text[0] = '\0';
  ok = ok && stream.readArray(_text.data(), text_length) == sizeof(*_text.data()) * text_length;
  _text[text_length] = '\0';

  return ok;
}


Text3D &Text3D::operator=(const Text3D &other) = default;


Text3D &Text3D::operator=(Text3D &&other) noexcept = default;

Shape *Text3D::clone() const
{
  auto *copy = new Text3D(text(), Id());
  onClone(copy);
  return copy;
}


void Text3D::onClone(Text3D *copy) const
{
  Shape::onClone(copy);
}
}  // namespace tes
