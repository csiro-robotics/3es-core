//
// author: Kazys Stepanas
//
#ifndef _3ESTEXT3D_H_
#define _3ESTEXT3D_H_

#include "3es-core.h"

#include "3esshape.h"

#include <cstdint>
#include <cstring>

namespace tes
{
/// A shape 3D world position and perspective adjusted text, optionally screen facing.
/// Expects UTF-8 encoding.
///
/// FIXME: add rotation support to the text, identifying the orientation axes.
class _3es_coreAPI Text3D : public Shape
{
public:
  /// Construct a 3D text.
  /// @param text The text to display.
  /// @param id The shape id and category, with unique id among @c Text3D objects, or zero for a transient shape.
  /// @param transform Directional transformation for the text. The length is used to control the font size.
  Text3D(const char *text = "", const ShapeId &id = ShapeId(), const Directional &transform = Directional());

  /// Copy constructor
  Text3D(const Text3D &other);
  /// Move constructor
  Text3D(Text3D &&other);

  ~Text3D();

  inline const char *type() const override { return "text3D"; }

  bool screenFacing() const;
  Text3D &setScreenFacing(bool worldSpace);

  Text3D &setFacing(const Vector3d &toCamera);
  Vector3d facing() const;

  double fontSize() const;
  Text3D &setFontSize(double size);

  inline char *text() const { return _text; }
  inline uint16_t textLength() const { return _textLength; }

  Text3D &setText(const char *text, uint16_t textLength);

  virtual bool writeCreate(PacketWriter &stream) const override;

  bool readCreate(PacketReader &stream) override;

  Text3D &operator=(const Text3D &other);
  Text3D &operator=(Text3D &&other);

  Shape *clone() const override;

protected:
  void onClone(Text3D *copy) const;

private:
  char *_text;
  uint16_t _textLength;
};


inline Text3D::Text3D(const char *text, const ShapeId &id, const Directional &transform)
  : Shape(SIdText3D, id, transform)
  , _text(nullptr)
  , _textLength(0)
{
  setText(text, text ? (uint16_t)strlen(text) : 0);
}


inline bool Text3D::screenFacing() const
{
  return (_data.flags & Text3DFScreenFacing) != 0;
}


inline Text3D &Text3D::setScreenFacing(bool worldSpace)
{
  _data.flags = uint16_t(_data.flags & ~Text3DFScreenFacing);
  _data.flags = uint16_t(_data.flags | Text3DFScreenFacing * !!worldSpace);
  return *this;
}


inline Text3D &Text3D::setFacing(const Vector3d &toCamera)
{
  setScreenFacing(false);
  Quaterniond rot;
  if (toCamera.dot(Directional::DefaultDirection) > -0.9998f)
  {
    rot = Quaterniond(Directional::DefaultDirection, toCamera);
  }
  else
  {
    rot.setAxisAngle(Vector3d::axisx, M_PI);
  }
  setRotation(rot);
  return *this;
}


inline Vector3d Text3D::facing() const
{
  Quaterniond rot = rotation();
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

#endif  // _3ESTEXT3D_H_
