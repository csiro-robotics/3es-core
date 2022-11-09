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
