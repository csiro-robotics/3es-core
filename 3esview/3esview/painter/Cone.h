
#ifndef TES_VIEW_PAINTER_CONE_H
#define TES_VIEW_PAINTER_CONE_H

#include <3esview/ViewConfig.h>

#include "ShapePainter.h"

namespace tes::view::painter
{
/// Cone painter.
class TES_VIEWER_API Cone : public ShapePainter
{
public:
  /// Constructor.
  /// @param culler Bounds culler
  Cone(std::shared_ptr<BoundsCuller> culler, std::shared_ptr<shaders::ShaderLibrary> shaders);

  /// Solid mesh creation function.
  /// @return A solid (or transparent) mesh representation.
  static Magnum::GL::Mesh solidMesh();

  /// Wireframe mesh creation function.
  /// @return A wireframe mesh representation.
  static Magnum::GL::Mesh wireframeMesh();

private:
};
}  // namespace tes::view::painter

#endif  // TES_VIEW_PAINTER_CONE_H
