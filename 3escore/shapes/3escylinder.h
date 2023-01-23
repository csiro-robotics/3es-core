//
// author: Kazys Stepanas
//
#ifndef _3ESCYLINDER_H_
#define _3ESCYLINDER_H_

#include <3escore/CoreConfig.h>

#include "3esshape.h"

namespace tes
{
/// Defines a cylinder shape to display.
///
/// An arrow is defined by:
/// Component      | Description
/// -------------- | -----------------------------------------------------------------------------------------------
/// @c centre()    | The centre of the cylinder. Alias for @p position().
/// @c axis()      | Defines the cylinder primary axis. Affects @c rotation().
/// @c length()    | The length of the cylinder body.
/// @c radius()    | Radius of the cylinder walls.
class _3es_coreAPI Cylinder : public Shape
{
public:
  /// Construct a cylinder object.
  /// @param id The shape id and category, unique among @c Capsule objects, or zero for a transient shape.
  /// @param transform The directional transformation for the capsule.
  Cylinder(const Id &id = Id(), const Directional &transform = Directional());

  /// Construct a cylinder object.
  /// @param id The shape id and category, unique among @c Capsule objects, or zero for a transient shape.
  /// @param transform An arbitrary transform for the shape, supporting non-uniform scaling.
  Cylinder(const Id &id, const Transform &transform);

  /// Copy constructor
  /// @param other Object to copy.
  Cylinder(const Cylinder &other);

  inline const char *type() const override { return "cylinder"; }

  /// Set the cylinder body radius.
  /// @param radius The radius to set.
  /// @return @c *this
  Cylinder &setRadius(double radius);
  /// Get the cylinder radius.
  /// @return The cylinder radius.
  double radius() const;

  /// Set the cylinder body length.
  /// @param length The body length to set.
  /// @return @c *this
  Cylinder &setLength(double radius);
  /// Get the cylinder body length.
  /// @param The body length.
  double length() const;

  /// Set the position fo the cylinder centre.
  /// @param centre The centre coordinate.
  /// @return @c *this
  Cylinder &setCentre(const Vector3d &centre);
  /// Get the cylinder centre position.
  /// @return The centre coordinate.
  Vector3d centre() const;

  /// Set the cylinder primary axis. Affects @p rotation().
  /// @param axis The new axis to set.
  /// @return @c *this
  Cylinder &setAxis(const Vector3d &axis);
  /// Get the cylinder primary axis.
  ///
  /// May not exactly match the axis given via @p setAxis() as the axis is defined by the quaternion @c rotation().
  /// @return The primary axis.
  Vector3d axis() const;
};


inline Cylinder::Cylinder(const Id &id, const Directional &transform)
  : Shape(SIdCylinder, id, transform)
{}


inline Cylinder::Cylinder(const Id &id, const Transform &transform)
  : Shape(SIdCylinder, id, transform)
{}


inline Cylinder::Cylinder(const Cylinder &other)
  : Shape(other)
{}


inline Cylinder &Cylinder::setRadius(double radius)
{
  Vector3d s = Shape::scale();
  s.x = s.y = radius;
  setScale(s);
  return *this;
}


inline double Cylinder::radius() const
{
  return scale().x;
}


inline Cylinder &Cylinder::setLength(double length)
{
  Vector3d s = Shape::scale();
  s.z = length;
  setScale(s);
  return *this;
}


inline double Cylinder::length() const
{
  return scale().z;
}


inline Cylinder &Cylinder::setCentre(const Vector3d &centre)
{
  setPosition(centre);
  return *this;
}


inline Vector3d Cylinder::centre() const
{
  return position();
}


inline Cylinder &Cylinder::setAxis(const Vector3d &axis)
{
  Quaterniond rot;
  if (axis.dot(Directional::DefaultDirection) > -0.9998f)
  {
    rot = Quaterniond(Directional::DefaultDirection, axis);
  }
  else
  {
    rot.setAxisAngle(Vector3d::axisx, M_PI);
  }
  setRotation(rot);
  return *this;
}


inline Vector3d Cylinder::axis() const
{
  Quaterniond rot = rotation();
  return rot * Directional::DefaultDirection;
}
}  // namespace tes

#endif  // _3ESCYLINDER_H_
