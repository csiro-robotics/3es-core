//
// author: Kazys Stepanas
//
#include "3esbox.h"

#include <array>
#include <algorithm>

namespace tes::box
{

namespace
{
// Box vertices. Note we alias vertices in order to generate the correct face normals.
const std::array<Vector3f, 24> boxVertices =  //
  {
    // +X
    Vector3f(0.5f, 0.5f, -0.5f), Vector3f(0.5f, 0.5f, 0.5f), Vector3f(0.5f, -0.5f, 0.5f), Vector3f(0.5f, -0.5f, -0.5f),
    // -X
    Vector3f(-0.5f, -0.5f, -0.5f), Vector3f(-0.5f, -0.5f, 0.5f), Vector3f(-0.5f, 0.5f, 0.5f),
    Vector3f(-0.5f, 0.5f, -0.5f),

    // +Y
    Vector3f(-0.5f, 0.5f, -0.5f), Vector3f(-0.5f, 0.5f, 0.5f), Vector3f(0.5f, 0.5f, 0.5f), Vector3f(0.5f, 0.5f, -0.5f),
    // -Y
    Vector3f(0.5f, -0.5f, -0.5f), Vector3f(0.5f, -0.5f, 0.5f), Vector3f(-0.5f, -0.5f, 0.5f),
    Vector3f(-0.5f, -0.5f, -0.5f),

    // +Z
    Vector3f(0.5f, -0.5f, 0.5f), Vector3f(0.5f, 0.5f, 0.5f), Vector3f(-0.5f, 0.5f, 0.5f), Vector3f(-0.5f, -0.5f, 0.5f),
    // -Z
    Vector3f(0.5f, 0.5f, -0.5f), Vector3f(0.5f, -0.5f, -0.5f), Vector3f(-0.5f, -0.5f, -0.5f),
    Vector3f(-0.5f, 0.5f, -0.5f)
  };

// Box normals. Indexing matches boxVertices
const std::array<Vector3f, 24> boxNormals =  //
  {
    // +X
    Vector3f(1.0f, 0.0f, 0.0f), Vector3f(1.0f, 0.0f, 0.0f), Vector3f(1.0f, 0.0f, 0.0f), Vector3f(1.0f, 0.0f, 0.0f),
    // -X
    Vector3f(-1.0f, 0.0f, 0.0f), Vector3f(-1.0f, 0.0f, 0.0f), Vector3f(-1.0f, 0.0f, 0.0f), Vector3f(-1.0f, 0.0f, 0.0f),

    // +Y
    Vector3f(0.0f, 1.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f),
    // -Y
    Vector3f(0.0f, -1.0f, 0.0f), Vector3f(0.0f, -1.0f, 0.0f), Vector3f(0.0f, -1.0f, 0.0f), Vector3f(0.0f, -1.0f, 0.0f),

    // +Z
    Vector3f(0.0f, 0.0f, 1.0f), Vector3f(0.0f, 0.0f, 1.0f), Vector3f(0.0f, 0.0f, 1.0f), Vector3f(0.0f, 0.0f, 1.0f),
    // -Z
    Vector3f(0.0f, 0.0f, -1.0f), Vector3f(0.0f, 0.0f, -1.0f), Vector3f(0.0f, 0.0f, -1.0f), Vector3f(0.0f, 0.0f, -1.0f)
  };

const std::array<unsigned, 36> boxIndices =  //
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


const std::array<Vector3f, 8> wireBoxVertices = {
  Vector3f(-0.5f, 0.5f, -0.5f),  Vector3f(0.5f, 0.5f, -0.5f),  Vector3f(0.5f, -0.5f, -0.5f),
  Vector3f(-0.5f, -0.5f, -0.5f), Vector3f(-0.5f, 0.5f, 0.5f),  Vector3f(0.5f, 0.5f, 0.5f),
  Vector3f(0.5f, -0.5f, 0.5f),   Vector3f(-0.5f, -0.5f, 0.5f),
};

const std::array<unsigned, 24> wireBoxIndices = {
  0, 1, 1, 2, 2, 3, 3, 0,  // bottom
  4, 5, 5, 6, 6, 7, 7, 4,  // top
  0, 4, 1, 5, 2, 6, 3, 7   // sides
};


void makeBox(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> *normals,
             const Vector3f &scale)
{
  vertices.resize(boxVertices.size());
  for (size_t i = 0; i < boxVertices.size(); ++i)
  {
    vertices[i] = Vector3f(scale.x * boxVertices[i].x, scale.y * boxVertices[i].y, scale.z * boxVertices[i].z);
  }

  if (normals)
  {
    normals->resize(boxNormals.size());
    std::copy(boxNormals.begin(), boxNormals.end(), normals->begin());
  }

  indices.resize(boxIndices.size());
  std::copy(boxIndices.begin(), boxIndices.end(), indices.begin());
}
}  // namespace


void solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> &normals,
           const Vector3f &scale)
{
  return makeBox(vertices, indices, &normals, scale);
}


void solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, const Vector3f &scale)
{
  return makeBox(vertices, indices, nullptr, scale);
}


void wireframe(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, const Vector3f &scale)
{
  (void)scale;
  const unsigned initialVertCount = unsigned(vertices.size());
  const unsigned initialIdxCount = unsigned(indices.size());
  vertices.resize(vertices.size() + wireBoxVertices.size());
  indices.resize(indices.size() + wireBoxIndices.size());

  std::copy(wireBoxVertices.begin(), wireBoxVertices.end(), vertices.begin() + initialVertCount);
  std::copy(wireBoxIndices.begin(), wireBoxIndices.end(), indices.begin() + initialIdxCount);
}
}  // namespace tes::box
