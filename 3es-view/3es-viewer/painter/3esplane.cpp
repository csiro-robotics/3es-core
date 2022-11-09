#include "3esplane.h"

#include "mesh/3esconverter.h"

#include <shapes/3essimplemesh.h>

#include <Magnum/GL/Renderer.h>

#include <mutex>

namespace tes::viewer::painter
{
Plane::Plane(std::shared_ptr<BoundsCuller> culler)
  : ShapePainter(std::exchange(culler, nullptr), { Part{ solidMesh() } }, { Part{ wireframeMesh() } },
                 { Part{ solidMesh() } }, ShapeCache::calcSphericalBounds)
{}

Magnum::GL::Mesh Plane::solidMesh()
{
  static SimpleMesh build_mesh(0, 0, 0, DtTriangles, SimpleMesh::Vertex | SimpleMesh::Normal | SimpleMesh::Index);
  static std::mutex guard;

  std::unique_lock<std::mutex> lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    const std::array<tes::Vector3f, 9> vertices = {
      Vector3f(-0.5f, -0.5f, 0.0f),
      Vector3f(0.5f, -0.5f, 0.0f),  //
      Vector3f(0.5f, 0.5f, 0.0f),
      Vector3f(-0.5f, 0.5f, 0.0f),  //
      Vector3f(-0.2f, 0, 0.0f),
      Vector3f(0.2f, 0, 0.0f),  //
      Vector3f(0, -0.2f, 0.0f),
      Vector3f(0, 0.2f, 0.0f),  //
      Vector3f(0, 0, 1),
    };
    const std::array<unsigned, 12> indices = { 0, 1, 2, 0, 2, 3, 4, 5, 8, 6, 7, 8 };
    const std::vector<tes::Vector3f> normals(vertices.size(), Vector3f(0, 0, 1));

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setNormals(0, normals.data(), normals.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}


Magnum::GL::Mesh Plane::wireframeMesh()
{
  static SimpleMesh build_mesh(0, 0, 0, DtLines, SimpleMesh::Vertex | SimpleMesh::Index);
  static std::mutex guard;

  std::unique_lock<std::mutex> lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    const std::array<tes::Vector3f, 6> vertices = {
      Vector3f(-0.5f, -0.5f, 0.0f),
      Vector3f(0.5f, -0.5f, 0.0f),  //
      Vector3f(0.5f, 0.5f, 0.0f),
      Vector3f(-0.5f, 0.5f, 0.0f),  //
      Vector3f(0, 0, 0),
      Vector3f(0, 0, 1),
    };
    const std::array<unsigned, 10> indices = { 0, 1, 1, 2, 2, 3, 3, 0, 4, 5 };

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}


void Plane::drawOpaque(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix)
{
  Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::FaceCulling);
  ShapePainter::drawOpaque(stamp, projection_matrix);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::FaceCulling);
}


void Plane::drawTransparent(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix)
{
  Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::FaceCulling);
  ShapePainter::drawTransparent(stamp, projection_matrix);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::FaceCulling);
}
}  // namespace tes::viewer::painter
