//
// author: Kazys Stepanas
//
#ifndef _3ESPLANE_H_
#define _3ESPLANE_H_

#include "3es-core.h"
#include "3esshape.h"

#include <3esrotation.h>

namespace tes
{
/// Defines a rectangular planar section to display.
///
/// A plane is defined by:
/// Component      | Description
/// -------------- | -----------------------------------------------------------------------------------------------
/// @c position()  | Where to display a planar section.
/// @c normal()    | The plane normal.
/// @c scale()     | Defines the size of the plane rectangle (X,Y) and @c normalLength() (Z).
class _3es_coreAPI Plane : public Shape
{
public:
  /// Create a plane.
  /// @param id The shape id and category, unique among @c Plane objects, or zero for a transient shape.
  /// @param transform The directional transformation for the capsule.
  Plane(const IdCat &id = IdCat(), const Directional &transform = Directional());

  /// Create a plane.
  /// @param id The shape id and category, unique among @c Plane objects, or zero for a transient shape.
  /// @param transform The directional transformation for the capsule.
  Plane(const IdCat &id, const Transform &transform);

  inline const char *type() const override { return "plane"; }

  /// Set the plane normal. Affects @p rotation().
  /// @param axis The new axis to set.
  /// @return @c *this
  Plane &setNormal(const Vector3d &normal);
  /// Get the plane normal.
  ///
  /// May not exactly match the axis given via @p setNormal() as the axis is defined by the quaternion @c rotation().
  /// @return The plane normal.
  Vector3d normal() const;

  /// Set the plane "scale", which controls the render size.
  ///
  /// The X,Y axes control the size of the rectangle used to display the plane at @p position(). The Z is the same
  /// as the @c normalLength(). Note there is non guarantee on the orientation of the plane rectangle.
  ////
  /// @param scale The scaling values to set.
  /// @return @c *this
  Plane &setScale(double scale);
  /// Get the plane scaling values.
  /// @return The plane scaling values.
  double scale() const;

  /// Set the plane normal's display length. Alias for @c scale().z
  /// @param length Display length to set.
  /// @return @c *this
  Plane &setNormalLength(double length);

  /// Get the plane normal display length.
  /// @return The normal display length.
  double normalLength() const;
};


inline Plane::Plane(const IdCat &id, const Directional &transform)
  : Shape(SIdPlane, id, transform)
{
}


inline Plane::Plane(const IdCat &id, const Transform &transform)
  : Shape(SIdPlane, id, transform)
{
}


inline Plane &Plane::setNormal(const Vector3d &normal)
{
  Quaterniond rot(Directional::DefaultDirection, normal);
  setRotation(rot);
  return *this;
}


inline Vector3d Plane::normal() const
{
  Quaterniond rot = rotation();
  return rot * Directional::DefaultDirection;
}


inline Plane &Plane::setScale(double scale)
{
  Vector3d s = Shape::scale();
  s.x = s.z = scale;
  Shape::setScale(s);
  return *this;
}


inline double Plane::scale() const
{
  return Shape::scale().x;
}


inline Plane &Plane::setNormalLength(double len)
{
  Vector3d s = Shape::scale();
  s.y = len;
  Shape::setScale(s);
  return *this;
}


inline double Plane::normalLength() const
{
  return Shape::scale().y;
}
}  // namespace tes

#endif  // _3ESPLANE_H_
