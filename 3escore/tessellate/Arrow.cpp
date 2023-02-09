//
// author: Kazys Stepanas
//
#include "Arrow.h"

#include "Cone.h"
#include "Cylinder.h"

//
#include <3escore/CoreUtil.h>
#include <3escore/Quaternion.h>

#include <algorithm>

namespace tes::arrow
{
namespace
{
// Disabled linter warnings on doing unsigned maths, then passing the results as arguments to
// std::vector::operator[] and std::vector::resize(). The choice to use unsigned maths is
// deliberate.
// clang-format off
// NOLINTBEGIN(bugprone-misplaced-widening-cast, bugprone-implicit-widening-of-multiplication-result)
// clang-format on
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
bool buildArrow(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
                std::vector<Vector3f> *normals, unsigned facets, float head_radius,
                float cylinder_radius, float cylinder_length, float arrow_length,
                const Vector3f &arrow_axis)
{
  if (facets < 3 || cylinder_length <= 0 || arrow_length < 0 || arrow_length <= cylinder_length ||
      head_radius <= 0 || cylinder_radius <= 0 || head_radius <= cylinder_radius)
  {
    return false;
  }

  // For each facet we have these vertices:
  // - cone walls (apex and base) * 2
  // - cone base * 2
  // - cylinder walls top/bottom * 2
  // - cylinder end cap
  const size_t facets_multiplier = 2u + 2u + 2u + 1u;
  vertices.resize(facets * facets_multiplier);
  if (normals)
  {
    normals->resize(vertices.size());
  }
  Vector3f normal;
  Vector3f apex;
  Vector3f to_apex;

  const Vector3f build_axis(0, 0, 1);
  // Set the cone apex.
  unsigned vind = 0;
  for (unsigned i = 0; i < facets; ++i)
  {
    vertices[vind] = build_axis * arrow_length;
    ++vind;
  }

  // Cone wall and base: 2 vertices offset by facets
  for (unsigned i = 0; i < facets; ++i)
  {
    const float facet_angle = static_cast<float>(i * 2 * M_PI) / static_cast<float>(facets);
    vertices[vind] = vertices[vind + facets] = Vector3f(
      head_radius * std::sin(facet_angle), head_radius * std::cos(facet_angle), cylinder_length);
    ++vind;
  }
  // Account for having built two vertices per facet above.
  vind += facets;

  // Cylinder/cone seem (cone base and cylinder top): 2 vertices per facet for normal generation.
  for (unsigned i = 0; i < facets; ++i)
  {
    const float facet_angle = static_cast<float>(i * 2 * M_PI) / static_cast<float>(facets);
    vertices[vind] = vertices[vind + facets] =
      Vector3f(cylinder_radius * std::sin(facet_angle), cylinder_radius * std::cos(facet_angle),
               cylinder_length);
    ++vind;
  }
  // Account for having built two vertices per facet above.
  vind += facets;

  // Cylinder bottom and base: 2 vertices each
  for (unsigned i = 0; i < facets; ++i)
  {
    const float facet_angle = static_cast<float>(i * 2 * M_PI) / static_cast<float>(facets);
    vertices[vind] = vertices[vind + facets] =
      Vector3f(cylinder_radius * std::sin(facet_angle), cylinder_radius * std::cos(facet_angle), 0);
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
      to_apex = apex - normal;
      // Remove height component from the future normal
      normal -= normal.dot(build_axis) * build_axis;

      // Cross and normalise to get the actual normal.
      const Vector3f v2 = to_apex.cross(normal);
      normal = v2.cross(to_apex).normalised();
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
      (*normals)[vind] = -1.0f * build_axis;
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
  std::array<unsigned, 4> quad;
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

  if (arrow_axis.dot(build_axis) < 1.0f)
  {
    // Need to transform along a different axis.
    const Quaternionf rotation = Quaternionf(build_axis, arrow_axis);
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

bool solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
           std::vector<Vector3f> &normals, unsigned facets, float head_radius,
           float cylinder_radius, float cylinder_length, float arrow_length, const Vector3f &axis)
{
  return buildArrow(vertices, indices, &normals, facets, head_radius, cylinder_radius,
                    cylinder_length, arrow_length, axis);
}

bool solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, unsigned facets,
           float head_radius, float cylinder_radius, float cylinder_length, float arrow_length,
           const Vector3f &axis)

{
  return buildArrow(vertices, indices, nullptr, facets, head_radius, cylinder_radius,
                    cylinder_length, arrow_length, axis);
}

bool wireframe(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, unsigned facets,
               float head_radius, float cylinder_radius, float cylinder_length, float arrow_length,
               const Vector3f &axis)
{
  if (facets < 3 || cylinder_length <= 0 || arrow_length < 0 || arrow_length <= cylinder_length ||
      head_radius <= 0 || cylinder_radius <= 0 || head_radius <= cylinder_radius)
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
  const float head_length = arrow_length - cylinder_length;
  const float head_angle = std::atan(head_radius / head_length);
  cone::wireframe(vertices, indices, axis * arrow_length, axis, head_length, head_angle, facets);

  // Add a cylinder.
  const auto cylinder_base_index = int_cast<unsigned>(vertices.size());
  cylinder::wireframe(vertices, indices, axis, cylinder_length, cylinder_radius, facets);
  // We need to move the cylinder up so it connects to the head. It's currently centred on the
  // origin.
  for (unsigned i = cylinder_base_index; i < vertices.size(); ++i)
  {
    vertices[i] += axis * 0.5f * cylinder_length;
  }

  // We could also connect the cylinder top ring to the cone ring.

  return true;
}
// clang-format off
// NOLINTEND(bugprone-misplaced-widening-cast, bugprone-implicit-widening-of-multiplication-result)
// clang-format on
}  // namespace tes::arrow
