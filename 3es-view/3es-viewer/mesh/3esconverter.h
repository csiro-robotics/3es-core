#ifndef TES_VIEWER_MESH_CONVERTER_H
#define TES_VIEWER_MESH_CONVERTER_H

#include "3es-viewer.h"

#include <Magnum/GL/Mesh.h>

namespace tes
{
class MeshResource;
}

namespace tes::viewer::mesh
{
Magnum::GL::Mesh convert(const tes::MeshResource &mesh_resource);
}  // namespace tes::viewer::mesh

#endif  // TES_VIEWER_MESH_CONVERTER_H
