#include "3escapsule.h"

#include "mesh/3esconverter.h"

#include <shapes/3essimplemesh.h>
#include <tessellate/3escapsule.h>

#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Trade/MeshData.h>

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/ArrayViewStl.h>

#include <cassert>
#include <mutex>

namespace tes::viewer::painter
{
Capsule::Capsule(std::shared_ptr<BoundsCuller> culler)
  : ShapePainter(std::exchange(culler, nullptr), solidMesh(), wireframeMesh(), solidMesh(), Magnum::Matrix4{},
                 ShapeCache::defaultCalcBounds)
{}

Magnum::GL::Mesh Capsule::solidMesh()
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
    tes::capsule::solid(vertices, indices, normals, 1.0f, Vector3f(0.0f), 3);

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setNormals(0, normals.data(), normals.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}


Magnum::GL::Mesh Capsule::wireframeMesh()
{
  static SimpleMesh build_mesh(0, 0, 0, DtLines, SimpleMesh::Vertex | SimpleMesh::Index);
  static std::mutex guard;

  std::unique_lock<std::mutex> lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    std::vector<tes::Vector3f> vertices;
    std::vector<unsigned> indices;
    tes::capsule::wireframe(vertices, indices, 1.0f);

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}
}  // namespace tes::viewer::painter
