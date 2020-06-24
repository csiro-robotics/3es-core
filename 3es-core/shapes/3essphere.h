//
// author: Kazys Stepanas
//
#ifndef _3ESSPHERE_H_
#define _3ESSPHERE_H_

#include "3es-core.h"
#include "3esshape.h"

namespace tes
{
/// Defines a sphere to display.
///
/// A sphere is defined by:
/// Component      | Description
/// -------------- | -----------------------------------------------------------------------------------------------
/// @c centre()    | The sphere centre. An alias for @p position().
/// @c radius()    | The sphere radius.
class _3es_coreAPI Sphere : public Shape
{
public:
  /// Create a sphere.
  /// @param id The shape id and category, with unique id among @c Sphere objects, or zero for a transient shape.
  /// @param transform The spherical transform for the sphere.
  Sphere(const ShapeId &id = ShapeId(), const Spherical &transform = Spherical());

  /// Create an ellipsoid. This constructor allows for scaling and rotating the sphere in order to create an ellipsoid.
  /// @param id The shape id and category, with unique id among @c Sphere objects, or zero for a transient shape.
  /// @param transform An arbitrary transform for the shape, supporting non-uniform scaling.
  Sphere(const ShapeId &id, const Transform &transform);

  inline const char *type() const override { return "sphere"; }

  /// Set the sphere radius. This sets the same scale for all dimensions.
  /// @param radius The sphere radius.
  /// @return @c *this
  Sphere &setRadius(double radius);
  /// Get the sphere radius.
  /// @return The sphere radius.
  double radius() const;

  /// Set the sphere centre coordinate.
  /// @param centre The new sphere centre.
  /// @return @c *this
  Sphere &setCentre(const Vector3d &centre);
  /// Get the sphere centre coordinate.
  /// @return The sphere centre.
  Vector3d centre() const;
};


inline Sphere::Sphere(const ShapeId &id, const Spherical &transform)
  : Shape(SIdSphere, id, transform)
{
}


inline Sphere::Sphere(const ShapeId &id, const Transform &transform)
  : Shape(SIdSphere, id, transform)
{
}


inline Sphere &Sphere::setRadius(double radius)
{
  setScale(Vector3d(radius));
  return *this;
}


inline double Sphere::radius() const
{
  return scale().x;
}


inline Sphere &Sphere::setCentre(const Vector3d &centre)
{
  setPosition(centre);
  return *this;
}


inline Vector3d Sphere::centre() const
{
  return position();
}
}  // namespace tes

#endif  // _3ESSPHERE_H_
