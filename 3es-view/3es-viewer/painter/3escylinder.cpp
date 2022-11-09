#include "3escylinder.h"

#include "mesh/3esconverter.h"

#include <shapes/3essimplemesh.h>
#include <tessellate/3escylinder.h>

#include <mutex>

namespace tes::viewer::painter
{
Cylinder::Cylinder(std::shared_ptr<BoundsCuller> culler)
  : ShapePainter(std::exchange(culler, nullptr), { Part{ solidMesh() } }, { Part{ wireframeMesh() } },
                 { Part{ solidMesh() } }, calculateBounds)
{}


void Cylinder::calculateBounds(const Magnum::Matrix4 &transform, Magnum::Vector3 &centre, Magnum::Vector3 &halfExtents)
{
  return ShapeCache::calcCylindricalBounds(transform, 1.0f, 1.0f, centre, halfExtents);
}


Magnum::GL::Mesh Cylinder::solidMesh()
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

    tes::cylinder::solid(vertices, indices, normals, Vector3f(0, 0, 1), 1.0f, 1.0f, 24);

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setNormals(0, normals.data(), normals.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}


Magnum::GL::Mesh Cylinder::wireframeMesh()
{
  static SimpleMesh build_mesh(0, 0, 0, DtLines, SimpleMesh::Vertex | SimpleMesh::Index);
  static std::mutex guard;

  std::unique_lock<std::mutex> lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    std::vector<tes::Vector3f> vertices;
    std::vector<unsigned> indices;
    tes::cylinder::wireframe(vertices, indices, Vector3f(0, 0, 1), 1.0f, 1.0f, 8);

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}
}  // namespace tes::viewer::painter
