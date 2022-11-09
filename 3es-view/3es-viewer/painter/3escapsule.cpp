#include "3escapsule.h"

#include "mesh/3esconverter.h"

#include <shapes/3essimplemesh.h>
#include <tessellate/3escapsule.h>

#include <cassert>
#include <mutex>

namespace tes::viewer::painter
{
Capsule::Capsule(std::shared_ptr<BoundsCuller> culler)
  : ShapePainter(std::exchange(culler, nullptr), solidMesh(), wireframeMesh(), solidMesh(),
                 ShapeCache::defaultCalcBounds)
{}

std::vector<Capsule::Part> Capsule::solidMesh()
{
  static std::array<SimpleMesh, 3> build_mesh = {
    SimpleMesh(0, 0, 0, DtTriangles, SimpleMesh::Vertex | SimpleMesh::Normal | SimpleMesh::Index),
    SimpleMesh(0, 0, 0, DtTriangles, SimpleMesh::Vertex | SimpleMesh::Normal | SimpleMesh::Index),
    SimpleMesh(0, 0, 0, DtTriangles, SimpleMesh::Vertex | SimpleMesh::Normal | SimpleMesh::Index)
  };
  static std::mutex guard;

  std::unique_lock<std::mutex> lock(guard);

  // Build with the tes tesselator.
  if (build_mesh[0].vertexCount() == 0)
  {
    std::vector<tes::Vector3f> vertices;
    std::vector<tes::Vector3f> normals;
    std::vector<unsigned> indices;
    std::array<capsule::PartIndexOffset, 4> index_offsets;
    tes::capsule::solid(vertices, indices, normals, 1.0f, 1.0f, 24, Vector3f(0, 0, 1), &index_offsets, true);

    for (int i = 0; i < 3; ++i)
    {
      build_mesh[i].setVertexCount(index_offsets[i + 1].vertices - index_offsets[i].vertices);
      build_mesh[i].setIndexCount(index_offsets[i + 1].indices - index_offsets[i].indices);

      build_mesh[i].setVertices(0, vertices.data() + index_offsets[0].vertices, build_mesh[i].vertexCount());
      build_mesh[i].setNormals(0, normals.data(), build_mesh[i].vertexCount());
      build_mesh[i].setIndices(0, indices.data(), build_mesh[i].indexCount());
    }
  }

  const Magnum::Vector3 half_axis(0, 0, 0.5f);
  return { Capsule::Part(mesh::convert(build_mesh[0]), Magnum::Matrix4::translation(half_axis)),
           Capsule::Part(mesh::convert(build_mesh[1])),
           Capsule::Part(mesh::convert(build_mesh[2]), Magnum::Matrix4::translation(-half_axis)) };
}


std::vector<Capsule::Part> Capsule::wireframeMesh()
{
  static std::array<SimpleMesh, 3> build_mesh = {
    SimpleMesh(0, 0, 0, DtLines, SimpleMesh::Vertex | SimpleMesh::Index),
    SimpleMesh(0, 0, 0, DtLines, SimpleMesh::Vertex | SimpleMesh::Index),
    SimpleMesh(0, 0, 0, DtLines, SimpleMesh::Vertex | SimpleMesh::Index),
  };
  static std::mutex guard;

  std::unique_lock<std::mutex> lock(guard);

  // Build with the tes tesselator.
  if (build_mesh[0].vertexCount() == 0)
  {
    std::vector<tes::Vector3f> vertices;
    std::vector<tes::Vector3f> normals;
    std::vector<unsigned> indices;
    std::array<capsule::PartIndexOffset, 4> index_offsets;
    tes::capsule::wireframe(vertices, indices, 1.0f, 1.0f, 24, Vector3f(0, 0, 1), &index_offsets, true);

    for (int i = 0; i < 3; ++i)
    {
      build_mesh[i].setVertexCount(index_offsets[i + 1].vertices - index_offsets[i].vertices);
      build_mesh[i].setIndexCount(index_offsets[i + 1].indices - index_offsets[i].indices);

      build_mesh[i].setVertices(0, vertices.data() + index_offsets[0].vertices, build_mesh[i].vertexCount());
      build_mesh[i].setNormals(0, normals.data(), build_mesh[i].vertexCount());
      build_mesh[i].setIndices(0, indices.data(), build_mesh[i].indexCount());
    }
  }

  const Magnum::Vector3 half_axis(0, 0, 0.5f);
  return { Capsule::Part(mesh::convert(build_mesh[0]), Magnum::Matrix4::translation(half_axis)),
           Capsule::Part(mesh::convert(build_mesh[1])),
           Capsule::Part(mesh::convert(build_mesh[2]), Magnum::Matrix4::translation(-half_axis)) };
}
}  // namespace tes::viewer::painter
