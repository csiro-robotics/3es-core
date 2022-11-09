#include "3essphere.h"

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
};

}  // namespace

Sphere::Sphere(std::shared_ptr<BoundsCuller> culler)
  : ShapePainter(std::exchange(culler, nullptr), solidMesh(), wireframeMesh(), Magnum::Matrix4{},
                 ShapeCache::defaultCalcBounds)
{}

Magnum::GL::Mesh Sphere::solidMesh()
{
  const MeshData &data = MeshData::solid();
  Corrade::Containers::Array<Magnum::Trade::MeshAttributeData> attributes{
    Corrade::Containers::InPlaceInit,
    { Magnum::Trade::MeshAttributeData{ Magnum::Trade::MeshAttribute::Position,
                                        Corrade::Containers::stridedArrayView(data.vertices, &data.vertices[0].position,
                                                                              data.vertices.size(),
                                                                              sizeof(MeshData::VertexSolid)) },
      Magnum::Trade::MeshAttributeData{ Magnum::Trade::MeshAttribute::Normal,
                                        Corrade::Containers::stridedArrayView(data.vertices, &data.vertices[0].normal,
                                                                              data.vertices.size(),
                                                                              sizeof(MeshData::VertexSolid)) } }
  };

  using namespace Corrade::Containers;
  auto index_view = ArrayView<const Magnum::UnsignedShort>(data.indices);
  auto vertex_view = ArrayView<const void>(data.vertices);
  Magnum::Trade::MeshData md(Magnum::MeshPrimitive::Triangles, Magnum::Trade::DataFlags{},
                             ArrayView<const void>(index_view), Magnum::Trade::MeshIndexData{ index_view },
                             Magnum::Trade::DataFlags{}, vertex_view, std::move(attributes));
  return Magnum::MeshTools::compile(md);
}


Magnum::GL::Mesh Sphere::wireframeMesh()
{
  return Magnum::GL::Mesh();
}
}  // namespace tes::viewer::painter
