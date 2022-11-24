#include "3esconverter.h"

#include <3esmeshmessages.h>
#include <shapes/3esmeshresource.h>

#include <Magnum/Magnum.h>

#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Color.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Trade/MeshData.h>

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/ArrayViewStl.h>

#include <vector>

namespace tes::viewer::mesh
{
struct VertexP
{
  Magnum::Vector3 position;
};

struct VertexPN
{
  Magnum::Vector3 position;
  Magnum::Vector3 normal;
};

struct VertexPC
{
  Magnum::Vector3 position;
  Magnum::Color4 colour;
};

struct VertexPNC
{
  Magnum::Vector3 position;
  Magnum::Vector3 normal;
  Magnum::Color4 colour;
};

template <typename T>
using Array = Corrade::Containers::Array<T>;
template <typename T>
using ArrayView = Corrade::Containers::ArrayView<T>;

template <typename V>
struct VertexMapper
{
  inline bool validate(const DataBuffer &src_vertices, const DataBuffer &src_normals,
                       const DataBuffer &src_colours) const
  {
    (void)src_vertices;
    (void)src_normals;
    (void)src_colours;
    return false;
  }
  void operator()(V &vertex, size_t src_index, const DataBuffer &src_vertices, const DataBuffer &src_normals,
                  const DataBuffer &src_colours) = delete;

  Array<Magnum::Trade::MeshAttributeData> attributes(const Array<V> &vertices) const = delete;
};


template <>
struct VertexMapper<VertexP>
{
  inline bool validate(const DataBuffer &src_vertices, const DataBuffer &src_normals,
                       const DataBuffer &src_colours) const
  {
    (void)src_normals;
    (void)src_colours;
    return src_vertices.isValid();
  }
  inline Magnum::Vector3 operator()(VertexP &vertex, size_t src_index, const DataBuffer &src_vertices,
                                    const DataBuffer &src_normals, const DataBuffer &src_colours)
  {
    const auto x = src_vertices.get<Magnum::Float>(src_index, 0);
    const auto y = src_vertices.get<Magnum::Float>(src_index, 1);
    const auto z = src_vertices.get<Magnum::Float>(src_index, 2);
    vertex.position = Magnum::Vector3(x, y, z);
    return vertex.position;
  }

  Array<Magnum::Trade::MeshAttributeData> attributes(const Array<VertexP> &vertices) const
  {
    return Array<Magnum::Trade::MeshAttributeData>{
      Corrade::Containers::InPlaceInit,
      { Magnum::Trade::MeshAttributeData{
        Magnum::Trade::MeshAttribute::Position,
        Corrade::Containers::stridedArrayView(vertices, &vertices[0].position, vertices.size(), sizeof(VertexP)) } }
    };
  }
};


template <>
struct VertexMapper<VertexPN>
{
  inline bool validate(const DataBuffer &src_vertices, const DataBuffer &src_normals,
                       const DataBuffer &src_colours) const
  {
    (void)src_colours;
    return src_vertices.isValid() && src_normals.isValid();
  }
  inline Magnum::Vector3 operator()(VertexPN &vertex, size_t src_index, const DataBuffer &src_vertices,
                                    const DataBuffer &src_normals, const DataBuffer &src_colours)
  {
    const auto x = src_vertices.get<Magnum::Float>(src_index, 0);
    const auto y = src_vertices.get<Magnum::Float>(src_index, 1);
    const auto z = src_vertices.get<Magnum::Float>(src_index, 2);
    vertex.position = Magnum::Vector3(x, y, z);
    const auto nx = src_normals.get<Magnum::Float>(src_index, 0);
    const auto ny = src_normals.get<Magnum::Float>(src_index, 1);
    const auto nz = src_normals.get<Magnum::Float>(src_index, 2);
    vertex.normal = Magnum::Vector3(nx, ny, nz);
    return vertex.position;
  }

  Array<Magnum::Trade::MeshAttributeData> attributes(const Array<VertexPN> &vertices) const
  {
    return Array<Magnum::Trade::MeshAttributeData>{
      Corrade::Containers::InPlaceInit,
      {
        Magnum::Trade::MeshAttributeData{
          Magnum::Trade::MeshAttribute::Position,
          Corrade::Containers::stridedArrayView(vertices, &vertices[0].position, vertices.size(), sizeof(VertexPN)) },
        Magnum::Trade::MeshAttributeData{
          Magnum::Trade::MeshAttribute::Normal,
          Corrade::Containers::stridedArrayView(vertices, &vertices[0].normal, vertices.size(), sizeof(VertexPN)) },
      }
    };
  }
};


template <>
struct VertexMapper<VertexPC>
{
  inline bool validate(const DataBuffer &src_vertices, const DataBuffer &src_normals,
                       const DataBuffer &src_colours) const
  {
    (void)src_normals;
    return src_vertices.isValid() && src_colours.isValid();
  }
  inline Magnum::Vector3 operator()(VertexPC &vertex, size_t src_index, const DataBuffer &src_vertices,
                                    const DataBuffer &src_normals, const DataBuffer &src_colours)
  {
    const auto x = src_vertices.get<Magnum::Float>(src_index, 0);
    const auto y = src_vertices.get<Magnum::Float>(src_index, 1);
    const auto z = src_vertices.get<Magnum::Float>(src_index, 2);
    vertex.position = Magnum::Vector3(x, y, z);
    const auto c = Colour(src_colours.get<uint32_t>(src_index));
    vertex.colour = Magnum::Color4(Magnum::Color4ub(c.r, c.g, c.b, c.a));
    return vertex.position;
  }

  Array<Magnum::Trade::MeshAttributeData> attributes(const Array<VertexPC> &vertices) const
  {
    return Array<Magnum::Trade::MeshAttributeData>{
      Corrade::Containers::InPlaceInit,
      {
        Magnum::Trade::MeshAttributeData{
          Magnum::Trade::MeshAttribute::Position,
          Corrade::Containers::stridedArrayView(vertices, &vertices[0].position, vertices.size(), sizeof(VertexPC)) },
        Magnum::Trade::MeshAttributeData{
          Magnum::Trade::MeshAttribute::Color,
          Corrade::Containers::stridedArrayView(vertices, &vertices[0].colour, vertices.size(), sizeof(VertexPC)) },
      }
    };
  }
};


template <>
struct VertexMapper<VertexPNC>
{
  inline bool validate(const DataBuffer &src_vertices, const DataBuffer &src_normals,
                       const DataBuffer &src_colours) const
  {
    return src_vertices.isValid() && src_normals.isValid() && src_colours.isValid();
  }
  inline Magnum::Vector3 operator()(VertexPNC &vertex, size_t src_index, const DataBuffer &src_vertices,
                                    const DataBuffer &src_normals, const DataBuffer &src_colours)
  {
    const auto x = src_vertices.get<Magnum::Float>(src_index, 0);
    const auto y = src_vertices.get<Magnum::Float>(src_index, 1);
    const auto z = src_vertices.get<Magnum::Float>(src_index, 2);
    vertex.position = Magnum::Vector3(x, y, z);
    const auto nx = src_normals.get<Magnum::Float>(src_index, 0);
    const auto ny = src_normals.get<Magnum::Float>(src_index, 1);
    const auto nz = src_normals.get<Magnum::Float>(src_index, 2);
    vertex.normal = Magnum::Vector3(nx, ny, nz);
    const auto c = Colour(src_colours.get<uint32_t>(src_index));
    vertex.colour = Magnum::Color4(Magnum::Color4ub(c.r, c.g, c.b, c.a));
    return vertex.position;
  }

  Array<Magnum::Trade::MeshAttributeData> attributes(const Array<VertexPNC> &vertices) const
  {
    return Array<Magnum::Trade::MeshAttributeData>{
      Corrade::Containers::InPlaceInit,
      {
        Magnum::Trade::MeshAttributeData{
          Magnum::Trade::MeshAttribute::Position,
          Corrade::Containers::stridedArrayView(vertices, &vertices[0].position, vertices.size(), sizeof(VertexPNC)) },
        Magnum::Trade::MeshAttributeData{
          Magnum::Trade::MeshAttribute::Normal,
          Corrade::Containers::stridedArrayView(vertices, &vertices[0].normal, vertices.size(), sizeof(VertexPNC)) },
        Magnum::Trade::MeshAttributeData{
          Magnum::Trade::MeshAttribute::Color,
          Corrade::Containers::stridedArrayView(vertices, &vertices[0].colour, vertices.size(), sizeof(VertexPNC)) },
      }
    };
  }
};


template <typename V>
Magnum::GL::Mesh convert(const tes::MeshResource &mesh_resource, Magnum::MeshPrimitive draw_type,
                         tes::Bounds<Magnum::Float> &bounds, const ConvertOptions &options)
{
  Array<V> vertices(Corrade::Containers::DefaultInit, mesh_resource.vertexCount());
  Array<Magnum::UnsignedInt> indices(Corrade::Containers::DefaultInit, mesh_resource.indexCount());

  const DataBuffer src_vertices = mesh_resource.vertices();
  const DataBuffer src_normals = mesh_resource.normals();
  const DataBuffer src_colour = mesh_resource.colours();

  VertexMapper<V> mapper;
  if (!mapper.validate(src_vertices, src_normals, src_colour))
  {
    return Magnum::GL::Mesh();
  }

  for (size_t i = 0; i < vertices.size(); ++i)
  {
    const auto vertex = mapper(vertices[i], i, src_vertices, src_normals, src_colour);
    if (i != 0)
    {
      bounds.expand(tes::Vector3<Magnum::Float>(vertex.x(), vertex.y(), vertex.z()));
    }
    else
    {
      bounds = Bounds<Magnum::Float>(tes::Vector3<Magnum::Float>(vertex.x(), vertex.y(), vertex.z()));
    }
  }

  const DataBuffer src_indices = mesh_resource.indices();
  if (src_indices.count())
  {
    for (size_t i = 0; i < indices.size(); ++i)
    {
      indices[i] = src_indices.get<unsigned>(i);
    }
  }
  else if (options.auto_index)
  {
    indices = Array<Magnum::UnsignedInt>(Corrade::Containers::DefaultInit, mesh_resource.indexCount());
    for (unsigned i = 0; i < unsigned(indices.size()); ++i)
    {
      indices[i] = i;
    }
  }

  if (!indices.empty())
  {
    Magnum::Trade::MeshData md(draw_type, Magnum::Trade::DataFlags{}, ArrayView<const void>(indices),
                               Magnum::Trade::MeshIndexData{ indices }, Magnum::Trade::DataFlags{},
                               ArrayView<const void>(vertices), mapper.attributes(vertices));
    return Magnum::MeshTools::compile(md);
  }
  Magnum::Trade::MeshData md(draw_type, Magnum::Trade::DataFlags{}, ArrayView<const void>(vertices),
                             mapper.attributes(vertices));
  return Magnum::MeshTools::compile(md);
}

Magnum::GL::Mesh convert(const tes::MeshResource &mesh_resource, tes::Bounds<Magnum::Float> &bounds,
                         const ConvertOptions &options)
{
  Magnum::MeshPrimitive primitive = {};

  switch (mesh_resource.drawType())
  {
  case DtPoints:
    primitive = Magnum::MeshPrimitive::Points;
    break;
  case DtLines:
    primitive = Magnum::MeshPrimitive::Lines;
    break;
  case DtTriangles:
    primitive = Magnum::MeshPrimitive::Triangles;
    break;
  case DtVoxels:
    // Requires the right geometry shader to work with this primitive type.
    primitive = Magnum::MeshPrimitive::Points;
    break;
  }

  if (mesh_resource.normals().isValid())
  {
    if (mesh_resource.colours().isValid())
    {
      return convert<VertexPNC>(mesh_resource, primitive, bounds, options);
    }
    return convert<VertexPN>(mesh_resource, primitive, bounds, options);
  }
  else if (mesh_resource.colours().isValid())
  {
    return convert<VertexPC>(mesh_resource, primitive, bounds, options);
  }
  return convert<VertexP>(mesh_resource, primitive, bounds, options);
}
}  // namespace tes::viewer::mesh
