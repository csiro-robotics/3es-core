//
// author: Kazys Stepanas
//
#include "Box.h"

#include <3escore/CoreUtil.h>

#include <algorithm>
#include <array>

namespace tes::box
{

namespace
{
// Box vertices. Note we alias vertices in order to generate the correct face normals.
const std::array<Vector3f, 24> kBoxVertices =  //
  {
    // +X
    Vector3f(0.5f, 0.5f, -0.5f), Vector3f(0.5f, 0.5f, 0.5f), Vector3f(0.5f, -0.5f, 0.5f),
    Vector3f(0.5f, -0.5f, -0.5f),
    // -X
    Vector3f(-0.5f, -0.5f, -0.5f), Vector3f(-0.5f, -0.5f, 0.5f), Vector3f(-0.5f, 0.5f, 0.5f),
    Vector3f(-0.5f, 0.5f, -0.5f),

    // +Y
    Vector3f(-0.5f, 0.5f, -0.5f), Vector3f(-0.5f, 0.5f, 0.5f), Vector3f(0.5f, 0.5f, 0.5f),
    Vector3f(0.5f, 0.5f, -0.5f),
    // -Y
    Vector3f(0.5f, -0.5f, -0.5f), Vector3f(0.5f, -0.5f, 0.5f), Vector3f(-0.5f, -0.5f, 0.5f),
    Vector3f(-0.5f, -0.5f, -0.5f),

    // +Z
    Vector3f(0.5f, -0.5f, 0.5f), Vector3f(0.5f, 0.5f, 0.5f), Vector3f(-0.5f, 0.5f, 0.5f),
    Vector3f(-0.5f, -0.5f, 0.5f),
    // -Z
    Vector3f(0.5f, 0.5f, -0.5f), Vector3f(0.5f, -0.5f, -0.5f), Vector3f(-0.5f, -0.5f, -0.5f),
    Vector3f(-0.5f, 0.5f, -0.5f)
  };

// Box normals. Indexing matches kBoxVertices
const std::array<Vector3f, 24> kBoxNormals =  //
  {
    // +X
    Vector3f(1.0f, 0.0f, 0.0f), Vector3f(1.0f, 0.0f, 0.0f), Vector3f(1.0f, 0.0f, 0.0f),
    Vector3f(1.0f, 0.0f, 0.0f),
    // -X
    Vector3f(-1.0f, 0.0f, 0.0f), Vector3f(-1.0f, 0.0f, 0.0f), Vector3f(-1.0f, 0.0f, 0.0f),
    Vector3f(-1.0f, 0.0f, 0.0f),

    // +Y
    Vector3f(0.0f, 1.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f),
    Vector3f(0.0f, 1.0f, 0.0f),
    // -Y
    Vector3f(0.0f, -1.0f, 0.0f), Vector3f(0.0f, -1.0f, 0.0f), Vector3f(0.0f, -1.0f, 0.0f),
    Vector3f(0.0f, -1.0f, 0.0f),

    // +Z
    Vector3f(0.0f, 0.0f, 1.0f), Vector3f(0.0f, 0.0f, 1.0f), Vector3f(0.0f, 0.0f, 1.0f),
    Vector3f(0.0f, 0.0f, 1.0f),
    // -Z
    Vector3f(0.0f, 0.0f, -1.0f), Vector3f(0.0f, 0.0f, -1.0f), Vector3f(0.0f, 0.0f, -1.0f),
    Vector3f(0.0f, 0.0f, -1.0f)
  };

const std::array<unsigned, 36> kBoxIndices =  //
  {
    // +X
    0, 1, 2, 0, 2, 3,
    // -X
    4, 5, 6, 4, 6, 7,
    // +Y
    8, 9, 10, 8, 10, 11,
    // -Y
    12, 13, 14, 12, 14, 15,
    // +Z
    16, 17, 18, 16, 18, 19,
    // -Z
    20, 21, 22, 20, 22, 23
  };


const std::array<Vector3f, 8> kWireBoxVertices = {
  Vector3f(-0.5f, 0.5f, -0.5f),  Vector3f(0.5f, 0.5f, -0.5f),  Vector3f(0.5f, -0.5f, -0.5f),
  Vector3f(-0.5f, -0.5f, -0.5f), Vector3f(-0.5f, 0.5f, 0.5f),  Vector3f(0.5f, 0.5f, 0.5f),
  Vector3f(0.5f, -0.5f, 0.5f),   Vector3f(-0.5f, -0.5f, 0.5f),
};

const std::array<unsigned, 24> kWireBoxIndices = {
  0, 1, 1, 2, 2, 3, 3, 0,  // bottom
  4, 5, 5, 6, 6, 7, 7, 4,  // top
  0, 4, 1, 5, 2, 6, 3, 7   // sides
};


void makeBox(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
             std::vector<Vector3f> *normals, const Vector3f &scale)
{
  vertices.resize(kBoxVertices.size());
  for (size_t i = 0; i < kBoxVertices.size(); ++i)
  {
    vertices[i] = Vector3f(scale.x() * kBoxVertices[i].x(), scale.y() * kBoxVertices[i].y(),
                           scale.z() * kBoxVertices[i].z());
  }

  if (normals)
  {
    normals->resize(kBoxNormals.size());
    std::copy(kBoxNormals.begin(), kBoxNormals.end(), normals->begin());
  }

  indices.resize(kBoxIndices.size());
  std::copy(kBoxIndices.begin(), kBoxIndices.end(), indices.begin());
}
}  // namespace


void solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
           std::vector<Vector3f> &normals, const Vector3f &scale)
{
  return makeBox(vertices, indices, &normals, scale);
}


void solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, const Vector3f &scale)
{
  return makeBox(vertices, indices, nullptr, scale);
}


void wireframe(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
               const Vector3f &scale)
{
  (void)scale;
  const auto initial_vert_count = int_cast<unsigned>(vertices.size());
  const auto initial_idx_count = int_cast<unsigned>(indices.size());
  vertices.resize(vertices.size() + kWireBoxVertices.size());
  indices.resize(indices.size() + kWireBoxIndices.size());

  std::copy(kWireBoxVertices.begin(), kWireBoxVertices.end(),
            vertices.begin() + initial_vert_count);
  std::copy(kWireBoxIndices.begin(), kWireBoxIndices.end(), indices.begin() + initial_idx_count);
}
}  // namespace tes::box
