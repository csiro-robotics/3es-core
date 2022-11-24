#ifndef TES_VIEWER_MESH_CONVERTER_H
#define TES_VIEWER_MESH_CONVERTER_H

#include "3es-viewer.h"

#include <3esbounds.h>

#include <Magnum/GL/Mesh.h>

namespace tes
{
class MeshResource;
}

namespace tes::viewer::mesh
{
/// Options to adjust the behaviour of @c convert() functions.
struct ConvertOptions
{
  /// If indices are missing, automatically create sequential vertex indexing.
  bool auto_index = false;
};

Magnum::GL::Mesh convert(const tes::MeshResource &mesh_resource, tes::Bounds<Magnum::Float> &bounds,
                         const ConvertOptions &options = {});

inline Magnum::GL::Mesh convert(const tes::MeshResource &mesh_resource, const ConvertOptions &options = {})
{
  tes::Bounds<Magnum::Float> bounds;
  return convert(mesh_resource, bounds, options);
}
}  // namespace tes::viewer::mesh

#endif  // TES_VIEWER_MESH_CONVERTER_H
