#include "Pose.h"

#include <3esview/mesh/Converter.h>

#include <3escore/shapes/SimpleMesh.h>
#include <3escore/tessellate/Arrow.h>

#include <mutex>

namespace tes::view::painter
{
Pose::Pose(std::shared_ptr<BoundsCuller> culler, std::shared_ptr<shaders::ShaderLibrary> shaders)
  : ShapePainter(std::move(culler), std::move(shaders), { Part{ solidMesh() } },
                 { Part{ wireframeMesh() } }, { Part{ solidMesh() } },
                 ShapeCache::calcSphericalBounds)
{}

Magnum::GL::Mesh Pose::solidMesh()
{
  static SimpleMesh build_mesh(
    0, 0, 0, DtTriangles,
    SimpleMesh::Vertex | SimpleMesh::Normal | SimpleMesh::Colour | SimpleMesh::Index);
  static std::mutex guard;

  std::unique_lock<std::mutex> lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    std::vector<Vector3f> vertices;
    std::vector<Vector3f> vertices_part;
    std::vector<Vector3f> normals;
    std::vector<Vector3f> normals_part;
    std::vector<uint32_t> colours;
    std::vector<unsigned> indices;
    std::vector<unsigned> indices_part;

    const unsigned facets = 24;
    const float head_radius = 0.1f;
    const float body_radius = 0.05f;
    const float length = 1.0f;
    const float body_length = 0.81f;

    unsigned base_index = unsigned(vertices.size());
    arrow::solid(vertices_part, indices_part, normals_part, facets, head_radius, body_radius,
                 body_length, length, Vector3f(1, 0, 0));

    // Size buffers for 3 arrows.
    vertices.reserve(vertices_part.size() * 3);
    vertices.resize(vertices_part.size());
    normals.reserve(normals_part.size() * 3);
    normals.resize(normals_part.size());
    indices.reserve(indices_part.size() * 3);

    // Copy from temporary buffer.
    std::copy(vertices_part.begin(), vertices_part.end(), vertices.begin() + base_index);
    std::copy(normals_part.begin(), normals_part.end(), normals.begin() + base_index);
    for (size_t i = 0; i < vertices.size(); ++i)
    {
      colours.emplace_back(Colour(255, 0, 0).colour32());
    }
    for (const auto i : indices_part)
    {
      indices.emplace_back(i + base_index);
    }

    base_index = unsigned(vertices.size());
    arrow::solid(vertices_part, indices_part, normals_part, facets, head_radius, body_radius,
                 body_length, length, Vector3f(0, 1, 0));
    vertices.resize(vertices.size() + vertices_part.size());
    normals.resize(normals.size() + normals_part.size());
    // Copy from temporary buffer.
    std::copy(vertices_part.begin(), vertices_part.end(), vertices.begin() + base_index);
    std::copy(normals_part.begin(), normals_part.end(), normals.begin() + base_index);
    for (size_t i = 0; i < vertices.size(); ++i)
    {
      colours.emplace_back(Colour(255, 0, 0).colour32());
    }
    for (const auto i : indices_part)
    {
      indices.emplace_back(i + base_index);
    }

    base_index = unsigned(vertices.size());
    arrow::solid(vertices_part, indices_part, normals_part, facets, head_radius, body_radius,
                 body_length, length, Vector3f(0, 0, 1));
    vertices.resize(vertices.size() + vertices_part.size());
    normals.resize(normals.size() + normals_part.size());
    // Copy from temporary buffer.
    std::copy(vertices_part.begin(), vertices_part.end(), vertices.begin() + base_index);
    std::copy(normals_part.begin(), normals_part.end(), normals.begin() + base_index);
    for (size_t i = 0; i < vertices.size(); ++i)
    {
      colours.emplace_back(Colour(255, 0, 0).colour32());
    }
    for (const auto i : indices_part)
    {
      indices.emplace_back(i + base_index);
    }

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setNormals(0, normals.data(), normals.size());
    build_mesh.setColours(0, colours.data(), colours.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}


Magnum::GL::Mesh Pose::wireframeMesh()
{
  // static SimpleMesh build_mesh(0, 0, 0, DtLines, SimpleMesh::Vertex | SimpleMesh::Colour |
  // SimpleMesh::Index);
  static SimpleMesh build_mesh(0, 0, 0, DtLines, SimpleMesh::Vertex | SimpleMesh::Index);
  static std::mutex guard;

  std::unique_lock<std::mutex> lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    const std::array<Vector3f, 6> vertices = {
      Vector3f(0, 0, 0), Vector3f(1, 0, 0), Vector3f(0, 0, 0),
      Vector3f(0, 1, 0), Vector3f(0, 0, 0), Vector3f(0, 0, 1),
    };
    // const std::array<uint32_t, 6> colours = {
    //   Colour(255, 0, 0).c, Colour(255, 0, 0).c, Colour(0, 255, 0).c,
    //   Colour(0, 255, 0).c, Colour(0, 0, 255).c, Colour(0, 0, 255).c,
    // };
    const std::array<unsigned, 6> indices = { 0, 1, 2, 3, 4, 5 };

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    // build_mesh.setColours(0, colours.data(), colours.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}
}  // namespace tes::view::painter
