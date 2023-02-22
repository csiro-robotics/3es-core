//
// author: Kazys Stepanas
//
#include "Cylinder.h"

#include <algorithm>
#include <array>

#include <3escore/CoreUtil.h>

namespace tes::cylinder
{
// Disabled linter warnings on doing unsigned maths, then passing the results as arguments to
// std::vector::operator[] and std::vector::resize(). The choice to use unsigned maths is
// deliberate.
// clang-format off
// NOLINTBEGIN(bugprone-misplaced-widening-cast, bugprone-implicit-widening-of-multiplication-result)
// clang-format on
namespace
{
void makeCylinder(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
                  std::vector<Vector3f> *normals, const Vector3f &axis, float height, float radius,
                  unsigned facets, bool open)
{
  facets = std::max(facets, 3u);
  const float segment_angle = static_cast<float>(2.0 * M_PI) / static_cast<float>(facets);

  // Build two radial vectors out from the cylinder axis perpendicular to each other (like a
  // cylinder).
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

  vertices.resize(facets * ((open) ? 2 : 4));
  if (normals)
  {
    normals->resize(vertices.size());
  }

  Vector3f ring_centre;
  Vector3f radial;
  Vector3f vertex;
  ring_centre = axis * 0.5f * height;
  for (unsigned f = 0; f < facets; ++f)
  {
    const float facet_angle = static_cast<float>(f) * segment_angle;
    radial = radius * (std::cos(facet_angle) * radials[0] + std::sin(facet_angle) * radials[1]);
    vertex = ring_centre + radial;
    // We add a third and fourth set of vertices for the end caps.
    vertices[f] = vertex;
    vertices[f + facets] = vertex - axis * height;
    if (!open)
    {
      vertices[f + 2 * facets] = vertices[f];
      vertices[f + 3 * facets] = vertices[f + facets];
    }

    if (normals)
    {
      (*normals)[f] = (*normals)[f + facets] = radial;

      // End cap normals.
      if (!open)
      {
        (*normals)[f + 2 * facets] = axis;
        (*normals)[f + 3 * facets] = -axis;
      }
    }
  }

  // Triangulate between the end rings.
  const unsigned top_ring_start_index = 0;
  const unsigned bottom_ring_start_index = facets;
  for (unsigned f = 0; f < facets; ++f)
  {
    indices.push_back(bottom_ring_start_index + f);
    indices.push_back(bottom_ring_start_index + (f + 1) % facets);
    indices.push_back(top_ring_start_index + (f + 1) % facets);

    indices.push_back(bottom_ring_start_index + f);
    indices.push_back(top_ring_start_index + (f + 1) % facets);
    indices.push_back(top_ring_start_index + f);
  }

  // Triangulate the end caps.
  if (!open)
  {
    const unsigned top_cap_start_index = 2 * facets;
    const unsigned bottom_cap_start_index = 3 * facets;
    for (unsigned f = 1; f < facets - 1; ++f)
    {
      indices.push_back(top_cap_start_index + 0);
      indices.push_back(top_cap_start_index + f);
      indices.push_back(top_cap_start_index + f + 1);
    }

    for (unsigned f = 1; f < facets - 1; ++f)
    {
      indices.push_back(bottom_cap_start_index + 0);
      indices.push_back(bottom_cap_start_index + f + 1);
      indices.push_back(bottom_cap_start_index + f);
    }
  }
}
}  // namespace


void solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, const Vector3f &axis,
           float height, float radius, unsigned facets, bool open)
{
  return makeCylinder(vertices, indices, nullptr, axis, height, radius, facets, open);
}


void solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
           std::vector<Vector3f> &normals, const Vector3f &axis, float height, float radius,
           unsigned facets, bool open)
{
  return makeCylinder(vertices, indices, &normals, axis, height, radius, facets, open);
}

void wireframe(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
               const Vector3f &axis, float height, float radius, unsigned segments)
{
  // Build a rings.
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

  // Build a circle around the axis.
  std::array<unsigned, 2> ring_start;
  ring_start[0] = int_cast<unsigned>(vertices.size());
  for (unsigned i = 0; i < segments; ++i)
  {
    const float circle_angle =
      static_cast<float>(i) * 2.0f * static_cast<float>(M_PI) / static_cast<float>(segments);
    vertices.emplace_back(radius * std::cos(circle_angle) * radials[0] +  //
                          radius * std::sin(circle_angle) * radials[1]);
  }

  // Duplicate the ring.
  ring_start[1] = int_cast<unsigned>(vertices.size());
  for (unsigned i = 0; i < segments; ++i)
  {
    vertices.emplace_back(vertices[ring_start[0] + i]);
    // Also fix up the ring vertices, offsetting them with the axis.
    vertices[ring_start[0] + i] += axis * 0.5f * height;
    vertices.back() -= axis * 0.5f * height;
  }

  // Build the ring vertices.
  for (int r = 0; r < 2; ++r)
  {
    for (unsigned i = 0; i < segments; ++i)
    {
      indices.emplace_back(ring_start[r] + i);
      indices.emplace_back(ring_start[r] + (i + 1) % segments);
    }
  }

  // Connec the rings.
  for (unsigned i = 0; i < segments; ++i)
  {
    indices.emplace_back(ring_start[0] + i);
    indices.emplace_back(ring_start[1] + i);
  }
}
// clang-format off
// NOLINTEND(bugprone-misplaced-widening-cast, bugprone-implicit-widening-of-multiplication-result)
// clang-format on
}  // namespace tes::cylinder
