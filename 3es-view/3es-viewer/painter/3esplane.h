
#ifndef TES_VIEWER_PAINTER_PLANE_H
#define TES_VIEWER_PAINTER_PLANE_H

#include "3es-viewer.h"

#include "3esshapepainter.h"

namespace tes::viewer::painter
{
/// Plane painter.
class Plane : public ShapePainter
{
public:
  /// Constructor.
  /// @param culler Bounds culler
  Plane(std::shared_ptr<BoundsCuller> culler);

  /// Solid mesh creation function.
  /// @return A solid (or transparent) mesh representation.
  static Magnum::GL::Mesh solidMesh();

  /// Wireframe mesh creation function.
  /// @return A wireframe mesh representation.
  static Magnum::GL::Mesh wireframeMesh();

  void drawOpaque(unsigned render_mark, const Magnum::Matrix4 &projection_matrix) override;
  void drawTransparent(unsigned render_mark, const Magnum::Matrix4 &projection_matrix) override;

private:
};
}  // namespace tes::viewer::painter

#endif  // TES_VIEWER_PAINTER_PLANE_H
