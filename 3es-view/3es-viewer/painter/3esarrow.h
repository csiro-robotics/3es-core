
#ifndef TES_VIEWER_PAINTER_ARROW_H
#define TES_VIEWER_PAINTER_ARROW_H

#include "3es-viewer.h"

#include "3esshapepainter.h"

namespace tes::viewer::painter
{
/// Arrow painter.
class Arrow : public ShapePainter
{
public:
  /// Constructor.
  /// @param culler Bounds culler
  Arrow(std::shared_ptr<BoundsCuller> culler);

  /// Solid mesh creation function.
  /// @return A solid (or transparent) mesh representation.
  static Magnum::GL::Mesh solidMesh();

  /// Wireframe mesh creation function.
  /// @return A wireframe mesh representation.
  static Magnum::GL::Mesh wireframeMesh();

private:
};
}  // namespace tes::viewer::painter

#endif  // TES_VIEWER_PAINTER_ARROW_H
