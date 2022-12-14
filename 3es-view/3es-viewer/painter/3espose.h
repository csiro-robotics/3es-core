
#ifndef TES_VIEWER_PAINTER_POSE_H
#define TES_VIEWER_PAINTER_POSE_H

#include "3es-viewer.h"

#include "3esshapepainter.h"

namespace tes::viewer::painter
{
/// Pose painter.
class TES_VIEWER_API Pose : public ShapePainter
{
public:
  /// Constructor.
  /// @param culler Bounds culler
  Pose(std::shared_ptr<BoundsCuller> culler, std::shared_ptr<shaders::ShaderLibrary> shaders);

  /// Solid mesh creation function.
  /// @return A solid (or transparent) mesh representation.
  static Magnum::GL::Mesh solidMesh();

  /// Wireframe mesh creation function.
  /// @return A wireframe mesh representation.
  static Magnum::GL::Mesh wireframeMesh();

private:
};
}  // namespace tes::viewer::painter

#endif  // TES_VIEWER_PAINTER_POSE_H
