#include "Sphere.h"

#include <3esview/mesh/Converter.h>

#include <3escore/shapes/SimpleMesh.h>
#include <3escore/tessellate/Sphere.h>

#include <mutex>

namespace tes::viewer::painter
{
Sphere::Sphere(std::shared_ptr<BoundsCuller> culler, std::shared_ptr<shaders::ShaderLibrary> shaders)
  : ShapePainter(std::move(culler), std::move(shaders), { Part{ solidMesh() } }, { Part{ wireframeMesh() } },
                 { Part{ solidMesh() } }, ShapeCache::calcSphericalBounds)
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
