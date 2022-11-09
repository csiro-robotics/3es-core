//
// author: Kazys Stepanas
//
#include "3esarrow.h"

#include "3escone.h"
#include "3escylinder.h"
#include "3esquaternion.h"

#include <algorithm>

namespace tes::arrow
{
namespace
{
bool buildArrow(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> *normals,
                unsigned facets, float headRadius, float cylinderRadius, float cylinderLength, float arrowLength,
                const Vector3f arrowAxis)
{
  if (facets < 3 || cylinderLength <= 0 || arrowLength < 0 || arrowLength <= cylinderLength || headRadius <= 0 ||
      cylinderRadius <= 0 || headRadius <= cylinderRadius)
  {
    return false;
  }

  // For each facet we have these vertices:
  // - cone walls (apex and base) * 2
  // - cone base * 2
  // - cylinder walls top/bottom * 2
  // - cylinder end cap
  vertices.resize(facets * (2 + 2 + 2 + 1));
  if (normals)
  {
    normals->resize(vertices.size());
  }
  Vector3f normal, apex, toApex;

  const Vector3f buildAxis(0, 0, 1);
  // Set the cone apex.
  unsigned vind = 0;
  for (unsigned i = 0; i < facets; ++i)
  {
    vertices[vind] = buildAxis * arrowLength;
    ++vind;
  }

  // Cone wall and base: 2 vertices offset by facets
  for (unsigned i = 0; i < facets; ++i)
  {
    const float facetAngle = float(i * 2 * M_PI) / float(facets);
    vertices[vind] = vertices[vind + facets] =
      Vector3f(headRadius * std::sin(facetAngle), headRadius * std::cos(facetAngle), cylinderLength);
    ++vind;
  }
  // Account for having built two vertices per facet above.
  vind += facets;

  // Cylinder/cone seem (cone base and cylinder top): 2 vertices per facet for normal generation.
  for (unsigned i = 0; i < facets; ++i)
  {
    const float facetAngle = float(i * 2 * M_PI) / float(facets);
    vertices[vind] = vertices[vind + facets] =
      Vector3f(cylinderRadius * std::sin(facetAngle), cylinderRadius * std::cos(facetAngle), cylinderLength);
    ++vind;
  }
  // Account for having built two vertices per facet above.
  vind += facets;

  // Cylinder bottom and base: 2 vertices each
  for (unsigned i = 0; i < facets; ++i)
  {
    const float facetAngle = float(i * 2 * M_PI) / float(facets);
    vertices[vind] = vertices[vind + facets] =
      Vector3f(cylinderRadius * std::sin(facetAngle), cylinderRadius * std::cos(facetAngle), 0);
    ++vind;
  }

  // Generate normals. Cone walls first (apex and cylinder wall).
  vind = 0;
  if (normals)
  {
    for (unsigned i = 0; i < facets; ++i)
    {
      normal = vertices[vind + facets];
      apex = vertices[vind];
      toApex = apex - normal;
      // Remove height component from the future normal
      normal -= normal.dot(buildAxis) * buildAxis;

      // Cross and normalise to get the actual normal.
      Vector3f v2 = toApex.cross(normal);
      normal = v2.cross(toApex).normalised();
      (*normals)[vind] = (*normals)[vind + facets] = normal;
      ++vind;
    }
    // Account for having built two normals per facet above.
    vind += facets;

    // Cone base * 2.
    for (unsigned i = 0; i < facets; ++i)
    {
      (*normals)[vind] = (*normals)[vind + facets] = Vector3f(0, 0, -1);
      ++vind;
    }
    // Account for having built two normals per facet above.
    vind += facets;

    // Cylinder walls: top and bottom.
    for (unsigned i = 0; i < facets; ++i)
    {
      // Use the cylinder base vertices as normals. They have no y component.
      (*normals)[vind] = (*normals)[vind + facets] = vertices[vind + 2 * facets].normalised();
      ++vind;
    }
    // Account for having built two normals per facet above.
    vind += facets;

    // Cylinder base.
    for (unsigned i = 0; i < facets; ++i)
    {
      (*normals)[vind] = -1.0f * buildAxis;
      ++vind;
    }
  }

  // Now generate indices to tessellate. Listed below are the number of triangles per part,
  // using three indices per triangle.
  // - Arrow head => facets
  // - Arrow base (cylinder transition) => 2 * facets
  // - Cylinder walls => 2 * facets
  // - Cylinder base => facets - 2
  indices.resize((facets + 2 * facets + 2 * facets + facets - 2) * 3);
  unsigned iind = 0;

  // Cone walls
  for (unsigned i = 0; i < facets; ++i)
  {
    indices[iind++] = i;
    indices[iind++] = (i + 1) % facets + facets;
    indices[iind++] = i + facets;
  }

  // Cone base.
  unsigned quad[4];
  vind = 2 * facets;
  for (unsigned i = 0; i < facets; ++i)
  {
    quad[0] = vind + i;
    quad[1] = vind + (i + 1) % facets;
    quad[2] = vind + facets + i;
    quad[3] = vind + facets + (i + 1) % facets;
    indices[iind++] = quad[0];
    indices[iind++] = quad[1];
    indices[iind++] = quad[2];
    indices[iind++] = quad[1];
    indices[iind++] = quad[3];
    indices[iind++] = quad[2];
  }

  // Cylinder walls.
  vind += 2 * facets;
  for (unsigned i = 0; i < facets; ++i)
  {
    quad[0] = vind + i;
    quad[1] = vind + (i + 1) % facets;
    quad[2] = vind + facets + i;
    quad[3] = vind + facets + (i + 1) % facets;
    indices[iind++] = quad[0];
    indices[iind++] = quad[1];
    indices[iind++] = quad[2];
    indices[iind++] = quad[1];
    indices[iind++] = quad[3];
    indices[iind++] = quad[2];
  }

  // Cylinder/arrow base.
  vind += 2 * facets;
  for (unsigned i = 1; i < facets - 1; ++i)
  {
    indices[iind++] = vind;
    indices[iind++] = vind + i;
    indices[iind++] = vind + i + 1;
  }

  if (arrowAxis.dot(buildAxis) < 1.0f)
  {
    // Need to transform along a different axis.
    const Quaternionf rotation = Quaternionf(buildAxis, arrowAxis);
    for (size_t i = 0; i < vertices.size(); ++i)
    {
      vertices[i] = rotation * vertices[i];
      if (normals)
      {
        (*normals)[i] = rotation * (*normals)[i];
      }
    }
  }

  return true;
}
}  // namespace

bool solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> &normals,
           unsigned facets, float headRadius, float cylinderRadius, float cylinderLength, float arrowLength,
           const Vector3f axis)
{
  return buildArrow(vertices, indices, &normals, facets, headRadius, cylinderRadius, cylinderLength, arrowLength, axis);
}

bool solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, unsigned facets, float headRadius,
           float cylinderRadius, float cylinderLength, float arrowLength, const Vector3f axis)

{
  return buildArrow(vertices, indices, nullptr, facets, headRadius, cylinderRadius, cylinderLength, arrowLength, axis);
}

bool wireframe(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, unsigned segments, float headRadius,
               float cylinderRadius, float cylinderLength, float arrowLength, const Vector3f axis)
{
  if (segments < 3 || cylinderLength <= 0 || arrowLength < 0 || arrowLength <= cylinderLength || headRadius <= 0 ||
      cylinderRadius <= 0 || headRadius <= cylinderRadius)
  {
    return false;
  }

  // Start with a cone.
  // Calculate the cone angle from the head radius.
  //        /|
  //       /a|
  //      /  |
  //     /   | h
  //    /    |
  //   /     |
  //    -----
  //      b
  // a = atan(b/h)
  const float headLength = arrowLength - cylinderLength;
  const float headAngle = std::atan(headRadius / headLength);
  cone::wireframe(vertices, indices, axis * arrowLength, axis, headLength, headAngle, segments);

  // Add a cylinder.
  const unsigned cylinderBaseIndex = unsigned(indices.size());
  cylinder::wireframe(vertices, indices, axis, cylinderLength, cylinderRadius, segments);

  // We need to move the cylinder up so it connects to the head. It's currently centred on the origin.
  for (unsigned i = cylinderBaseIndex; i < vertices.size(); ++i)
  {
    vertices[i] += axis * 0.5f * cylinderLength;
  }

  // We could also connect the cylinder top ring to the cone ring.

  return true;
}
}  // namespace tes::arrow
