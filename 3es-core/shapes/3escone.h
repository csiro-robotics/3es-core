//
// author: Kazys Stepanas
//
#ifndef _3ESCONE_H_
#define _3ESCONE_H_

#include "3es-core.h"
#include "3esshape.h"

namespace tes
{
/// Defines a cone shape to display.
///
/// A cone is defined by:
/// Component      | Description
/// -------------- | -----------------------------------------------------------------------------------------------
/// @c point()     | The cone apex position. Alias for @c position().
/// @c direction() | The direction from the apex the cone flanges out.
/// @c length()    | Scaling value for the arrow. Defines the true length when @c direction() is unit length.
/// @c angle()     | Angle cone axis to the walls at the apex.
class _3es_coreAPI Cone : public Shape
{
public:
  /// Default direction used as a reference orientation for packing the rotation.
  ///
  /// The @c rotation() value is relative to this vector.
  ///
  /// The default is <tt>(0, 0, 1)</tt>
  static const Vector3d DefaultDir;

  /// Construct a cone object.
  /// @param id The shape ID, unique among @c Cone objects, or zero for a transient shape.
  /// @param category The category grouping for the shape used for filtering.
  /// @param point Defines the cone apex.
  /// @param dir Cone direction.
  /// @param angle Angle from cone axis to walls at the apex (radians).
  /// @param length Length of the cone from apex to base.
  Cone(const Id &id = Id(), const Vector3d &point = Vector3d(0, 0, 0), const Vector3d &dir = DefaultDir,
       float angle = 45.0f / 180.0f * float(M_PI), float length = 1.0f);

  inline const char *type() const override { return "cone"; }

  /// Sets the cone angle at the apex (radians).
  /// @param angle The angle to set (radians).
  /// @return @c *this
  Cone &setAngle(float angle);
  /// Get the cone angle at the apex (radians).
  /// @return The cone angle (radians).
  float angle() const;

  /// Set the cone length, apex to base.
  /// @param length The length to set.
  /// @return @c *this
  Cone &setLength(float length);
  /// Get the cone length, apex to base.
  /// @return The cone length.
  float length() const;

  /// Set the position of the cone apex.
  /// @param point The apex coordinate.
  /// @return @c *this
  Cone &setPoint(const Vector3d &point);
  /// Get the position of the cone apex.
  /// @return point The apex coordinate.
  Vector3d point() const;

  /// Set the cone direction vector.
  /// @param direction The direction vector to set. Must be unit length.
  /// @return @c *this
  Cone &setDirection(const Vector3d &dir);
  /// Get the cone direction vector.
  ///
  /// May not exactly match the axis given via @p setDirection() as the direction is defined by the quaternion
  /// @c rotation().
  /// @return The arrow direction vector.
  Vector3d direction() const;
};


inline Cone::Cone(const Id &id, const Vector3d &point, const Vector3d &dir, float angle, float length)
  : Shape(SIdCone, id, Transform(point, Vector3d(angle, angle, length)))
{
  setDirection(dir);
}


inline Cone &Cone::setAngle(float angle)
{
  Vector3d s = scale();
  s.x = s.y = s.z * std::tan(angle);
  setScale(s);
  return *this;
}


inline float Cone::angle() const
{
  return scale().x;
  // scale X/Y encode the radius of the cone base.
  // Convert to angle angle as:
  //   tan(theta) = radius / length
  //   theta = atan(radius / length)
  const Vector3d s = scale();
  const float length = s.z;
  const float radius = s.x;
  return (length != 0.0f) ? std::atan(radius / length) : 0.0f;
}


inline Cone &Cone::setLength(float length)
{
  // Changing the length requires maintaining the angle, so we must adjust the radius to suit.
  const float angle = this->angle();
  _data.attributes.scale[2] = length;
  setAngle(angle);
  return *this;
}


inline float Cone::length() const
{
  return scale().z;
}


inline Cone &Cone::setPoint(const Vector3d &point)
{
  setPosition(point);
  return *this;
}


inline Vector3d Cone::point() const
{
  return position();
}


inline Cone &Cone::setDirection(const Vector3d &dir)
{
  Quaterniond rot;
  if (dir.dot(DefaultDir) > -0.9998)
  {
    rot = Quaterniond(DefaultDir, dir);
  }
  else
  {
    rot.setAxisAngle(Vector3d::axisx, M_PI);
  }
  setRotation(rot);
  return *this;
}


inline Vector3d Cone::direction() const
{
  Quaterniond rot = rotation();
  return rot * DefaultDir;
}
}  // namespace tes

#endif  // _3ESCONE_H_
