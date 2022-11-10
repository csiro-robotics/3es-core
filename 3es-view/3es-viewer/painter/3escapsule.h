#ifndef TES_VIEWER_PAINTER_CAPSULE_H
#define TES_VIEWER_PAINTER_CAPSULE_H

#include "3es-viewer.h"

#include "3esshapepainter.h"

#include <3esvector3.h>

namespace tes
{
class SimpleMesh;
}

namespace tes::viewer::painter
{
/// Capsule painter.
class Capsule : public ShapePainter
{
public:
  /// Constructor.
  /// @param culler Bounds culler
  Capsule(std::shared_ptr<BoundsCuller> culler);

  void reset() override;

  bool update(const Id &id, FrameNumber frame_number, const Magnum::Matrix4 &transform,
              const Magnum::Color4 &colour) override;
  bool remove(const Id &id, FrameNumber frame_number) override;

  /// Calculate bounds for a capsule shape.
  /// @param transform The shape transform to calculate with.
  /// @param[out] centre Bounds centre output.
  /// @param[out] half_extents Bounds half extents output.
  static void calculateBounds(const Magnum::Matrix4 &transform, Magnum::Vector3 &centre, Magnum::Vector3 &half_extents);

  void drawOpaque(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix) override;
  void drawTransparent(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix) override;

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
  util::ResourceListId Capsule::addShape(const ViewableWindow &view_window, Type type, const Magnum::Matrix4 &transform,
                                         const Magnum::Color4 &colour, const ParentId &parent_id) override;

private:
  static void buildEndCapSolid(SimpleMesh &mesh, bool bottomCap);

  std::array<std::unique_ptr<ShapeCache>, 2> *endCapCachesForType(Type type);

  static std::array<Magnum::Matrix4, 2> calcEndCapTransforms(const Magnum::Matrix4 &transform);

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
