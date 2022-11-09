#ifndef TES_VIEWER_PAINTER_CAPSULE_H
#define TES_VIEWER_PAINTER_CAPSULE_H

#include "3es-viewer.h"

#include "3esshapepainter.h"

namespace tes::viewer::painter
{
/// Capsule painter.
class Capsule : public ShapePainter
{
public:
  /// Constructor.
  /// @param culler Bounds culler
  Capsule(std::shared_ptr<BoundsCuller> culler);

  /// Solid mesh creation function.
  /// @return A solid (or transparent) mesh representation.
  static std::vector<Part> solidMesh();

  /// Wireframe mesh creation function.
  /// @return A wireframe mesh representation.
  static std::vector<Part> wireframeMesh();

private:
};
}  // namespace tes::viewer::painter

#endif  // TES_VIEWER_PAINTER_CAPSULE_H
