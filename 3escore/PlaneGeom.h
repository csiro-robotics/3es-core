//
// author: Kazys Stepanas
//
#ifndef TES_CORE_PLANE_GEOM_H
#define TES_CORE_PLANE_GEOM_H

#include "Vector4.h"

/// Plane geometry functions.
///
/// A plane is defined by a @c Vector4 where the xyz components are the normal and the w component
/// is the plane distance.
namespace tes::planegeom
{
/// Point classification results.
enum PlaneClassification
{
  PCBehind = -1,  ///< Behind the plane.
  PCOn = 0,       ///< On or part of the plane.
  PCInFront = 1   ///< In front of the plane.
};

/// Create a plane from a normal and distance (D) value.
/// @param normal The plane normal. Must be normalised.
/// @param distance the plane D component.
/// @return The plane equation.
template <typename T>
[[nodiscard]] inline Vector4<T> create(const Vector3<T> &normal, T distance)
{
  return Vector4f(normal, distance);
}

/// Create a plane from a normal and a point on the plane.
/// @param normal The plane normal. Must be normalised.
/// @param point_on_plane An arbitrary point on the plane.
/// @return The plane equation.
template <typename T>
[[nodiscard]] Vector4<T> fromNormalAndPoint(const Vector3<T> &normal,
                                            const Vector3<T> &point_on_plane)
{
  const T distance = -normal.dot(point_on_plane);
  return Vector4<T>(normal, distance);
}

/// Calculate the signed distance between the @p plane and @p point.
/// Positive is in front, negative behind and zero on.
/// @param plane The plane equation.
/// @param point The point of interest.
/// @return The shorted distance from @p point to the plane (signed).
template <typename T>
[[nodiscard]] inline T signedDistanceToPoint(const Vector4<T> &plane, const Vector3<T> &point)
{
  return plane.xyz().dot(point) + plane.w();
}


/// Project a @p point onto a @p plane.
/// @param plane The plane equation.
/// @param point The point of interest.
/// @return The closest point on the plane to @p point.
template <typename T>
[[nodiscard]] inline Vector3<T> projectPoint(const Vector4<T> &plane, const Vector3<T> &point)
{
  const T signedDistance = signedDistanceToPoint(plane, point);
  return point - plane.xyz() * signedDistance;
}


/// Classify a point with respect to a plane (see @c PlaneClassification).
/// @param plane The plane equation.
/// @param point The point of interest.
/// @param epsilon Epsilon value used as a tolerance for @c PCOn results.
/// @return The point's @c PlaneClassification.
template <typename T>
[[nodiscard]] inline int classifyPoint(const Vector4<T> &plane, const Vector3<T> &point,
                                       T epsilon = 0)
{
  const T signedDistance = signedDistanceToPoint(plane, point);
  if (signedDistance < -epsilon)
  {
    return PCBehind;
  }

  if (signedDistance > epsilon)
  {
    return PCInFront;
  }

  return PCOn;
}
}  // namespace tes::planegeom

#endif  // TES_CORE_PLANE_GEOM_H
