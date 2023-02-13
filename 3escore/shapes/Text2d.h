//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SHAPES_TEXT2D_H
#define TES_CORE_SHAPES_TEXT2D_H

#include <3escore/CoreConfig.h>

#include "Shape.h"

#include <3escore/CoreUtil.h>

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
  /// @param id The shape id and category, with unique id among @c Text2D objects, or zero for a
  /// transient shape.
  /// @param pos The position of the text.
  Text2D(std::string text = {}, const Id &id = Id(), const Spherical &pos = Spherical());

  /// Copy constructor
  /// @param other Object to copy.
  Text2D(const Text2D &other);

  /// Move constructor
  /// @param other Object to move.
  Text2D(Text2D &&other) noexcept;

  ~Text2D() override;

  [[nodiscard]] const char *type() const override { return "text2D"; }

  [[nodiscard]] bool inWorldSpace() const;
  Text2D &setInWorldSpace(bool world_space);

  [[nodiscard]] const std::string &text() const { return _text; }
  [[nodiscard]] uint16_t textLength() const { return int_cast<uint16_t>(_text.size()); }

  Text2D &setText(const std::string &text)
  {
    _text = text;
    return *this;
  }

  bool writeCreate(PacketWriter &stream) const override;

  bool readCreate(PacketReader &stream) override;

  Text2D &operator=(const Text2D &other);
  Text2D &operator=(Text2D &&other) noexcept;

  [[nodiscard]] std::shared_ptr<Shape> clone() const override;

protected:
  void onClone(Text2D &copy) const;

private:
  std::string _text;
};


inline Text2D::Text2D(std::string text, const Id &id, const Spherical &pos)
  : Shape(SIdText2D, id, pos)
  , _text(std::move(text))
{}

inline bool Text2D::inWorldSpace() const
{
  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  return (_data.flags & Text2DFWorldSpace) != 0;
}


inline Text2D &Text2D::setInWorldSpace(bool world_space)
{
  // NOLINTBEGIN(hicpp-signed-bitwise)
  _data.flags = static_cast<uint16_t>(_data.flags & ~Text2DFWorldSpace);
  _data.flags = static_cast<uint16_t>(_data.flags | Text2DFWorldSpace * !!world_space);
  // NOLINTEND(hicpp-signed-bitwise)
  return *this;
}
}  // namespace tes

#endif  // TES_CORE_SHAPES_TEXT2D_H
