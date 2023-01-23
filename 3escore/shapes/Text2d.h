//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SHAPES_TEXT2D_H
#define TES_CORE_SHAPES_TEXT2D_H

#include <3escore/CoreConfig.h>

#include "Shape.h"

#include <cstdint>
#include <cstring>

namespace tes
{
/// A shape which renders screen space text, optionally positioned in 3D.
///
/// Positioning is in normalised screen coordinates.
/// Expects UTF-8 encoding.
class TES_CORE_API Text2D : public Shape
{
public:
  /// Construct a 2D text.
  /// @param text The text to display.
  /// @param id The shape id and category, with unique id among @c Text2D objects, or zero for a transient shape.
  /// @param pos The position of the text.
  Text2D(const char *text = "", const Id &id = Id(), const Spherical &pos = Spherical());

  /// Copy constructor
  /// @param other Object to copy.
  Text2D(const Text2D &other);

  /// Move constructor
  /// @param other Object to move.
  Text2D(Text2D &&other);

  ~Text2D();

  inline const char *type() const override { return "text2D"; }

  bool inWorldSpace() const;
  Text2D &setInWorldSpace(bool worldSpace);

  inline char *text() const { return _text; }
  inline uint16_t textLength() const { return _textLength; }

  Text2D &setText(const char *text, uint16_t textLength);

  bool writeCreate(PacketWriter &stream) const override;

  bool readCreate(PacketReader &stream) override;

  Text2D &operator=(const Text2D &other);
  Text2D &operator=(Text2D &&other);

  Shape *clone() const override;

protected:
  void onClone(Text2D *copy) const;

private:
  char *_text = nullptr;
  uint16_t _textLength = 0;
};


inline Text2D::Text2D(const char *text, const Id &id, const Spherical &pos)
  : Shape(SIdText2D, id, pos)
{
  setText(text, text ? (uint16_t)strlen(text) : 0);
}

inline bool Text2D::inWorldSpace() const
{
  return (_data.flags & Text2DFWorldSpace) != 0;
}


inline Text2D &Text2D::setInWorldSpace(bool worldSpace)
{
  _data.flags = uint16_t(_data.flags & ~Text2DFWorldSpace);
  _data.flags = uint16_t(_data.flags | Text2DFWorldSpace * !!worldSpace);
  return *this;
}
}  // namespace tes

#endif  // TES_CORE_SHAPES_TEXT2D_H
