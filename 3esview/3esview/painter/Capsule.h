#ifndef TES_VIEWER_PAINTER_CAPSULE_H
#define TES_VIEWER_PAINTER_CAPSULE_H

#include <3esview/ViewConfig.h>

#include "ShapePainter.h"

#include <3escore/Vector3.h>

namespace tes
{
class SimpleMesh;
}

namespace tes::viewer::painter
{
/// Capsule painter.
class TES_VIEWER_API Capsule : public ShapePainter
{
public:
  /// Constructor.
  /// @param culler Bounds culler
  Capsule(std::shared_ptr<BoundsCuller> culler, std::shared_ptr<shaders::ShaderLibrary> shaders);

  void reset() override;

  bool update(const Id &id, const Magnum::Matrix4 &transform, const Magnum::Color4 &colour) override;
  bool remove(const Id &id) override;

  /// Calculate bounds for a capsule shape.
  /// @param transform The shape transform to calculate with.
  /// @param[out] bounds Bounds output.
  static void calculateBounds(const Magnum::Matrix4 &transform, Bounds &bounds);

  void drawOpaque(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix,
                  const Magnum::Matrix4 &view_matrix) override;
  void drawTransparent(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix,
                       const Magnum::Matrix4 &view_matrix) override;

  void commit() override;

  /// Solid mesh creation function to generate the cyliindrical part.
  /// @return A solid (or transparent) mesh representation.
  static Magnum::GL::Mesh solidMeshCylinder();

  /// Wireframe mesh creation function to generate the cyliindrical part.
  /// @return A wireframe mesh representation.
  static Magnum::GL::Mesh wireframeMeshCylinder();

  /// Solid mesh creation function to generate the top end cap part.
  /// @return A solid (or transparent) mesh representation.
  static Magnum::GL::Mesh solidMeshCapTop();

  /// Solid mesh creation function to generate the bottom end cap part.
  /// @return A solid (or transparent) mesh representation.
  static Magnum::GL::Mesh solidMeshCapBottom();

  /// Wireframe mesh creation function to generate the top end cap part.
  /// @return A wireframe mesh representation.
  static Magnum::GL::Mesh wireframeMeshCap();

protected:
  util::ResourceListId Capsule::addShape(const Id &shape_id, Type type, const Magnum::Matrix4 &transform,
                                         const Magnum::Color4 &colour, bool hidden, const ParentId &parent_id,
                                         unsigned *child_index) override;

private:
  static void buildEndCapSolid(SimpleMesh &mesh, bool bottomCap);

  std::array<std::unique_ptr<ShapeCache>, 2> *endCapCachesForType(Type type);

  /// Transform modifier function for end cap transforms.
  ///
  /// This normalises the Z scale to match the X/Y scale, so we have a uniform spherical end cap, but converts the
  /// Z scale to an axial translation.
  ///
  /// @param[in,out] transform The transform to modify.
  /// @param positive True to apply a positive axial translation, false for negative. This yields the two different end
  /// caps.
  static void endCapTransformModifier(Magnum::Matrix4 &transform, bool positive);

  // We have additional shape caches which draw the separate parts of the capsule.
  // The cylinder can be scaled, but the end caps need to be translated by the Z
  // scale, then scaled uniformly by X (expecting scale X = Y).
  // The base class caches are used for the cylinder parts.
  std::array<std::unique_ptr<ShapeCache>, 2> _solid_end_caps;
  std::array<std::unique_ptr<ShapeCache>, 2> _wireframe_end_caps;
  std::array<std::unique_ptr<ShapeCache>, 2> _transparent_end_caps;

  static constexpr float kDefaultRadius = 1.0f;
  static constexpr float kDefaultHeight = 1.0f;
  static const Vector3f kDefaultAxis;
};
}  // namespace tes::viewer::painter

#endif  // TES_VIEWER_PAINTER_CAPSULE_H
