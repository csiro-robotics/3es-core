
#ifndef TES_VIEWER_PAINTER_STAR_H
#define TES_VIEWER_PAINTER_STAR_H

#include "3es-viewer.h"

#include "3esshapepainter.h"

namespace tes::viewer::painter
{
/// Star painter.
class Star : public ShapePainter
{
public:
  /// Constructor.
  /// @param culler Bounds culler
  Star(std::shared_ptr<BoundsCuller> culler);

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

#endif  // TES_VIEWER_PAINTER_STAR_H
