//
// author: Kazys Stepanas
//
#include "3esbox.h"

#include <algorithm>

using namespace tes;

namespace
{
  // Box vertices. Note we alias vertices in order to generate the correct face normals.
  const tes::Vector3f boxVertices[] =  //
  {                                    //
                                       // +X
    tes::Vector3f(0.5f, 0.5f, -0.5f), tes::Vector3f(0.5f, 0.5f, 0.5f), tes::Vector3f(0.5f, -0.5f, 0.5f),
    tes::Vector3f(0.5f, -0.5f, -0.5f),
    // -X
    tes::Vector3f(-0.5f, -0.5f, -0.5f), tes::Vector3f(-0.5f, -0.5f, 0.5f), tes::Vector3f(-0.5f, 0.5f, 0.5f),
    tes::Vector3f(-0.5f, 0.5f, -0.5f),

    // +Y
    tes::Vector3f(-0.5f, 0.5f, -0.5f), tes::Vector3f(-0.5f, 0.5f, 0.5f), tes::Vector3f(0.5f, 0.5f, 0.5f),
    tes::Vector3f(0.5f, 0.5f, -0.5f),
    // -Y
    tes::Vector3f(0.5f, -0.5f, -0.5f), tes::Vector3f(0.5f, -0.5f, 0.5f), tes::Vector3f(-0.5f, -0.5f, 0.5f),
    tes::Vector3f(-0.5f, -0.5f, -0.5f),

    // +Z
    tes::Vector3f(0.5f, -0.5f, 0.5f), tes::Vector3f(0.5f, 0.5f, 0.5f), tes::Vector3f(-0.5f, 0.5f, 0.5f),
    tes::Vector3f(-0.5f, -0.5f, 0.5f),
    // -Z
    tes::Vector3f(0.5f, 0.5f, -0.5f), tes::Vector3f(0.5f, -0.5f, -0.5f), tes::Vector3f(-0.5f, -0.5f, -0.5f),
    tes::Vector3f(-0.5f, 0.5f, -0.5f)
  };

  // Box normals. Indexing matches boxVertices
  const tes::Vector3f boxNormals[] =  //
  {                                    //
    // +X
    tes::Vector3f(1.0f, 0.0f, 0.0f), tes::Vector3f(1.0f, 0.0f, 0.0f), tes::Vector3f(1.0f, 0.0f, 0.0f),
    tes::Vector3f(1.0f, 0.0f, 0.0f),
    // -X
    tes::Vector3f(-1.0f, 0.0f, 0.0f), tes::Vector3f(-1.0f, 0.0f, 0.0f), tes::Vector3f(-1.0f, 0.0f, 0.0f),
    tes::Vector3f(-1.0f, 0.0f, 0.0f),

    // +Y
    tes::Vector3f(0.0f, 1.0f, 0.0f), tes::Vector3f(0.0f, 1.0f, 0.0f), tes::Vector3f(0.0f, 1.0f, 0.0f),
    tes::Vector3f(0.0f, 1.0f, 0.0f),
    // -Y
    tes::Vector3f(0.0f, -1.0f, 0.0f), tes::Vector3f(0.0f, -1.0f, 0.0f), tes::Vector3f(0.0f, -1.0f, 0.0f),
    tes::Vector3f(0.0f, -1.0f, 0.0f),

    // +Z
    tes::Vector3f(0.0f, 0.0f, 1.0f), tes::Vector3f(0.0f, 0.0f, 1.0f), tes::Vector3f(0.0f, 0.0f, 1.0f),
    tes::Vector3f(0.0f, 0.0f, 1.0f),
    // -Z
    tes::Vector3f(0.0f, 0.0f, -1.0f), tes::Vector3f(0.0f, 0.0f, -1.0f), tes::Vector3f(0.0f, 0.0f, -1.0f),
    tes::Vector3f(0.0f, 0.0f, -1.0f)
  };

  const size_t boxVertexCount = sizeof(boxVertices) / sizeof(boxVertices[0]);

  const unsigned boxIndices[] =  //
    {                             //
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

  const size_t boxIndexCount = sizeof(boxIndices) / sizeof(boxIndices[0]);


  void makeBox(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> *normals,
               const Vector3f &scale)
  {
    vertices.resize(boxVertexCount);
    for (size_t i = 0; i < boxVertexCount; ++i)
    {
      vertices[i] = Vector3f(scale.x * boxVertices[i].x, scale.y * boxVertices[i].y, scale.z * boxVertices[i].z);
    }

    if (normals)
    {
      normals->resize(boxVertexCount);
      std::copy(boxNormals, boxNormals + boxVertexCount, normals->begin());
    }

    indices.resize(boxIndexCount);
    std::copy(boxIndices, boxIndices + boxIndexCount, indices.begin());
  }
}


void tes::box::solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> &normals,
                     const Vector3f &scale)
{
  return makeBox(vertices, indices, &normals, scale);
}


void tes::box::solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
                     const Vector3f &scale)
{
  return makeBox(vertices, indices, nullptr, scale);
}
