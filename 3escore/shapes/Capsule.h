//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SHAPES_CAPSULE_H
#define TES_CORE_SHAPES_CAPSULE_H

#include <3escore/CoreConfig.h>

#include "Shape.h"

namespace tes
{
/// Defines a capsule shape to display. A capsule is a cylinder with hemisphere end caps.
///
/// An arrow is defined by:
/// Component      | Description
/// -------------- | -----------------------------------------------------------------------------------------------
/// @c centre()    | The centre of the capsule. Alias for @p position().
/// @c axis()      | Defines the capsule primary axis. Affects @c rotation().
/// @c length()    | The length of the cylindrical part of the capsule. The end caps increase the extents further.
/// @c radius()    | Radius of the capsule cylinder and end caps.
class TES_CORE_API Capsule : public Shape
{
public:
  /// Construct a capsule object.
  /// @param id The shape id and catgory, unique among @c Capsule objects, or zero for a transient shape.
  /// @param transform The directional transformation for the shape.
  Capsule(const Id &id = Id(), const Directional &transform = Directional());
  /// Construct a capsule object.
  /// @param id The shape id and catgory, unique among @c Capsule objects, or zero for a transient shape.
  /// @param transform An arbitrary transform for the shape, supporting non-uniform scaling.
  Capsule(const Id &id, const Transform &transform);

  /// Copy constructor
  /// @param other Object to copy.
  Capsule(const Capsule &other);

  inline const char *type() const override { return "capsule"; }

  /// Set the capsule body radius.
  /// @param radius The radius to set.
  /// @return @c *this
  Capsule &setRadius(double radius);
  /// Get the capsule radius.
  /// @return The capsule radius.
  double radius() const;

  /// Set the capsule body length. The end caps extend beyond this by the radius at each end.
  /// @param length The body length to set.
  /// @return @c *this
  Capsule &setLength(double radius);
  /// Get the capsule body length.
  /// @param The body length.
  double length() const;

  /// Set the position fo the capsule centre.
  /// @param centre The centre coordinate.
  /// @return @c *this
  Capsule &setCentre(const Vector3d &centre);
  /// Get the capsule centre position.
  /// @return The centre coordinate.
  Vector3d centre() const;

  /// Set the capsule primary axis. Affects @p rotation().
  /// @param axis The new axis to set.
  /// @return @c *this
  Capsule &setAxis(const Vector3d &axis);
  /// Get the capsule primary axis.
  ///
  /// May not exactly match the axis given via @p setAxis() as the axis is defined by the quaternion @c rotation().
  /// @return The primary axis.
  Vector3d axis() const;
};


inline Capsule::Capsule(const Id &id, const Directional &transform)
  : Shape(SIdCapsule, id, transform)
{}


inline Capsule::Capsule(const Id &id, const Transform &transform)
  : Shape(SIdCapsule, id, transform)
{}


inline Capsule::Capsule(const Capsule &other)
  : Shape(other)
{}


inline Capsule &Capsule::setRadius(double radius)
{
  Vector3d s = Shape::scale();
  s.x = s.y = radius;
  setScale(s);
  return *this;
}


inline double Capsule::radius() const
{
  return scale().x;
}


inline Capsule &Capsule::setLength(double length)
{
  Vector3d s = Shape::scale();
  s.z = length;
  setScale(s);
  return *this;
}


inline double Capsule::length() const
{
  return scale().z;
}


inline Capsule &Capsule::setCentre(const Vector3d &centre)
{
  setPosition(centre);
  return *this;
}


inline Vector3d Capsule::centre() const
{
  return position();
}


inline Capsule &Capsule::setAxis(const Vector3d &axis)
{
  setRotation(Directional::directionToRotation(axis));
  return *this;
}


inline Vector3d Capsule::axis() const
{
  Quaterniond rot = rotation();
  return rot * Directional::DefaultDirection;
}
}  // namespace tes

#endif  // TES_CORE_SHAPES_CAPSULE_H
