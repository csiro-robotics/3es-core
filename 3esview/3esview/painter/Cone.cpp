#include "Cone.h"

#include <3esview/mesh/Converter.h>

#include <3escore/shapes/SimpleMesh.h>
#include <3escore/tessellate/Cone.h>

#include <mutex>

namespace tes::viewer::painter
{
Cone::Cone(std::shared_ptr<BoundsCuller> culler, std::shared_ptr<shaders::ShaderLibrary> shaders)
  : ShapePainter(std::move(culler), std::move(shaders), { Part{ solidMesh() } }, { Part{ wireframeMesh() } },
                 { Part{ solidMesh() } }, ShapeCache::calcSphericalBounds)
{}

Magnum::GL::Mesh Cone::solidMesh()
{
  static SimpleMesh build_mesh(0, 0, 0, DtTriangles, SimpleMesh::Vertex | SimpleMesh::Normal | SimpleMesh::Index);
  static std::mutex guard;

  std::unique_lock<std::mutex> lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    // Calculate the cone radius from the cone angle.
    //        /|
    //       /a|
    //      /  |
    //     /   | h
    //    /    |
    //   /     |
    //    -----
    //      r
    // a = atan(r/h)
    // r = h * tan(a)
    const float coneLength = 1.0f;
    const float coneRadius = 1.0f;
    const float coneAngle = std::atan(coneRadius / coneLength);

    std::vector<tes::Vector3f> vertices;
    std::vector<tes::Vector3f> normals;
    std::vector<unsigned> indices;
    tes::cone::solid(vertices, indices, normals, Vector3f(0, 0, coneLength), Vector3f(0, 0, coneLength), coneLength,
                     coneAngle, 24);

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setNormals(0, normals.data(), normals.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}


Magnum::GL::Mesh Cone::wireframeMesh()
{
  static SimpleMesh build_mesh(0, 0, 0, DtLines, SimpleMesh::Vertex | SimpleMesh::Index);
  static std::mutex guard;

  std::unique_lock<std::mutex> lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    // Calculate the cone radius from the cone angle.
    //        /|
    //       /a|
    //      /  |
    //     /   | h
    //    /    |
    //   /     |
    //    -----
    //      r
    // a = atan(r/h)
    // r = h * tan(a)
    const float coneLength = 1.0f;
    const float coneRadius = 1.0f;
    const float coneAngle = std::atan(coneRadius / coneLength);
    std::vector<tes::Vector3f> vertices;
    std::vector<unsigned> indices;
    tes::cone::wireframe(vertices, indices, Vector3f(0, 0, coneLength), Vector3f(0, 0, coneLength), coneLength,
                         coneAngle, 16);

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}
}  // namespace tes::viewer::painter
