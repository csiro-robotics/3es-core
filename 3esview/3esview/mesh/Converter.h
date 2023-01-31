#ifndef TES_VIEW_MESH_CONVERTER_H
#define TES_VIEW_MESH_CONVERTER_H

#include <3esview/ViewConfig.h>

#include <3escore/Colour.h>
#include <3escore/Bounds.h>

#include <Magnum/GL/Mesh.h>

namespace tes
{
class MeshResource;
}

namespace tes::view::mesh
{
/// Options to adjust the behaviour of @c convert() functions.
struct TES_VIEWER_API ConvertOptions
{
  /// Default colour to apply if @c auto_colour is set.
  Colour default_colour = { 255, 255, 255 };
  /// If indices are missing, automatically create sequential vertex indexing.
  bool auto_index = false;
  /// If colours are missing, automatically apply @c default_colour to each vertex.
  bool auto_colour = false;
};

Magnum::GL::Mesh convert(const tes::MeshResource &mesh_resource, tes::Bounds<Magnum::Float> &bounds,
                         const ConvertOptions &options = {});

inline Magnum::GL::Mesh convert(const tes::MeshResource &mesh_resource, const ConvertOptions &options = {})
{
  tes::Bounds<Magnum::Float> bounds;
  return convert(mesh_resource, bounds, options);
}
}  // namespace tes::view::mesh

#endif  // TES_VIEW_MESH_CONVERTER_H
