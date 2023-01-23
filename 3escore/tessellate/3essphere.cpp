//
// author: Kazys Stepanas
//
#include "3essphere.h"

#include "3esvector3.h"

#include <array>
#include <cmath>
#include <unordered_map>

namespace tes::sphere
{
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
unsigned insertVertex(const Vector3f &vertex, std::vector<Vector3f> &vertices, SphereVertexMap &vertexMap)
{
  auto findResult = vertexMap.find(vertex);
  if (findResult == vertexMap.end())
  {
    // Add new vertex.
    unsigned idx = unsigned(vertices.size());
    vertices.push_back(vertex);
    vertexMap.insert(std::make_pair(vertex, idx));
    return idx;
  }

  return findResult->second;
}
}  // namespace


void initialise(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, SphereVertexMap *vertexMap)
{
  // We start with two hexagonal rings to approximate the sphere.
  // All subdivision occurs on a unit radius sphere, at the origin. We translate and
  // scale the vertices at the end.
  vertices.clear();
  indices.clear();

  static const float ringControlAngle = 25.0f / 180.0f * float(M_PI);
  static const float ringHeight = std::sin(ringControlAngle);
  static const float ringRadius = std::cos(ringControlAngle);
  static const float hexAngle = 2.0f * float(M_PI) / 6.0f;
  static const float ring2OffsetAngle = 0.5f * hexAngle;
  static const Vector3f initialVertices[] = {
    Vector3f(0, 0, 1),

    // Upper hexagon.
    Vector3f(ringRadius, 0, ringHeight),
    Vector3f(ringRadius * std::cos(hexAngle), ringRadius * std::sin(hexAngle), ringHeight),
    Vector3f(ringRadius * std::cos(2 * hexAngle), ringRadius * std::sin(2 * hexAngle), ringHeight),
    Vector3f(ringRadius * std::cos(3 * hexAngle), ringRadius * std::sin(3 * hexAngle), ringHeight),
    Vector3f(ringRadius * std::cos(4 * hexAngle), ringRadius * std::sin(4 * hexAngle), ringHeight),
    Vector3f(ringRadius * std::cos(5 * hexAngle), ringRadius * std::sin(5 * hexAngle), ringHeight),

    // Lower hexagon.
    Vector3f(ringRadius * std::cos(ring2OffsetAngle), ringRadius * std::sin(ring2OffsetAngle), -ringHeight),
    Vector3f(ringRadius * std::cos(ring2OffsetAngle + hexAngle), ringRadius * std::sin(ring2OffsetAngle + hexAngle),
             -ringHeight),
    Vector3f(ringRadius * std::cos(ring2OffsetAngle + 2 * hexAngle),
             ringRadius * std::sin(ring2OffsetAngle + 2 * hexAngle), -ringHeight),
    Vector3f(ringRadius * std::cos(ring2OffsetAngle + 3 * hexAngle),
             ringRadius * std::sin(ring2OffsetAngle + 3 * hexAngle), -ringHeight),
    Vector3f(ringRadius * std::cos(ring2OffsetAngle + 4 * hexAngle),
             ringRadius * std::sin(ring2OffsetAngle + 4 * hexAngle), -ringHeight),
    Vector3f(ringRadius * std::cos(ring2OffsetAngle + 5 * hexAngle),
             ringRadius * std::sin(ring2OffsetAngle + 5 * hexAngle), -ringHeight),

    Vector3f(0, 0, -1),
  };
  const unsigned initialVertexCount = sizeof(initialVertices) / sizeof(initialVertices[0]);

  const unsigned initialIndices[] = { 0, 1,  2, 0, 2,  3, 0, 3,  4,  0,  4,  5,  0,  5,  6,  0,  6,  1,
                                      1, 7,  2, 2, 8,  3, 3, 9,  4,  4,  10, 5,  5,  11, 6,  6,  12, 1,
                                      7, 8,  2, 8, 9,  3, 9, 10, 4,  10, 11, 5,  11, 12, 6,  12, 7,  1,
                                      7, 13, 8, 8, 13, 9, 9, 13, 10, 10, 13, 11, 11, 13, 12, 12, 13, 7 };
  const unsigned initialIndexCount = sizeof(initialIndices) / sizeof(initialIndices[0]);

  for (unsigned i = 0; i < initialVertexCount; ++i)
  {
    vertices.push_back(initialVertices[i]);
    if (vertexMap)
    {
      vertexMap->insert(std::make_pair(initialVertices[i], i));
    }
  }

  for (unsigned i = 0; i < initialIndexCount; i += 3)
  {
    indices.push_back(initialIndices[i + 0]);
    indices.push_back(initialIndices[i + 1]);
    indices.push_back(initialIndices[i + 2]);
  }
}


void subdivide(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, SphereVertexMap &vertexMap)
{
  const unsigned triangleCount = unsigned(indices.size() / 3);
  unsigned triangle[3];
  unsigned abc[3];
  unsigned def[3];
  Vector3f verts[3];
  Vector3f newVertices[3];

  for (unsigned i = 0; i < triangleCount; ++i)
  {
    triangle[0] = abc[0] = indices[i * 3 + 0];
    triangle[1] = abc[1] = indices[i * 3 + 1];
    triangle[2] = abc[2] = indices[i * 3 + 2];

    // Fetch the vertices.
    verts[0] = vertices[triangle[0]];
    verts[1] = vertices[triangle[1]];
    verts[2] = vertices[triangle[2]];

    // Calculate the new vertex at the centre of the existing triangle.
    newVertices[0] = (0.5f * (verts[0] + verts[1])).normalised();
    newVertices[1] = (0.5f * (verts[1] + verts[2])).normalised();
    newVertices[2] = (0.5f * (verts[2] + verts[0])).normalised();

    // Create new triangles.
    // Given triangle ABC, and adding vertices DEF such that:
    //  D = AB/2  E = BC/2  F = CA/2
    // We have four new triangles:
    //  ADF, BED, CFE, DEF
    // ABC are in order in 'abc', while DEF will be in 'def'.
    // FIXME: find existing point to use.
    def[0] = insertVertex(newVertices[0], vertices, vertexMap);
    def[1] = insertVertex(newVertices[1], vertices, vertexMap);
    def[2] = insertVertex(newVertices[2], vertices, vertexMap);

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


void solidLatLong(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> &normals,
                  float radius, const Vector3f &origin, unsigned hemisphereRingCount, unsigned segments,
                  const Vector3f &axis_in, bool hemisphereOnly)
{
  hemisphereRingCount = std::max(hemisphereRingCount, 1u);
  segments = std::max(segments, 3u);
  Vector3f axis = axis_in;
  if (std::abs(axis.magnitudeSquared() - 1.0f) > 1e-2f)
  {
    axis = Vector3f(0, 0, 1);
  }

  std::array<Vector3f, 2> radials;
  Vector3f v;
  float segmentAngle = 2.0f * float(M_PI) / segments;
  float ringStepAngle = 0.5f * float(M_PI) / hemisphereRingCount;

  if (axis.dot(Vector3f(1, 0, 0)) < 1e-3f)
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
  unsigned indexCap = hemisphereRingCount * segments * 2 * 3 - 3 * segments;
  if (hemisphereOnly)
  {
    vertices.reserve(hemisphereRingCount * segments + 1);
  }
  else
  {
    // Double the vertices excluding the shared, equatorial vertices.
    vertices.reserve(2 * (hemisphereRingCount * segments + 1) - segments);
    indexCap *= 2;
  }
  indices.reserve(indexCap);
  normals.reserve(vertices.capacity());

  // First build a unit sphere.
  // Create vertices for the rings.
  for (unsigned r = 0; r < hemisphereRingCount; ++r)
  {
    float ringHeight = std::sin(r * ringStepAngle);
    float ringRadius = std::sqrt(1 - ringHeight * ringHeight);
    for (unsigned i = 0; i < segments; ++i)
    {
      float angle = i * segmentAngle;
      v = ringRadius * std::cos(angle) * radials[0] + ringRadius * std::sin(angle) * radials[1];
      v += ringHeight * axis;
      vertices.emplace_back(v);
    }
  }

  // Add the polar vertex.
  vertices.emplace_back(axis);

  // We have vertices for a hemi-sphere. Mirror if we are building a full sphere.
  if (!hemisphereOnly)
  {
    unsigned mirrorStart = segments;  // Skip the shared, equatorial ring.
    unsigned mirrorCount = unsigned(vertices.size() - 1);
    for (unsigned i = mirrorStart; i < mirrorCount; ++i)
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
  unsigned ringStartIndex, previousRingStartIndex, poleIndex;
  previousRingStartIndex = 0;
  for (unsigned r = 1; r < hemisphereRingCount; ++r)
  {
    ringStartIndex = r * segments;

    for (unsigned i = 0; i < segments; ++i)
    {
      indices.emplace_back(previousRingStartIndex + i);
      indices.emplace_back(previousRingStartIndex + (i + 1) % segments);
      indices.emplace_back(ringStartIndex + (i + 1) % segments);

      indices.emplace_back(previousRingStartIndex + i);
      indices.emplace_back(ringStartIndex + (i + 1) % segments);
      indices.emplace_back(ringStartIndex + i);
    }

    previousRingStartIndex = ringStartIndex;
  }

  // Connect the final ring to the polar vertex.
  ringStartIndex = (hemisphereRingCount - 1) * segments;
  poleIndex = ringStartIndex + segments;
  for (unsigned i = 0; i < segments; ++i)
  {
    indices.emplace_back(ringStartIndex + i);
    indices.emplace_back(ringStartIndex + (i + 1) % segments);
    indices.emplace_back(poleIndex);  // Polar vertex
  }

  // Build lower hemi-sphere as required.
  if (!hemisphereOnly)
  {
    unsigned hemisphereOffset = hemisphereRingCount * segments + 1;
    // Stil use zero as the first previous ring. This is the shared equator.
    previousRingStartIndex = 0;
    for (unsigned r = 1; r < hemisphereRingCount; ++r)
    {
      // Take one off r for the shared equator.
      ringStartIndex = (r - 1) * segments + hemisphereOffset;

      for (unsigned i = 0; i < segments; ++i)
      {
        indices.emplace_back(previousRingStartIndex + i);
        indices.emplace_back(ringStartIndex + (i + 1) % segments);
        indices.emplace_back(previousRingStartIndex + (i + 1) % segments);

        indices.emplace_back(previousRingStartIndex + i);
        indices.emplace_back(ringStartIndex + i);
        indices.emplace_back(ringStartIndex + (i + 1) % segments);
      }

      previousRingStartIndex = ringStartIndex;
    }

    // Connect the final ring to the polar vertex.
    // Take two from hemisphereRingCount for the shared equator.
    if (hemisphereRingCount > 1)
    {
      ringStartIndex = (hemisphereRingCount - 1 - 1) * segments + hemisphereOffset;
      poleIndex = ringStartIndex + segments;
    }
    else
    {
      // Shared equator.
      ringStartIndex = 0;
      // Skip the other pole index.
      poleIndex = ringStartIndex + segments + 1;
    }
    for (unsigned i = 0; i < segments; ++i)
    {
      indices.emplace_back(ringStartIndex + (i + 1) % segments);
      indices.emplace_back(ringStartIndex + i);
      indices.emplace_back(poleIndex);  // Polar vertex
    }
  }
}


void solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> &normals,
           float radius, const Vector3f &origin, int depth)
{
  solid(vertices, indices, radius, origin, depth);

  normals.resize(vertices.size());

  for (size_t i = 0; i < vertices.size(); ++i)
  {
    normals[i] = (vertices[i] - origin).normalised();
  }
}


void solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, float radius, const Vector3f &origin,
           int depth)
{
  SphereVertexMap vertexMap;
  initialise(vertices, indices, &vertexMap);

  // We also limit the maximum number of iterations.
  for (int i = 0; i < depth; ++i)
  {
    // Subdivide polygons.
    subdivide(vertices, indices, vertexMap);
  }

  // Move and scale the points.
  for (Vector3f &vert : vertices)
  {
    vert = vert * radius + origin;
  }
}

void wireframe(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, float radius, const Vector3f &origin,
               int ring_vertex_count)
{
  ring_vertex_count = std::max(3, ring_vertex_count);
  if (ring_vertex_count < 0)
  {
    ring_vertex_count = 3;
  }

  vertices.clear();
  indices.clear();
  vertices.reserve(ring_vertex_count * 3);
  indices.reserve(ring_vertex_count * 3);

  // Build a circle around the Z axis.
  for (int i = 0; i < ring_vertex_count; ++i)
  {
    float angle = i * 2.0f * float(M_PI) / (float)ring_vertex_count;
    vertices.emplace_back(origin + radius * Vector3f(std::cos(angle), std::sin(angle), 0));
  }

  // Build a circle around the Y axis.
  for (int i = 0; i < ring_vertex_count; ++i)
  {
    float angle = i * 2.0f * float(M_PI) / (float)ring_vertex_count;
    vertices.emplace_back(origin + radius * Vector3f(std::cos(angle), 0, std::sin(angle)));
  }

  // Build a circle around the X axis.
  for (int i = 0; i < ring_vertex_count; ++i)
  {
    float angle = i * 2.0f * float(M_PI) / (float)ring_vertex_count;
    vertices.emplace_back(origin + radius * Vector3f(0, std::cos(angle), std::sin(angle)));
  }

  // Build indices.
  // Z circle.
  int voffset = 0;
  for (int i = 0; i < ring_vertex_count - 1; ++i)
  {
    indices.emplace_back(voffset + i);
    indices.emplace_back(voffset + i + 1);
  }
  // Complete the circle.
  indices.emplace_back(voffset + ring_vertex_count - 1);
  indices.emplace_back(voffset);

  // Y circle.
  voffset += ring_vertex_count;
  for (int i = 0; i < ring_vertex_count - 1; ++i)
  {
    indices.emplace_back(voffset + i);
    indices.emplace_back(voffset + i + 1);
  }
  // Complete the circle.
  indices.emplace_back(voffset + ring_vertex_count - 1);
  indices.emplace_back(voffset);

  // Y circle.
  voffset += ring_vertex_count;
  for (int i = 0; i < ring_vertex_count - 1; ++i)
  {
    indices.emplace_back(voffset + i);
    indices.emplace_back(voffset + i + 1);
  }
  // Complete the circle.
  indices.emplace_back(voffset + ring_vertex_count - 1);
  indices.emplace_back(voffset);
}
}  // namespace tes::sphere