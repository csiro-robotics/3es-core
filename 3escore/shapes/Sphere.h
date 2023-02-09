//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SHAPES_SPHERE_H
#define TES_CORE_SHAPES_SPHERE_H

#include <3escore/CoreConfig.h>

#include "Shape.h"

namespace tes
{
/// Defines a sphere to display.
///
/// A sphere is defined by:
/// Component      | Description
/// -------------- |
/// -----------------------------------------------------------------------------------------------
/// @c centre()    | The sphere centre. An alias for @p position().
/// @c radius()    | The sphere radius.
class TES_CORE_API Sphere : public Shape
{
public:
  /// Create a sphere.
  /// @param id The shape id and category, with unique id among @c Sphere objects, or zero for a
  /// transient shape.
  /// @param transform The spherical transform for the sphere.
  Sphere(const Id &id = Id(), const Spherical &transform = Spherical());

  /// Create an ellipsoid. This constructor allows for scaling and rotating the sphere in order to
  /// create an ellipsoid.
  /// @param id The shape id and category, with unique id among @c Sphere objects, or zero for a
  /// transient shape.
  /// @param transform An arbitrary transform for the shape, supporting non-uniform scaling.
  Sphere(const Id &id, const Transform &transform);

  /// Copy constructor.
  /// Shape to copy.
  Sphere(const Sphere &other);

  [[nodiscard]] const char *type() const override { return "sphere"; }

  /// Set the sphere radius. This sets the same scale for all dimensions.
  /// @param radius The sphere radius.
  /// @return @c *this
  Sphere &setRadius(double radius);
  /// Get the sphere radius.
  /// @return The sphere radius.
  [[nodiscard]] double radius() const;

  /// Set the sphere centre coordinate.
  /// @param centre The new sphere centre.
  /// @return @c *this
  Sphere &setCentre(const Vector3d &centre);
  /// Get the sphere centre coordinate.
  /// @return The sphere centre.
  [[nodiscard]] Vector3d centre() const;
};


inline Sphere::Sphere(const Id &id, const Spherical &transform)
  : Shape(SIdSphere, id, transform)
{}


inline Sphere::Sphere(const Id &id, const Transform &transform)
  : Shape(SIdSphere, id, transform)
{}

inline Sphere::Sphere(const Sphere &other) = default;


inline Sphere &Sphere::setRadius(double radius)
{
  setScale(Vector3d(radius));
  return *this;
}


inline double Sphere::radius() const
{
  return scale().x();
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

#endif  // TES_CORE_SHAPES_SPHERE_H
