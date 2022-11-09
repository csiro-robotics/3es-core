//
// author: Kazys Stepanas
//
#include "3escapsule.h"

#include "3escylinder.h"
#include "3essphere.h"

#include <algorithm>

namespace tes::capsule
{
namespace
{
void migratePart(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> *normals,
                 const std::vector<Vector3f> &part_vertices, const std::vector<Vector3f> *part_normals,
                 const std::vector<unsigned> &part_indices, unsigned rebase_index)
{
  // Migrate part into the destination arrays, patching indices as we go.
  vertices.reserve(vertices.size() + part_vertices.size());
  std::for_each(part_vertices.begin(), part_vertices.end(),
                [&vertices](const Vector3f &v) { vertices.emplace_back(v); });
  indices.reserve(indices.size() + part_indices.size());
  std::for_each(part_indices.begin(), part_indices.end(),
                [&indices, rebase_index](unsigned idx) { indices.emplace_back(idx + rebase_index); });
  if (normals && part_normals)
  {
    normals->reserve(normals->size() + part_normals->size());
    std::for_each(part_normals->begin(), part_normals->end(),
                  [normals](const Vector3f &n) { normals->emplace_back(n); });
  }
}

void makeCapsule(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> *normals,
                 const Vector3f &axis, float height, float radius, unsigned facets,
                 std::array<PartIndexOffset, 4> *part_isolated_index_offsets, bool local_end_caps)
{
  std::vector<Vector3f> part_vertices;
  std::vector<Vector3f> part_normals;
  std::vector<unsigned> part_indices;

  // Build the parts in the temporary storage, then migrate to the output containers.

  const Vector3f sphereOffset = (local_end_caps) ? Vector3f(0.0f) : 0.5f * height * axis;

  // Generate the top cap hemisphere.
  tes::sphere::solidLatLong(part_vertices, part_indices, part_normals, radius, sphereOffset, 5, facets, axis, true);
  // Set to rebase indices based on the exiting vertex count.
  unsigned rebase_index = vertices.size();
  if (part_isolated_index_offsets)
  {
    // No rebasing. Store the part start index.
    (*part_isolated_index_offsets)[int(PartIndex::TopStart)] =
      PartIndexOffset{ unsigned(vertices.size()), unsigned(indices.size()) };
    rebase_index = 0;
  }
  migratePart(vertices, indices, normals, part_vertices, &part_normals, part_indices, rebase_index);
  part_vertices.clear();
  part_normals.clear();
  part_indices.clear();

  // Build the bottom hemisphere. Flip the axis for the bottom hemisphere.
  tes::sphere::solidLatLong(part_vertices, part_indices, part_normals, radius, -sphereOffset, 5, facets, -axis, true);
  rebase_index = vertices.size();
  if (part_isolated_index_offsets)
  {
    (*part_isolated_index_offsets)[int(PartIndex::BottomStart)] =
      PartIndexOffset{ unsigned(vertices.size()), unsigned(indices.size()) };
    rebase_index = 0;
  }
  migratePart(vertices, indices, normals, part_vertices, &part_normals, part_indices, rebase_index);
  part_vertices.clear();
  part_normals.clear();
  part_indices.clear();

  // Build the open cylinder.
  tes::cylinder::solid(part_vertices, part_indices, part_normals, axis, height, radius, facets, true);
  rebase_index = vertices.size();
  if (part_isolated_index_offsets)
  {
    (*part_isolated_index_offsets)[int(PartIndex::BodyStart)] =
      PartIndexOffset{ unsigned(vertices.size()), unsigned(indices.size()) };
    rebase_index = 0;
  }
  migratePart(vertices, indices, normals, part_vertices, &part_normals, part_indices, rebase_index);
  part_vertices.clear();
  part_normals.clear();
  part_indices.clear();

  if (part_isolated_index_offsets)
  {
    (*part_isolated_index_offsets)[int(PartIndex::BodyEnd)] =
      PartIndexOffset{ unsigned(vertices.size()), unsigned(indices.size()) };
  }
}
}  // namespace


void solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> &normals,
           float height, float radius, unsigned facets, const Vector3f &axis,
           std::array<PartIndexOffset, 4> *part_isolated_index_offsets, bool local_end_caps)
{
  return makeCapsule(vertices, indices, &normals, axis, height, radius, facets, part_isolated_index_offsets,
                     local_end_caps);
}


void solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, float height, float radius, unsigned facets,
           const Vector3f &axis, std::array<PartIndexOffset, 4> *part_isolated_index_offsets, bool local_end_caps)
{
  return makeCapsule(vertices, indices, nullptr, axis, height, radius, facets, part_isolated_index_offsets,
                     local_end_caps);
}


void wireframe(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, float height, float radius,
               unsigned segments, const Vector3f &axis, std::array<PartIndexOffset, 4> *part_isolated_index_offsets,
               bool local_end_caps)
{
  // Build two spheres, connected by four lines.
  std::vector<Vector3f> part_vertices;
  std::vector<unsigned> part_indices;

  // Build the parts in the temporary storage, then migrate to the output containers.

  const Vector3f sphereOffset = (local_end_caps) ? Vector3f(0.0f) : 0.5f * height * axis;

  // Generate the top cap hemisphere.
  tes::sphere::wireframe(part_vertices, part_indices, radius, sphereOffset, segments);
  // Set to rebase indices based on the exiting vertex count.
  unsigned rebase_index = vertices.size();
  if (part_isolated_index_offsets)
  {
    // No rebasing. Store the part start index.
    (*part_isolated_index_offsets)[int(PartIndex::TopStart)] =
      PartIndexOffset{ unsigned(vertices.size()), unsigned(indices.size()) };
    rebase_index = 0;
  }
  migratePart(vertices, indices, nullptr, part_vertices, nullptr, part_indices, rebase_index);
  part_vertices.clear();
  part_indices.clear();

  // Build the bottom hemisphere. Flip the axis for the bottom hemisphere.
  tes::sphere::wireframe(part_vertices, part_indices, radius, -sphereOffset, segments);
  rebase_index = vertices.size();
  if (part_isolated_index_offsets)
  {
    (*part_isolated_index_offsets)[int(PartIndex::BottomStart)] =
      PartIndexOffset{ unsigned(vertices.size()), unsigned(indices.size()) };
    rebase_index = 0;
  }
  migratePart(vertices, indices, nullptr, part_vertices, nullptr, part_indices, rebase_index);
  part_vertices.clear();
  part_indices.clear();

  // Build the lines for the cylinder.
  std::array<Vector3f, 4> radials;

  // Calculate a perpendicular vector to the axis.
  if (axis.cross(Vector3f(1, 0, 0)).magnitudeSquared() > 1e-6f)
  {
    radials[0] = axis.cross(Vector3f(1, 0, 0)).normalised();
  }
  else
  {
    radials[0] = axis.cross(Vector3f(0, 1, 0)).normalised();
  }
  radials[1] = axis.cross(radials[0]).normalised();
  radials[2] = -radials[0];
  radials[3] = -radials[1];

  for (int i = 0; i < 4; ++i)
  {
    part_indices.emplace_back(unsigned(vertices.size()));
    part_vertices.emplace_back(0.5f * height * axis + radials[i] * radius);
    part_indices.emplace_back(unsigned(vertices.size()));
    part_vertices.emplace_back(-0.5f * height * axis + radials[i] * radius);
  }
  rebase_index = vertices.size();
  if (part_isolated_index_offsets)
  {
    (*part_isolated_index_offsets)[int(PartIndex::BodyStart)] =
      PartIndexOffset{ unsigned(vertices.size()), unsigned(indices.size()) };
    rebase_index = 0;
  }
  migratePart(vertices, indices, nullptr, part_vertices, nullptr, part_indices, rebase_index);
  part_vertices.clear();
  part_indices.clear();

  if (part_isolated_index_offsets)
  {
    (*part_isolated_index_offsets)[int(PartIndex::BodyEnd)] =
      PartIndexOffset{ unsigned(vertices.size()), unsigned(indices.size()) };
  }
}
}  // namespace tes::capsule
