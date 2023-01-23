//
// author: Kazys Stepanas
//
#ifndef _3ESCONE_H_
#define _3ESCONE_H_

#include <3escore/CoreConfig.h>

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
  /// Construct a cone object.
  /// @param id The shape id and category, unique among @c Cone objects, or zero for a transient shape.
  /// @param transform The directional transformation for the shape. The radius specified is applied at the base.
  Cone(const Id &id = Id(), const Directional &transform = Directional());

  /// Construct a cone object.
  /// @param id The shape id and category, unique among @c Cone objects, or zero for a transient shape.
  /// @param transform An arbitrary transform for the shape, supporting non-uniform scaling.
  Cone(const Id &id, const Transform &transform);

  /// Copy constructor
  /// @param other Object to copy.
  Cone(const Cone &other);

  inline const char *type() const override { return "cone"; }

  /// Sets the cone radius at the base.
  /// @param radius The base radius.
  /// @return @c *this
  Cone &setRadius(double radius);
  /// Get the cone angle at the apex (radians).
  /// @return The cone angle (radians).
  double radius() const;

  /// Set the cone length, apex to base.
  /// @param length The length to set.
  /// @return @c *this
  Cone &setLength(double length);
  /// Get the cone length, apex to base.
  /// @return The cone length.
  double length() const;

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


inline Cone::Cone(const Id &id, const Directional &transform)
  : Shape(SIdCone, id, transform)
{}


inline Cone::Cone(const Id &id, const Transform &transform)
  : Shape(SIdCone, id, transform)
{}


inline Cone::Cone(const Cone &other)
  : Shape(other)
{}


inline Cone &Cone::setRadius(double radius)
{
  Vector3d s = scale();
  s.x = s.y = radius;
  setScale(s);
  return *this;
}


inline double Cone::radius() const
{
  return scale().x;
}


inline Cone &Cone::setLength(double length)
{
  Vector3d s = scale();
  s.z = length;
  setScale(s);
  return *this;
}


inline double Cone::length() const
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
  if (dir.dot(Directional::DefaultDirection) > -0.9998)
  {
    rot = Quaterniond(Directional::DefaultDirection, dir);
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
  return rot * Directional::DefaultDirection;
}
}  // namespace tes

#endif  // _3ESCONE_H_
