//
// author: Kazys Stepanas
//
#ifndef TES_CORE_TRI_GEOM_H
#define TES_CORE_TRI_GEOM_H

#include "Vector4.h"

#include <array>

/// Geometry functions based around triangles.
namespace tes::trigeom
{
//--------------------------------------------------------------------------
// Vector3f functions
//--------------------------------------------------------------------------
/// @overload
[[nodiscard]] Vector3f centre(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2);
/// Calculate the centre of the triangle formed by the given three points.
/// @param tri The triangle vertices.
/// @return The centre of the triangle formed by the three points.
[[nodiscard]] Vector3f centre(const std::array<Vector3f, 3> &tri);
/// @overload
[[nodiscard]] Vector3f centre(const Vector3f tri[3]);  // NOLINT(modernize-avoid-c-arrays)

/// @overload
[[nodiscard]] Vector3f normal(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2);
/// Calculate the triangle normal.
/// Results are undefined for degenerate triangles.
/// @param tri Triangle vertices.
/// @return The triangle normal.
[[nodiscard]] Vector3f normal(const std::array<Vector3f, 3> &tri);
/// @overload
[[nodiscard]] Vector3f normal(const Vector3f tri[3]);  // NOLINT(modernize-avoid-c-arrays)

/// @overload
[[nodiscard]] Vector4f plane(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2);
/// Calculate a plane representation for the given triangle.
///
/// This calculates the triangle plane in the resulting XYZ coordinates,
/// and a plane distance value in W.
///
/// Results are undefined for degenerate triangles.
/// @param tri Triangle vertices.
/// @return A representation of the triangle plane.
[[nodiscard]] Vector4f plane(const std::array<Vector3f, 3> &tri);
/// @overload
[[nodiscard]] Vector4f plane(const Vector3f tri[3]);  // NOLINT(modernize-avoid-c-arrays)

/// @overload
[[nodiscard]] bool isDegenerate(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2,
                                float epsilon = Vector3f::kEpsilon);
/// Check for a degenerate triangle This checks the magnitude of the cross product of the
/// edges to be greater than @p epsilon for non-degenerate triangles.
/// @param tri Triangle vertices.
/// @param epsilon Error tolerance.
/// @return True if the triangle is degenerate.
[[nodiscard]] bool isDegenerate(const std::array<Vector3f, 3> &tri,
                                float epsilon = Vector3f::kEpsilon);
/// @overload
[[nodiscard]] bool isDegenerate(const Vector3f tri[3],  // NOLINT(modernize-avoid-c-arrays)
                                float epsilon = Vector3f::kEpsilon);

/// @overload
[[nodiscard]] bool isPointInside(const Vector3f &point, const Vector3f &v0, const Vector3f &v1,
                                 const Vector3f &v2);
/// Check if a point lies inside a triangle, assuming they are on the same plane.
/// Results are undefined for degenerate triangles.
/// @param point The point to test. Assumed to be on the triangle plane.
/// @param tri The triangle vertices.
/// @return True if @p point lies inside the triangle.
[[nodiscard]] bool isPointInside(const Vector3f &point,
                                 const Vector3f tri[3]);  // NOLINT(modernize-avoid-c-arrays)

/// @overload
[[nodiscard]] Vector3f nearestPoint(const Vector3f &point, const Vector3f &v0, const Vector3f &v1,
                                    const Vector3f &v2);
/// Find a point on or within @p tri closest to @p point.
/// The @p point need not be on the same plane as it is first projected onto that plane..
/// Results are undefined for degenerate triangles.
/// @param point The point of interest.
/// @param tri The triangle vertices.
/// @return The point on or within @p tri closest to @p point.
[[nodiscard]] Vector3f nearestPoint(const Vector3f &point, const std::array<Vector3f, 3> &tri);
/// @overload
[[nodiscard]] Vector3f nearestPoint(const Vector3f &point,
                                    const Vector3f tri[3]);  // NOLINT(modernize-avoid-c-arrays)

/// Performs a ray/triangle intersection test.
///
/// When an intersection occurs, the @p hit_time is set to represent the 'time' of
/// intersection along the ray @p dir. This is always positive and intersections backwards
/// along the ray are ignored. The location of the intersection can be calculated as:
/// @code{.unparsed}
///   Vector3f p = origin + hit_time * dir;
/// @endcode
///
/// So long as @p dir is normalised, the @p hit_time represents the distance long the ray
/// at which intersection occurs.
///
/// @param[out] hit_time Represents the intersection 'time'. Must not be null.
/// @param v0 A triangle vertex.
/// @param v1 A triangle vertex.
/// @param v2 A triangle vertex.
/// @param origin The ray origin.
/// @param dir The ray direction. Need not be normalised, but is best to be.
/// @param epsilon Intersection error tolerance.
bool intersectRay(float *hit_time, const Vector3f &v0, const Vector3f &v1, const Vector3f &v2,
                  const Vector3f &origin, const Vector3f &dir, float epsilon = Vector3f::kEpsilon);

/// Triangle intersection test.
///
/// As a special case, the triangles are not considered intersecting when they exactly
/// touch (equal vertices) and epsilon is zero.
[[nodiscard]] bool intersectTriangles(const Vector3f &a0, const Vector3f &a1, const Vector3f &a2,
                                      const Vector3f &b0, const Vector3f &b1, const Vector3f &b2,
                                      float epsilon = Vector3f::kEpsilon);

/// Intersect a triangle with an axis aligned box.
/// @param tri The triangle vertices.
/// @param aabb The axis aligned box. Index zero is the minimum extents, index one the maximum.
/// @return True if the triangle overlaps, lies inside or contains the box.
[[nodiscard]] bool intersectAABB(const std::array<Vector3f, 3> &tri,
                                 const std::array<Vector3f, 2> &aabb);
/// @overload
[[nodiscard]] bool intersectAABB(const Vector3f tri[3],    // NOLINT(modernize-avoid-c-arrays)
                                 const Vector3f aabb[2]);  // NOLINT(modernize-avoid-c-arrays)

//--------------------------------------------------------------------------
// Vector3d functions
//--------------------------------------------------------------------------
/// @overload
[[nodiscard]] Vector3d centre(const Vector3d &v0, const Vector3d &v1, const Vector3d &v2);
/// @overload
[[nodiscard]] Vector3d centre(const std::array<Vector3d, 3> &tri);
/// @overload
[[nodiscard]] Vector3d centre(const Vector3d tri[3]);  // NOLINT(modernize-avoid-c-arrays)

/// @overload
[[nodiscard]] Vector3d normal(const Vector3d &v0, const Vector3d &v1, const Vector3d &v2);
/// @overload
[[nodiscard]] Vector3d normal(std::array<Vector3d, 3> &tri);
/// @overload
[[nodiscard]] Vector3d normal(const Vector3d tri[3]);  // NOLINT(modernize-avoid-c-arrays)

/// @overload
[[nodiscard]] Vector4d plane(const Vector3d &v0, const Vector3d &v1, const Vector3d &v2);
/// @overload
[[nodiscard]] Vector4d plane(std::array<Vector3d, 3> &tri);
/// @overload
[[nodiscard]] Vector4d plane(const Vector3d tri[3]);  // NOLINT(modernize-avoid-c-arrays)

/// @overload
[[nodiscard]] bool isDegenerate(const Vector3d &v0, const Vector3d &v1, const Vector3d &v2,
                                double epsilon = Vector3d::kEpsilon);
/// @overload
[[nodiscard]] bool isDegenerate(const Vector3d tri[3],  // NOLINT(modernize-avoid-c-arrays)
                                double epsilon = Vector3d::kEpsilon);

/// @overload
[[nodiscard]] bool isPointInside(const Vector3d &point, const Vector3d &v0, const Vector3d &v1,
                                 const Vector3d &v2);
/// @overload
[[nodiscard]] bool isPointInside(const Vector3d &point, std::array<Vector3d, 3> &tri);
/// @overload
[[nodiscard]] bool isPointInside(const Vector3d &point,
                                 const Vector3d tri[3]);  // NOLINT(modernize-avoid-c-arrays)

/// @overload
[[nodiscard]] Vector3d nearestPoint(const Vector3d &point, const Vector3d &v0, const Vector3d &v1,
                                    const Vector3d &v2);
/// @overload
[[nodiscard]] Vector3d nearestPoint(const Vector3d &point, std::array<Vector3d, 3> &tri);
/// @overload
[[nodiscard]] Vector3d nearestPoint(const Vector3d &point,
                                    const Vector3d tri[3]);  // NOLINT(modernize-avoid-c-arrays)

/// @overload
bool intersectRay(double *hit_time, const Vector3d &v0, const Vector3d &v1, const Vector3d &v2,
                  const Vector3d &origin, const Vector3d &dir, double epsilon = Vector3d::kEpsilon);
/// @overload
[[nodiscard]] bool intersectTriangles(const Vector3d &a0, const Vector3d &a1, const Vector3d &a2,
                                      const Vector3d &b0, const Vector3d &b1, const Vector3d &b2,
                                      double epsilon = Vector3d::kEpsilon);

/// @overload
[[nodiscard]] bool intersectAABB(const std::array<Vector3d, 3> &tri,
                                 const std::array<Vector3d, 2> &aabb);
/// @overload
[[nodiscard]] bool intersectAABB(const Vector3d tri[3],    // NOLINT(modernize-avoid-c-arrays)
                                 const Vector3d aabb[2]);  // NOLINT(modernize-avoid-c-arrays)
}  // namespace tes::trigeom

#include "TriGeom.inl"

#endif  // TES_CORE_TRI_GEOM_H
