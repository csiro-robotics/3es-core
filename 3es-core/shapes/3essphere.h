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
  /// @overload
  Sphere(uint32_t id = 0u, const V3Arg &centre = V3Arg(0, 0, 0), float radius = 1.0f);
  /// Create a sphere.
  /// @param id The shape ID, unique among @c Sphere objects, or zero for a transient shape.
  /// @param category The category grouping for the shape used for filtering.
  /// @param centre Defines the sphere centre coordinate.
  /// @param radius The sphere radius.
  Sphere(uint32_t id, uint16_t category, const V3Arg &centre = V3Arg(0, 0, 0), float radius = 1.0f);

  /// @overload
  Sphere(uint32_t id, const V3Arg &centre, const V3Arg &scale, const QuaternionArg &rot = QuaternionArg(0, 0, 0, 1));

  /// Create an ellipsoid. This constructor allows for scaling and rotating the sphere in order to create an ellipsoid.
  /// @param id The shape ID, unique among @c Sphere objects, or zero for a transient shape.
  /// @param category The category grouping for the shape used for filtering.
  /// @param centre Defines the sphere centre coordinate.
  /// @param scale Scaling values for the ellispoid along each axis.
  /// @param rot Orientation of the scale ellipsoid.
  Sphere(uint32_t id, uint16_t category, const V3Arg &centre, const V3Arg &scale,
         const QuaternionArg &rot = QuaternionArg(0, 0, 0, 1));

  inline const char *type() const override { return "sphere"; }

  /// Set the sphere radius. This sets the same scale for all dimensions.
  /// @param radius The sphere radius.
  /// @return @c *this
  Sphere &setRadius(float radius);
  /// Get the sphere radius.
  /// @return The sphere radius.
  float radius() const;

  /// Set the sphere centre coordinate.
  /// @param centre The new sphere centre.
  /// @return @c *this
  Sphere &setCentre(const V3Arg &centre);
  /// Get the sphere centre coordinate.
  /// @return The sphere centre.
  Vector3f centre() const;
};


inline Sphere::Sphere(uint32_t id, const V3Arg &centre, float radius)
  : Shape(SIdSphere, id)
{
  setPosition(centre);
  setScale(Vector3f(radius));
}


inline Sphere::Sphere(uint32_t id, uint16_t category, const V3Arg &centre, float radius)
  : Shape(SIdSphere, id, category)
{
  setPosition(centre);
  setScale(Vector3f(radius));
}


inline Sphere::Sphere(uint32_t id, const V3Arg &centre, const V3Arg &scale, const QuaternionArg &rot)
  : Shape(SIdSphere, id)
{
  setPosition(centre);
  setScale(scale);
  setRotation(rot);
}


inline Sphere::Sphere(uint32_t id, uint16_t category, const V3Arg &centre, const V3Arg &scale, const QuaternionArg &rot)
  : Shape(SIdSphere, id, category)
{
  setPosition(centre);
  setScale(scale);
  setRotation(rot);
}


inline Sphere &Sphere::setRadius(float radius)
{
  setScale(Vector3f(radius));
  return *this;
}


inline float Sphere::radius() const
{
  return scale().x;
}


inline Sphere &Sphere::setCentre(const V3Arg &centre)
{
  setPosition(centre);
  return *this;
}


inline Vector3f Sphere::centre() const
{
  return position();
}
}  // namespace tes

#endif  // _3ESSPHERE_H_
