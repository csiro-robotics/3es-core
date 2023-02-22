//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SHAPES_TEXT3D_H
#define TES_CORE_SHAPES_TEXT3D_H

#include <3escore/CoreConfig.h>

#include "Shape.h"

#include <cstdint>
#include <string>

namespace tes
{
/// A shape 3D world position and perspective adjusted text, optionally screen facing.
/// Expects UTF-8 encoding.
///
/// FIXME: add rotation support to the text, identifying the orientation axes.
class TES_CORE_API Text3D : public Shape
{
public:
  /// Construct a 3D text.
  /// @param text The text to display.
  /// @param id The shape id and category, with unique id among @c Text3D objects, or zero for a
  /// transient shape.
  /// @param transform Directional transformation for the text. The length is used to control the
  /// font size.
  Text3D(std::string text = {}, const Id &id = Id(), const Directional &transform = Directional());

  /// Copy constructor
  Text3D(const Text3D &other) = default;

  /// Move constructor
  Text3D(Text3D &&other) noexcept = default;

  ~Text3D() override = default;

  Text3D &operator=(const Text3D &other) = default;
  Text3D &operator=(Text3D &&other) noexcept = default;

  [[nodiscard]] const char *type() const override { return "text3D"; }

  Text3D &setScreenFacing(bool screen_facing);
  [[nodiscard]] bool screenFacing() const;

  Text3D &setFacing(const Vector3d &to_camera);
  [[nodiscard]] Vector3d facing() const;

  [[nodiscard]] double fontSize() const;
  Text3D &setFontSize(double size);

  [[nodiscard]] const std::string &text() const { return _text; }

  Text3D &setText(const std::string &text)
  {
    _text = text;
    return *this;
  }

  bool writeCreate(PacketWriter &stream) const override;

  bool readCreate(PacketReader &stream) override;

  [[nodiscard]] std::shared_ptr<Shape> clone() const override;

protected:
  void onClone(Text3D &copy) const;

private:
  std::string _text;
};


inline Text3D::Text3D(std::string text, const Id &id, const Directional &transform)
  : Shape(SIdText3D, id, transform)
  , _text(std::move(text))
{}


inline Text3D &Text3D::setScreenFacing(bool screen_facing)
{
  // NOLINTBEGIN(hicpp-signed-bitwise)
  _data.flags = static_cast<uint16_t>(_data.flags & ~Text3DFScreenFacing);
  _data.flags = static_cast<uint16_t>(_data.flags | Text3DFScreenFacing * !!screen_facing);
  // NOLINTEND(hicpp-signed-bitwise)
  return *this;
}


inline bool Text3D::screenFacing() const
{
  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  return (_data.flags & Text3DFScreenFacing) != 0;
}


inline Text3D &Text3D::setFacing(const Vector3d &to_camera)
{
  setScreenFacing(false);
  Quaterniond rot;
  const double dir_tolerance = 0.9998;
  if (to_camera.dot(Directional::DefaultDirection) > -dir_tolerance)
  {
    rot = Quaterniond(Directional::DefaultDirection, to_camera);
  }
  else
  {
    rot.setAxisAngle(Vector3d::AxisX, M_PI);
  }
  setRotation(rot);
  return *this;
}


inline Vector3d Text3D::facing() const
{
  const Quaterniond rot = rotation();
  return rot * Directional::DefaultDirection;
}


inline double Text3D::fontSize() const
{
  return _attributes.scale[2];
}


inline Text3D &Text3D::setFontSize(double size)
{
  _attributes.scale[2] = size;
  return *this;
}
}  // namespace tes

#endif  // TES_CORE_SHAPES_TEXT3D_H
