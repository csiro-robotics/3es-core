//
// author: Kazys Stepanas
//
#include "Cone.h"

#include <3escore/Debug.h>

#include <algorithm>
#include <array>

namespace tes::cone
{
// Disabled linter warnings on doing unsigned maths, then passing the results as arguments to
// std::vector::operator[] and std::vector::resize(). The choice to use unsigned maths is
// deliberate.
// clang-format off
// NOLINTBEGIN(bugprone-misplaced-widening-cast, bugprone-implicit-widening-of-multiplication-result)
// clang-format on
namespace
{
void makeCone(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
              std::vector<Vector3f> *normals, const Vector3f &apex, const Vector3f &axis,
              float height, float angle, unsigned facets)
{
  facets = std::max(facets, 3u);
  const float base_radius = height * std::atan(angle);
  float const segment_angle = static_cast<float>(2.0 * M_PI) / static_cast<float>(facets);

  // Build two radial vectors out from the cone axis perpendicular to each other (like a cylinder).
  // We'll use these to build the base ring.
  std::array<Vector3f, 2> radials;
  const float near_aligned_dot = std::cos(85.0f / 180.0f * static_cast<float>(M_PI));
  if (axis.dot(Vector3f::AxisY) < near_aligned_dot)
  {
    radials[0] = Vector3f::AxisY.cross(axis);
  }
  else
  {
    radials[0] = Vector3f::AxisX.cross(axis);
  }
  radials[0].normalise();
  radials[1] = axis.cross(radials[0]);

  // Three sets of vertices: base walls, apex walls, base closer.
  vertices.resize(facets * 3);
  if (normals)
  {
    normals->resize(vertices.size());
  }

  Vector3f ring_centre;
  Vector3f radial;
  Vector3f vertex;
  Vector3f tangent;
  Vector3f normal;
  Vector3f to_apex;
  ring_centre = apex - axis * height;
  for (unsigned f = 0; f < facets; ++f)
  {
    const float facet_angle = static_cast<float>(f) * segment_angle;
    radial =
      base_radius * (std::cos(facet_angle) * radials[0] + std::sin(facet_angle) * radials[1]);
    vertex = ring_centre + radial;
    vertices[f] = vertex;
    // And the apex vertex. One for each facet for distinct normals.
    vertices[f + facets] = apex;
    vertices[f + 2 * facets] = vertex;

    if (normals)
    {
      to_apex = apex - vertex;
      tangent = axis.cross(to_apex);
      normal = to_apex.cross(tangent).normalised();
      (*normals)[f] = (*normals)[f + facets] = normal;
      // Base normals.
      (*normals)[f + 2 * facets] = -axis;
    }
  }

  // Now triangulate between the rings.
  const unsigned wall_ring_start_index = 0;
  const unsigned apex_ring_start_index = facets;
  for (unsigned f = 0; f < facets; ++f)
  {
    indices.push_back(wall_ring_start_index + f);
    indices.push_back(wall_ring_start_index + (f + 1) % facets);
    indices.push_back(apex_ring_start_index + (f + 1) % facets);

    indices.push_back(wall_ring_start_index + f);
    indices.push_back(apex_ring_start_index + (f + 1) % facets);
    indices.push_back(apex_ring_start_index + f);
  }

  // Tesselate the base.
  const unsigned base_ring_start_index = 2 * facets;
  for (unsigned f = 1; f < facets - 1; ++f)
  {
    indices.push_back(base_ring_start_index + 0);
    indices.push_back(base_ring_start_index + f + 1);
    indices.push_back(base_ring_start_index + f);
  }
}
}  // namespace


void solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
           std::vector<Vector3f> &normals, const Vector3f &apex, const Vector3f &axis, float height,
           float angle, unsigned facets)
{
  return makeCone(vertices, indices, &normals, apex, axis, height, angle, facets);
}


void solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, const Vector3f &apex,
           const Vector3f &axis, float height, float angle, unsigned facets)
{
  return makeCone(vertices, indices, nullptr, apex, axis, height, angle, facets);
}


void wireframe(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
               const Vector3f &apex, const Vector3f &axis, float height, float angle,
               unsigned segments)
{
  TES_ASSERT(segments > 0);
  // Build a ring.
  // Build the lines for the cylinder.
  std::array<Vector3f, 2> radials;

  // Calculate base vectors perpendicular to the axis.
  const float epsilon = 1e-6f;
  if (axis.cross(Vector3f(1, 0, 0)).magnitudeSquared() > epsilon)
  {
    radials[0] = axis.cross(Vector3f(1, 0, 0)).normalised();
  }
  else
  {
    radials[0] = axis.cross(Vector3f(0, 1, 0)).normalised();
  }
  radials[1] = axis.cross(radials[0]).normalised();

  // Calculate the base width
  //      b
  //    ______
  //   |     /
  //   |    /
  // h |   /
  //   |  /
  //   |a/
  //   |/
  //
  // b = h * tan(a)
  const float base_radius = height * std::tan(angle);

  // Add the apex.
  const unsigned apex_index = 0;
  vertices.emplace_back(apex);

  // Build a circle around the axis.
  for (unsigned i = 0; i < segments; ++i)
  {
    const float circle_angle =
      static_cast<float>(i) * 2.0f * static_cast<float>(M_PI) / static_cast<float>(segments);
    vertices.emplace_back(base_radius * std::cos(circle_angle) * radials[0] +
                          base_radius * std::sin(circle_angle) * radials[1] + apex - axis * height);
  }

  // Connect the base ring.
  for (unsigned i = 0; i <= segments; ++i)
  {
    indices.emplace_back(apex_index + i);
    indices.emplace_back(apex_index + (i + 1) % segments);
  }

  // Connect the apex to the ring.
  for (unsigned i = 0; i < segments; ++i)
  {
    indices.emplace_back(apex_index);
    indices.emplace_back(apex_index + i);
  }
}
// clang-format off
// NOLINTEND(bugprone-misplaced-widening-cast, bugprone-implicit-widening-of-multiplication-result)
// clang-format on
}  // namespace tes::cone
