//
// author: Kazys Stepanas
//
#include "3escone.h"

#include <algorithm>

using namespace tes;

namespace
{
  void makeCone(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> *normals,
                const Vector3f &apex, const Vector3f &axis, float height, float angle, unsigned facets)
  {
    facets = std::max(facets, 3u);
    const float baseRadius = height * std::atanf(angle);
    float segmentAngle = float(2.0 * M_PI) / float(facets);

    // Build two radial vectors out from the cone axis perpendicular to each other (like a cylinder). We'll use these
    // to build the base ring.
    Vector3f radials[2];
    const float nearAlignedDot = std::cosf(85.0f / 180.0f * float(M_PI));
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

    // Three sets of vertices: base walls, apex walls, base closer.
    vertices.resize(facets * 3);
    if (normals)
    {
      normals->resize(vertices.size());
    }

    Vector3f ringCentre, radial, vertex, tangent, normal, toApex;
    ringCentre = apex - axis * height;
    for (unsigned f = 0; f < facets; ++f)
    {
      const float facetAngle = f * segmentAngle;
      radial = baseRadius * (std::cosf(facetAngle) * radials[0] + std::sinf(facetAngle) * radials[1]);
      vertex = ringCentre + radial;
      vertices[f] = vertex;
      // And the apex vertex. One for each facet for distinct normals.
      vertices[f + facets] = apex;
      vertices[f + 2 * facets] = vertex;

      if (normals)
      {
        toApex = apex - vertex;
        tangent = axis.cross(toApex);
        normal = toApex.cross(tangent).normalised();
        (*normals)[f] = (*normals)[f + facets] = normal;
        // Base normals.
        (*normals)[f + 2 * facets] = -axis;
      }
    }

    // Now triangulate between the rings.
    unsigned triangleCount = 0;
    const unsigned wallRingStartIndex = 0;
    const unsigned apexRingStartIndex = facets;
    for (unsigned f = 0; f < facets; ++f)
    {
      indices.push_back(wallRingStartIndex + f);
      indices.push_back(wallRingStartIndex + (f + 1) % facets);
      indices.push_back(apexRingStartIndex + (f + 1) % facets);

      indices.push_back(wallRingStartIndex + f);
      indices.push_back(apexRingStartIndex + (f + 1) % facets);
      indices.push_back(apexRingStartIndex + f);
    }

    // Tesselate the base.
    const unsigned baseRingStartIndex = 2 * facets;
    for (unsigned f = 1; f < facets - 1; ++f)
    {
      indices.push_back(baseRingStartIndex + 0);
      indices.push_back(baseRingStartIndex + f + 1);
      indices.push_back(baseRingStartIndex + f);
    }
  }
}


void tes::cone::solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
                      std::vector<Vector3f> &normals,
                      const Vector3f &apex, const Vector3f &axis, float height, float angle, unsigned facets)
{
  return makeCone(vertices, indices, &normals, apex, axis, height, angle, facets);
}


void tes::cone::solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
                      const Vector3f &apex, const Vector3f &axis, float height, float angle, unsigned facets)
{
  return makeCone(vertices, indices, nullptr, apex, axis, height, angle, facets);
}
