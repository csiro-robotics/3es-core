//
// author: Kazys Stepanas
//
#include "Cylinder.h"

#include <array>
#include <algorithm>

namespace tes::cylinder
{
namespace
{
void makeCylinder(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> *normals,
                  const Vector3f &axis, float height, float radius, unsigned facets, bool open)
{
  facets = std::max(facets, 3u);
  float segmentAngle = float(2.0 * M_PI) / float(facets);

  // Build two radial vectors out from the cylinder axis perpendicular to each other (like a cylinder).
  Vector3f radials[2];
  const float nearAlignedDot = std::cos(85.0f / 180.0f * float(M_PI));
  if (axis.dot(Vector3f::axisy) < nearAlignedDot)
  {
    radials[0] = Vector3f::axisy.cross(axis);
  }
  else
  {
    radials[0] = Vector3f::axisx.cross(axis);
  }
  radials[0].normalise();
  radials[1] = axis.cross(radials[0]);

  vertices.resize(facets * ((open) ? 2 : 4));
  if (normals)
  {
    normals->resize(vertices.size());
  }

  Vector3f ringCentre, radial, vertex;
  ringCentre = axis * 0.5f * height;
  for (unsigned f = 0; f < facets; ++f)
  {
    const float facetAngle = float(f) * segmentAngle;
    radial = radius * (std::cos(facetAngle) * radials[0] + std::sin(facetAngle) * radials[1]);
    vertex = ringCentre + radial;
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
  const unsigned topRingStartIndex = 0;
  const unsigned bottomRingStartIndex = facets;
  for (unsigned f = 0; f < facets; ++f)
  {
    indices.push_back(bottomRingStartIndex + f);
    indices.push_back(bottomRingStartIndex + (f + 1) % facets);
    indices.push_back(topRingStartIndex + (f + 1) % facets);

    indices.push_back(bottomRingStartIndex + f);
    indices.push_back(topRingStartIndex + (f + 1) % facets);
    indices.push_back(topRingStartIndex + f);
  }

  // Triangulate the end caps.
  if (!open)
  {
    const unsigned topCapStartIndex = 2 * facets;
    const unsigned bottomCapStartIndex = 3 * facets;
    for (unsigned f = 1; f < facets - 1; ++f)
    {
      indices.push_back(topCapStartIndex + 0);
      indices.push_back(topCapStartIndex + f);
      indices.push_back(topCapStartIndex + f + 1);
    }

    for (unsigned f = 1; f < facets - 1; ++f)
    {
      indices.push_back(bottomCapStartIndex + 0);
      indices.push_back(bottomCapStartIndex + f + 1);
      indices.push_back(bottomCapStartIndex + f);
    }
  }
}
}  // namespace


void solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, const Vector3f &axis, float height,
           float angle, unsigned facets, bool open)
{
  return makeCylinder(vertices, indices, nullptr, axis, height, angle, facets, open);
}


void solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> &normals,
           const Vector3f &axis, float height, float radius, unsigned facets, bool open)
{
  return makeCylinder(vertices, indices, &normals, axis, height, radius, facets, open);
}

void wireframe(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, const Vector3f &axis, float height,
               float radius, unsigned segments)
{
  // Build a rings.
  // Build the lines for the cylinder.
  std::array<Vector3f, 2> radials;

  // Calculate base vectors perpendicular to the axis.
  if (axis.cross(Vector3f(1, 0, 0)).magnitudeSquared() > 1e-6f)
  {
    radials[0] = axis.cross(Vector3f(1, 0, 0)).normalised();
  }
  else
  {
    radials[0] = axis.cross(Vector3f(0, 1, 0)).normalised();
  }
  radials[1] = axis.cross(radials[0]).normalised();

  // Build a circle around the axis.
  std::array<unsigned, 2> ringsStart;
  ringsStart[0] = unsigned(vertices.size());
  for (unsigned i = 0; i < segments; ++i)
  {
    const float circleAngle = float(i) * 2.0f * float(M_PI) / (float)segments;
    vertices.emplace_back(radius * std::cos(circleAngle) * radials[0] +  //
                          radius * std::sin(circleAngle) * radials[1]);
  }

  // Duplicate the ring.
  ringsStart[1] = unsigned(vertices.size());
  for (unsigned i = 0; i < segments; ++i)
  {
    vertices.emplace_back(vertices[ringsStart[0] + i]);
    // Also fix up the ring vertices, offsetting them with the axis.
    vertices[ringsStart[0] + i] += axis * 0.5f * height;
    vertices.back() -= axis * 0.5f * height;
  }

  // Build the ring vertices.
  for (int r = 0; r < 2; ++r)
  {
    for (unsigned i = 0; i < segments; ++i)
    {
      indices.emplace_back(ringsStart[r] + i);
      indices.emplace_back(ringsStart[r] + (i + 1) % segments);
    }
  }

  // Connec the rings.
  for (unsigned i = 0; i < segments; ++i)
  {
    indices.emplace_back(ringsStart[0] + i);
    indices.emplace_back(ringsStart[1] + i);
  }
}
}  // namespace tes::cylinder
