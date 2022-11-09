#include "3essphere.h"

#include "mesh/3esconverter.h"

#include <shapes/3essimplemesh.h>
#include <tessellate/3essphere.h>

#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Trade/MeshData.h>

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/ArrayViewStl.h>

#include <cassert>
#include <mutex>

namespace tes::viewer::painter
{
namespace
{
struct MeshData
{
  struct VertexSolid
  {
    Magnum::Vector3 position;
    Magnum::Vector3 normal;
  };

  struct VertexWireframe
  {
    Magnum::Vector3 position;
  };

  std::vector<VertexSolid> vertices;
  std::vector<Magnum::UnsignedShort> indices;

  static const MeshData &solid()
  {  // Static data items.
    static std::mutex mutex;
    static MeshData data;

    std::unique_lock<std::mutex> guard(mutex);

    if (data.indices.empty())
    {
      // Build with the tes tesselator.
      std::vector<tes::Vector3f> vertices;
      std::vector<tes::Vector3f> normals;
      std::vector<unsigned> indices;
      tes::sphere::solid(vertices, indices, normals, 1.0f, Vector3f(0.0f), 3);

      // Copy to the Magnum types.
      data.vertices.reserve(vertices.size());
      assert(vertices.size() == normals.size());
      for (size_t i = 0; i < std::min(vertices.size(), normals.size()); ++i)
      {
        data.vertices.emplace_back(VertexSolid{ Magnum::Vector3{ vertices[i].x, vertices[i].y, vertices[i].z },
                                                Magnum::Vector3{ normals[i].x, normals[i].y, normals[i].z } });
      }

      data.indices.reserve(indices.size());
      for (auto idx : indices)
      {
        data.indices.emplace_back(idx);
      }
    }

    return data;
  }

  static const MeshData &wireframe()
  {  // Static data items.
    static std::mutex mutex;
    static MeshData data;

    std::unique_lock<std::mutex> guard(mutex);

    // if (data.indices.empty())
    // {
    //   // Build with the tes tesselator.
    //   std::vector<tes::Vector3f> vertices;
    //   std::vector<unsigned> indices;
    //   tes::sphere::wireframe(vertices, indices, 1.0f, Vector3f(0.0f), 36);

    //   // Copy to the Magnum types.
    //   data.vertices.reserve(vertices.size());
    //   assert(vertices.size() == normals.size());
    //   for (size_t i = 0; i < std::min(vertices.size(), normals.size()); ++i)
    //   {
    //     data.vertices.emplace_back(VertexWireframe{ Magnum::Vector3{ vertices[i].x, vertices[i].y, vertices[i].z },
    //                                                 Magnum::Vector3{ normals[i].x, normals[i].y, normals[i].z } });
    //   }

    //   data.indices.reserve(indices.size());
    //   for (auto idx : indices)
    //   {
    //     data.indices.emplace_back(idx);
    //   }
    // }

    return data;
  }
};

}  // namespace

Sphere::Sphere(std::shared_ptr<BoundsCuller> culler)
  : ShapePainter(std::exchange(culler, nullptr), solidMesh(), wireframeMesh(), Magnum::Matrix4{},
                 ShapeCache::defaultCalcBounds)
{}

Magnum::GL::Mesh Sphere::solidMesh()
{
  static SimpleMesh build_mesh(0, 0, 0, DtTriangles, SimpleMesh::Vertex | SimpleMesh::Normal | SimpleMesh::Index);
  static std::mutex guard;

  std::unique_lock<std::mutex> lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    std::vector<tes::Vector3f> vertices;
    std::vector<tes::Vector3f> normals;
    std::vector<unsigned> indices;
    tes::sphere::solid(vertices, indices, normals, 1.0f, Vector3f(0.0f), 3);

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setNormals(0, normals.data(), normals.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}


Magnum::GL::Mesh Sphere::wireframeMesh()
{
  static SimpleMesh build_mesh(0, 0, 0, DtLines, SimpleMesh::Vertex | SimpleMesh::Index);
  static std::mutex guard;

  std::unique_lock<std::mutex> lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    std::vector<tes::Vector3f> vertices;
    std::vector<unsigned> indices;
    tes::sphere::wireframe(vertices, indices, 1.0f);

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}
}  // namespace tes::viewer::painter
