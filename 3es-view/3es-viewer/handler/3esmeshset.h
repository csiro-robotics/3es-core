//
// Author: Kazys Stepanas
//
#ifndef TES_VIEWER_HANDLER_MESH_SET_H
#define TES_VIEWER_HANDLER_MESH_SET_H

#include "3es-viewer.h"

#include "3esmeshresource.h"

#include <shapes/3esmeshset.h>

#include <Magnum/Math/Color.h>
#include <Magnum/GL/Mesh.h>

namespace tes::viewer::handler
{
/// The message handler for mesh sets which reference and render @c MeshResource items.
class MeshSet : public Message
{
public:
  // enum class Flag : unsigned
  // {
  //   Zero = 0u,
  //   Pending = 1u << 0u,
  //   MarkForDeath = 1u << 1u,
  //   DirtyAttributes = 1u << 2u,
  //   DirtyMesh = 1u << 3u,

  //   Dirty = DirtyAttributes | DirtyMesh
  // };

  MeshSet(std::shared_ptr<BoundsCuller> culler, std::shared_ptr<MeshResource> resources);

  void initialise() override;
  void reset() override;
  void beginFrame(const FrameStamp &stamp) override;
  void endFrame(const FrameStamp &stamp) override;

  void draw(DrawPass pass, const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix) override;

  void readMessage(PacketReader &reader) override;
  void serialise(Connection &out, ServerInfoMessage &info) override;

  /// Compose the object transform from the given object attributes.
  /// @param attrs Object attributes as read from the message payload.
  /// @return The matrix transformation for the shape.
  virtual Magnum::Matrix4 composeTransform(const ObjectAttributes &attrs) const;
  /// Decompose the object transform to the given object attributes.
  /// @param transform The transformation matrix to decompose.
  /// @param attrs Object attributes to write to.
  virtual void decomposeTransform(const Magnum::Matrix4 &transform, ObjectAttributes &attrs) const;

  /// Compose the object transform from the given object attributes.
  /// @param tes_transform 3es transform object to compose from.
  /// @return The matrix transformation for the shape.
  virtual Magnum::Matrix4 composeTransform(const tes::Transform &tes_transform) const;
  /// Decompose the object transform to the given object attributes.
  /// @param transform The transformation matrix to decompose.
  /// @param tes_transform 3es transform to write to.
  virtual void decomposeTransform(const Magnum::Matrix4 &transform, tes::Transform &tes_transform) const;

protected:
  virtual bool handleCreate(PacketReader &reader);
  virtual bool handleUpdate(PacketReader &reader);
  virtual bool handleDestroy(const DestroyMessage &msg, PacketReader &reader);

private:
  // using RenderMeshPtr = std::shared_ptr<RenderMesh>;

  /// A drawable item, which is one part of a @c MeshSet .
  struct Drawable
  {
    /// Bounds id for this part.
    BoundsId bounds_id = util::kNullResource;
    /// Current bounds for this part. If @c DrawableFlag::DirtyAttributes is set, then this has yet to be updated to
    /// the @c BoundsCuller .
    Bounds bounds;
    uint32_t resource_id = 0;
    Magnum::Matrix4 transform;
    Magnum::Color4 colour;
    std::shared_ptr<tes::MeshSet> owner;
    /// Index of this part id in the @c owner parts.
    unsigned part_id = 0;
    /// State flags.
    DrawableFlag flags = DrawableFlag::Zero;
    std::shared_ptr<Magnum::GL::Mesh> mesh;
    // TODO:(KS): shader;
  };

  struct MeshItem
  {
    std::shared_ptr<tes::MeshSet> shape;
    DrawableFlag flags = DrawableFlag::Zero;
  };

  bool create(const std::shared_ptr<tes::MeshSet> &shape);
  void beginFrameForDrawable(Drawable &drawable);

  /// Mutex locked whenever touching @c _shapes or @c _transients.
  mutable std::mutex _mutex;
  std::shared_ptr<BoundsCuller> _culler;
  std::shared_ptr<MeshResource> _resources;
  std::vector<MeshResource::DrawItem> _draw_set;
  std::vector<Drawable> _drawables;
  std::vector<MeshItem> _transients;
  std::unordered_map<uint32_t, MeshItem> _shapes;
};
}  // namespace tes::viewer::handler

#endif  // TES_VIEWER_HANDLER_MESH_SET_H
