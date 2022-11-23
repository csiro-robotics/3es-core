
#ifndef TES_VIEWER_PAINTER_CYLINDER_H
#define TES_VIEWER_PAINTER_CYLINDER_H

#include "3es-viewer.h"

#include "3esshapepainter.h"

namespace tes::viewer::painter
{
/// Cylinder painter.
class Cylinder : public ShapePainter
{
public:
  /// Constructor.
  /// @param culler Bounds culler
  Cylinder(std::shared_ptr<BoundsCuller> culler);

  /// Calculate bounds for a cylinder shape.
  /// @param transform The shape transform to calculate with.
  /// @param[out] bounds Bounds output.
  static void calculateBounds(const Magnum::Matrix4 &transform, Bounds &bounds);

  /// Solid mesh creation function.
  /// @return A solid (or transparent) mesh representation.
  static Magnum::GL::Mesh solidMesh();

  /// Wireframe mesh creation function.
  /// @return A wireframe mesh representation.
  static Magnum::GL::Mesh wireframeMesh();

private:
};
}  // namespace tes::viewer::painter

#endif  // TES_VIEWER_PAINTER_CYLINDER_H
