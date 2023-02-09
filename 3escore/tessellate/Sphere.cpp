//
// author: Kazys Stepanas
//
#include "Sphere.h"

#include <3escore/CoreUtil.h>
#include <3escore/Vector3.h>

#include <array>
#include <cmath>
#include <unordered_map>

namespace tes::sphere
{
// Disabled linter warnings on doing unsigned maths, then passing the results as arguments to
// std::vector::operator[] and std::vector::resize(). The choice to use unsigned maths is
// deliberate.
// clang-format off
// NOLINTBEGIN(bugprone-misplaced-widening-cast, bugprone-implicit-widening-of-multiplication-result)
// clang-format on
namespace
{
/// Add a vertex to @p points, reusing an existing vertex is a matching one is found.
///
/// This first searches for a matching vertex in @p point and returns its index if found.
/// Otherwise a new vertex is added.
///
/// @param vertex The vertex to add.
/// @param vertices The vertex data to add to.
/// @return The index which can be used to refer to the target vertex.
unsigned insertVertex(const Vector3f &vertex, std::vector<Vector3f> &vertices,
                      SphereVertexMap &vertex_map)
{
  const auto find_result = vertex_map.find(vertex);
  if (find_result == vertex_map.end())
  {
    // Add new vertex.
    auto idx = int_cast<unsigned>(vertices.size());
    vertices.push_back(vertex);
    vertex_map.insert(std::make_pair(vertex, idx));
    return idx;
  }

  return find_result->second;
}
}  // namespace


void initialise(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
                SphereVertexMap *vertex_map)
{
  // We start with two hexagonal rings to approximate the sphere.
  // All subdivision occurs on a unit radius sphere, at the origin. We translate and
  // scale the vertices at the end.
  vertices.clear();
  indices.clear();

  static const float ring_control_angle = 25.0f / 180.0f * static_cast<float>(M_PI);
  static const float ring_height = std::sin(ring_control_angle);
  static const float ring_radius = std::cos(ring_control_angle);
  static const float hex_angle = 2.0f * static_cast<float>(M_PI) / 6.0f;
  static const float ring_to_offset_angle = 0.5f * hex_angle;
  static const std::array<Vector3f, 14> initial_vertices = {
    Vector3f(0, 0, 1),

    // Upper hexagon.
    Vector3f(ring_radius, 0, ring_height),
    Vector3f(ring_radius * std::cos(hex_angle), ring_radius * std::sin(hex_angle), ring_height),
    Vector3f(ring_radius * std::cos(2 * hex_angle), ring_radius * std::sin(2 * hex_angle),
             ring_height),
    Vector3f(ring_radius * std::cos(3 * hex_angle), ring_radius * std::sin(3 * hex_angle),
             ring_height),
    Vector3f(ring_radius * std::cos(4 * hex_angle), ring_radius * std::sin(4 * hex_angle),
             ring_height),
    Vector3f(ring_radius * std::cos(5 * hex_angle), ring_radius * std::sin(5 * hex_angle),
             ring_height),

    // Lower hexagon.
    Vector3f(ring_radius * std::cos(ring_to_offset_angle),
             ring_radius * std::sin(ring_to_offset_angle), -ring_height),
    Vector3f(ring_radius * std::cos(ring_to_offset_angle + hex_angle),
             ring_radius * std::sin(ring_to_offset_angle + hex_angle), -ring_height),
    Vector3f(ring_radius * std::cos(ring_to_offset_angle + 2 * hex_angle),
             ring_radius * std::sin(ring_to_offset_angle + 2 * hex_angle), -ring_height),
    Vector3f(ring_radius * std::cos(ring_to_offset_angle + 3 * hex_angle),
             ring_radius * std::sin(ring_to_offset_angle + 3 * hex_angle), -ring_height),
    Vector3f(ring_radius * std::cos(ring_to_offset_angle + 4 * hex_angle),
             ring_radius * std::sin(ring_to_offset_angle + 4 * hex_angle), -ring_height),
    Vector3f(ring_radius * std::cos(ring_to_offset_angle + 5 * hex_angle),
             ring_radius * std::sin(ring_to_offset_angle + 5 * hex_angle), -ring_height),

    Vector3f(0, 0, -1),
  };
  const unsigned initial_vertex_count = sizeof(initial_vertices) / sizeof(initial_vertices[0]);

  static const std::array<unsigned, 72> initial_indices = {
    0,  1,  2, 0,  2,  3, 0, 3,  4, 0, 4,  5, 0, 5,  6,  0,  6,  1,  1,  7,  2,  2,  8,  3,
    3,  9,  4, 4,  10, 5, 5, 11, 6, 6, 12, 1, 7, 8,  2,  8,  9,  3,  9,  10, 4,  10, 11, 5,
    11, 12, 6, 12, 7,  1, 7, 13, 8, 8, 13, 9, 9, 13, 10, 10, 13, 11, 11, 13, 12, 12, 13, 7
  };
  const auto initial_index_count = static_cast<unsigned>(initial_indices.size());

  for (unsigned i = 0; i < initial_vertex_count; ++i)
  {
    vertices.push_back(initial_vertices[i]);
    if (vertex_map)
    {
      vertex_map->insert(std::make_pair(initial_vertices[i], i));
    }
  }

  for (unsigned i = 0; i < initial_index_count; i += 3)
  {
    indices.push_back(initial_indices[i + 0]);
    indices.push_back(initial_indices[i + 1]);
    indices.push_back(initial_indices[i + 2]);
  }
}


void subdivide(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
               SphereVertexMap &vertex_map)
{
  const auto triangle_count = int_cast<unsigned>(indices.size() / 3);
  std::array<unsigned, 3> triangle;
  std::array<unsigned, 3> abc;
  std::array<unsigned, 3> def;
  std::array<Vector3f, 3> verts;
  std::array<Vector3f, 3> new_vertices;

  for (unsigned i = 0; i < triangle_count; ++i)
  {
    triangle[0] = abc[0] = indices[i * 3 + 0];
    triangle[1] = abc[1] = indices[i * 3 + 1];
    triangle[2] = abc[2] = indices[i * 3 + 2];

    // Fetch the vertices.
    verts[0] = vertices[triangle[0]];
    verts[1] = vertices[triangle[1]];
    verts[2] = vertices[triangle[2]];

    // Calculate the new vertex at the centre of the existing triangle.
    new_vertices[0] = (0.5f * (verts[0] + verts[1])).normalised();
    new_vertices[1] = (0.5f * (verts[1] + verts[2])).normalised();
    new_vertices[2] = (0.5f * (verts[2] + verts[0])).normalised();

    // Create new triangles.
    // Given triangle ABC, and adding vertices DEF such that:
    //  D = AB/2  E = BC/2  F = CA/2
    // We have four new triangles:
    //  ADF, BED, CFE, DEF
    // ABC are in order in 'abc', while DEF will be in 'def'.
    // FIXME: find existing point to use.
    def[0] = insertVertex(new_vertices[0], vertices, vertex_map);
    def[1] = insertVertex(new_vertices[1], vertices, vertex_map);
    def[2] = insertVertex(new_vertices[2], vertices, vertex_map);

    // Replace the original triangle ABC with DEF
    indices[i * 3 + 0] = def[0];
    indices[i * 3 + 1] = def[1];
    indices[i * 3 + 2] = def[2];

    // Add triangles ADF, BED, CFE
    indices.push_back(abc[0]);
    indices.push_back(def[0]);
    indices.push_back(def[2]);

    indices.push_back(abc[1]);
    indices.push_back(def[1]);
    indices.push_back(def[0]);

    indices.push_back(abc[2]);
    indices.push_back(def[2]);
    indices.push_back(def[1]);
  }
}


// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void solidLatLong(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
                  std::vector<Vector3f> &normals, float radius, const Vector3f &origin,
                  unsigned hemisphere_ring_count, unsigned segments, const Vector3f &axis_in,
                  bool hemisphere_only)
{
  hemisphere_ring_count = std::max(hemisphere_ring_count, 1u);
  segments = std::max(segments, 3u);
  Vector3f axis = axis_in;
  const float epsilon = 1e-3f;
  if (std::abs(axis.magnitudeSquared() - 1.0f) > epsilon)
  {
    axis = Vector3f(0, 0, 1);
  }

  std::array<Vector3f, 2> radials;
  Vector3f v;
  const float segment_angle = 2.0f * static_cast<float>(M_PI) / static_cast<float>(segments);
  const float ring_step_angle =
    0.5f * static_cast<float>(M_PI) / static_cast<float>(hemisphere_ring_count);

  if (axis.dot(Vector3f(1, 0, 0)) < epsilon)
  {
    radials[0] = Vector3f(1, 0, 0);
  }
  else
  {
    radials[0] = Vector3f(0, 1, 0);
  }

  radials[1] = axis.cross(radials[0]).normalised();
  radials[0] = radials[1].cross(axis).normalised();

  // Two triangles (six indices) per segment per ring.
  // Last ring has only one trinagle per segment, for which we make the adjustment.
  // Will be doubled for full sphere.
  unsigned index_cap = hemisphere_ring_count * segments * 2 * 3 - 3 * segments;
  if (hemisphere_only)
  {
    vertices.reserve(hemisphere_ring_count * segments + 1);
  }
  else
  {
    // Double the vertices excluding the shared, equatorial vertices.
    vertices.reserve(2 * (hemisphere_ring_count * segments + 1) - segments);
    index_cap *= 2;
  }
  indices.reserve(index_cap);
  normals.reserve(vertices.capacity());

  // First build a unit sphere.
  // Create vertices for the rings.
  for (unsigned r = 0; r < hemisphere_ring_count; ++r)
  {
    const float ring_height = std::sin(static_cast<float>(r) * ring_step_angle);
    const float ring_radius = std::sqrt(1 - ring_height * ring_height);
    for (unsigned i = 0; i < segments; ++i)
    {
      const float angle = static_cast<float>(i) * segment_angle;
      v = ring_radius * std::cos(angle) * radials[0] + ring_radius * std::sin(angle) * radials[1];
      v += ring_height * axis;
      vertices.emplace_back(v);
    }
  }

  // Add the polar vertex.
  vertices.emplace_back(axis);

  // We have vertices for a hemi-sphere. Mirror if we are building a full sphere.
  if (!hemisphere_only)
  {
    const unsigned mirror_start = segments;  // Skip the shared, equatorial ring.
    const auto mirror_count = int_cast<unsigned>(vertices.size() - 1);
    for (unsigned i = mirror_start; i < mirror_count; ++i)
    {
      v = vertices[i];
      v -= 2.0f * v.dot(axis) * axis;
      vertices.emplace_back(v);
    }

    // Add the polar vertex.
    vertices.emplace_back(-axis);
  }

  // We have a unit sphere. These can be used as normals as is.
  for (auto &v : vertices)
  {
    normals.emplace_back(v);
    // At the same time we can offset the vertices and apply the radius.
    v = origin + radius * v;
  }

  // Finally build the indices for the triangles.
  // Tessellate each ring up the hemispheres.
  unsigned ring_start_index = 0;
  unsigned previous_ring_start_index = 0;
  unsigned pole_index = 0;
  previous_ring_start_index = 0;
  for (unsigned r = 1; r < hemisphere_ring_count; ++r)
  {
    ring_start_index = r * segments;

    for (unsigned i = 0; i < segments; ++i)
    {
      indices.emplace_back(previous_ring_start_index + i);
      indices.emplace_back(previous_ring_start_index + (i + 1) % segments);
      indices.emplace_back(ring_start_index + (i + 1) % segments);

      indices.emplace_back(previous_ring_start_index + i);
      indices.emplace_back(ring_start_index + (i + 1) % segments);
      indices.emplace_back(ring_start_index + i);
    }

    previous_ring_start_index = ring_start_index;
  }

  // Connect the final ring to the polar vertex.
  ring_start_index = (hemisphere_ring_count - 1) * segments;
  pole_index = ring_start_index + segments;
  for (unsigned i = 0; i < segments; ++i)
  {
    indices.emplace_back(ring_start_index + i);
    indices.emplace_back(ring_start_index + (i + 1) % segments);
    indices.emplace_back(pole_index);  // Polar vertex
  }

  // Build lower hemi-sphere as required.
  if (!hemisphere_only)
  {
    const unsigned hemisphere_offset = hemisphere_ring_count * segments + 1;
    // Stil use zero as the first previous ring. This is the shared equator.
    previous_ring_start_index = 0;
    for (unsigned r = 1; r < hemisphere_ring_count; ++r)
    {
      // Take one off r for the shared equator.
      ring_start_index = (r - 1) * segments + hemisphere_offset;

      for (unsigned i = 0; i < segments; ++i)
      {
        indices.emplace_back(previous_ring_start_index + i);
        indices.emplace_back(ring_start_index + (i + 1) % segments);
        indices.emplace_back(previous_ring_start_index + (i + 1) % segments);

        indices.emplace_back(previous_ring_start_index + i);
        indices.emplace_back(ring_start_index + i);
        indices.emplace_back(ring_start_index + (i + 1) % segments);
      }

      previous_ring_start_index = ring_start_index;
    }

    // Connect the final ring to the polar vertex.
    // Take two from hemisphere_ring_count for the shared equator.
    if (hemisphere_ring_count > 1)
    {
      ring_start_index = (hemisphere_ring_count - 1 - 1) * segments + hemisphere_offset;
      pole_index = ring_start_index + segments;
    }
    else
    {
      // Shared equator.
      ring_start_index = 0;
      // Skip the other pole index.
      pole_index = ring_start_index + segments + 1;
    }
    for (unsigned i = 0; i < segments; ++i)
    {
      indices.emplace_back(ring_start_index + (i + 1) % segments);
      indices.emplace_back(ring_start_index + i);
      indices.emplace_back(pole_index);  // Polar vertex
    }
  }
}


void solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
           std::vector<Vector3f> &normals, float radius, const Vector3f &origin, unsigned depth)
{
  solid(vertices, indices, radius, origin, depth);

  normals.resize(vertices.size());

  for (size_t i = 0; i < vertices.size(); ++i)
  {
    normals[i] = (vertices[i] - origin).normalised();
  }
}


void solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, float radius,
           const Vector3f &origin, unsigned depth)
{
  SphereVertexMap vertex_map;
  initialise(vertices, indices, &vertex_map);

  // We also limit the maximum number of iterations.
  for (unsigned i = 0; i < depth; ++i)
  {
    // Subdivide polygons.
    subdivide(vertices, indices, vertex_map);
  }

  // Move and scale the points.
  for (Vector3f &vert : vertices)
  {
    vert = vert * radius + origin;
  }
}

void wireframe(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, float radius,
               const Vector3f &origin, unsigned ring_vertex_count)
{
  ring_vertex_count = std::max(3u, ring_vertex_count);
  if (ring_vertex_count < 0)
  {
    ring_vertex_count = 3;
  }

  vertices.clear();
  indices.clear();
  vertices.reserve(ring_vertex_count * 3);
  indices.reserve(ring_vertex_count * 3);

  // Build a circle around the Z axis.
  for (unsigned i = 0; i < ring_vertex_count; ++i)
  {
    const float angle = static_cast<float>(i) * 2.0f * static_cast<float>(M_PI) /
                        static_cast<float>(ring_vertex_count);
    vertices.emplace_back(origin + radius * Vector3f(std::cos(angle), std::sin(angle), 0));
  }

  // Build a circle around the Y axis.
  for (unsigned i = 0; i < ring_vertex_count; ++i)
  {
    const float angle = static_cast<float>(i) * 2.0f * static_cast<float>(M_PI) /
                        static_cast<float>(ring_vertex_count);
    vertices.emplace_back(origin + radius * Vector3f(std::cos(angle), 0, std::sin(angle)));
  }

  // Build a circle around the X axis.
  for (unsigned i = 0; i < ring_vertex_count; ++i)
  {
    const float angle = static_cast<float>(i) * 2.0f * static_cast<float>(M_PI) /
                        static_cast<float>(ring_vertex_count);
    vertices.emplace_back(origin + radius * Vector3f(0, std::cos(angle), std::sin(angle)));
  }

  // Build indices.
  // Z circle.
  unsigned voffset = 0;
  for (unsigned i = 0; i < ring_vertex_count - 1; ++i)
  {
    indices.emplace_back(voffset + i);
    indices.emplace_back(voffset + i + 1);
  }
  // Complete the circle.
  indices.emplace_back(voffset + ring_vertex_count - 1);
  indices.emplace_back(voffset);

  // Y circle.
  voffset += ring_vertex_count;
  for (unsigned i = 0; i < ring_vertex_count - 1; ++i)
  {
    indices.emplace_back(voffset + i);
    indices.emplace_back(voffset + i + 1);
  }
  // Complete the circle.
  indices.emplace_back(voffset + ring_vertex_count - 1);
  indices.emplace_back(voffset);

  // Y circle.
  voffset += ring_vertex_count;
  for (unsigned i = 0; i < ring_vertex_count - 1; ++i)
  {
    indices.emplace_back(voffset + i);
    indices.emplace_back(voffset + i + 1);
  }
  // Complete the circle.
  indices.emplace_back(voffset + ring_vertex_count - 1);
  indices.emplace_back(voffset);
}
// clang-format off
// NOLINTEND(bugprone-misplaced-widening-cast, bugprone-implicit-widening-of-multiplication-result)
// clang-format on
}  // namespace tes::sphere
