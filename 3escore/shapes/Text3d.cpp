//
// author: Kazys Stepanas
//
#include "Text3d.h"

#include <utility>

using namespace tes;

Text3D::Text3D(const Text3D &other)
  : Shape(other)
{
  setText(other.text(), other.textLength());
}

Text3D::Text3D(Text3D &&other)
  : Shape(other)
  , _text(std::exchange(other._text, nullptr))
  , _textLength(std::exchange(other._textLength, 0))
{}

Text3D::~Text3D()
{
  delete[] _text;
}


bool Text3D::writeCreate(PacketWriter &stream) const
{
  bool ok = true;
  stream.reset(routingId(), CreateMessage::MessageId);
  ok = _data.write(stream, _attributes) && ok;

  // Write line count and lines.
  const uint16_t textLength = _textLength;
  ok = stream.writeElement(textLength) == sizeof(textLength) && ok;

  if (textLength)
  {
    // Don't write null terminator.
    ok = stream.writeArray(_text, textLength) == sizeof(*_text) * textLength && ok;
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
  uint16_t textLength = 0;
  ok = ok && stream.readElement(textLength) == sizeof(textLength);

  if (_textLength < textLength)
  {
    delete[] _text;
    _text = new char[textLength + 1];
  }
  _textLength = textLength;

  if (_textLength)
  {
    _text[0] = '\0';
    ok = ok && stream.readArray(_text, textLength) == sizeof(*_text) * textLength;
    _text[textLength] = '\0';
  }

  return ok;
}


Text3D &Text3D::operator=(const Text3D &other)
{
  _data = other._data;
  setText(other.text(), other.textLength());
  return *this;
}


Text3D &Text3D::operator=(Text3D &&other)
{
  _data = other._data;
  _text = other._text;
  _textLength = other._textLength;
  other._text = nullptr;
  other._textLength = 0;
  return *this;
}

Shape *Text3D::clone() const
{
  Text3D *copy = new Text3D(nullptr, Id());
  onClone(copy);
  return copy;
}


void Text3D::onClone(Text3D *copy) const
{
  Shape::onClone(copy);
  copy->setText(_text, _textLength);
}


Text3D &Text3D::setText(const char *text, uint16_t textLength)
{
  delete[] _text;
  _text = nullptr;
  _textLength = 0;
  if (text && textLength)
  {
    _text = new char[textLength + 1];
    _textLength = textLength;
#ifdef _MSC_VER
    strncpy_s(_text, _textLength + 1, text, textLength);
#else   // _MSC_VER
    strncpy(_text, text, textLength);
#endif  // _MSC_VER
    _text[textLength] = '\0';
  }
  return *this;
}
