//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_HANDLER_MESH_SET_H
#define TES_VIEW_HANDLER_MESH_SET_H

#include <3esview/ViewConfig.h>

#include "MeshResource.h"

#include <3escore/shapes/MeshSet.h>
#include <3esview/util/PendingAction.h>

#include <Magnum/Math/Color.h>
#include <Magnum/Math/Quaternion.h>
#include <Magnum/GL/Mesh.h>

namespace tes::view::handler
{
/// The message handler for mesh sets which reference and render @c MeshResource items.
class TES_VIEWER_API MeshSet : public Message
{
public:
  using PendingAction = util::PendingAction<std::shared_ptr<tes::MeshSet>>;

  MeshSet(std::shared_ptr<BoundsCuller> culler, std::shared_ptr<MeshResource> resources);

  void initialise() override;
  void reset() override;
  void prepareFrame(const FrameStamp &stamp) override;
  void endFrame(const FrameStamp &stamp) override;

  void draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params) override;

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
  virtual void decomposeTransform(const Magnum::Matrix4 &transform,
                                  tes::Transform &tes_transform) const;

protected:
  virtual bool handleCreate(PacketReader &reader);
  virtual bool handleUpdate(PacketReader &reader);
  virtual bool handleDestroy(const DestroyMessage &msg, PacketReader &reader);

private:
  /// A drawable item, which is one part of a @c MeshSet .
  struct Drawable
  {
    /// Bounds id for this part.
    BoundsId bounds_id = util::kNullResource;
    /// Bounding box of the @c resource_id . This is not the active bounds. That is stored
    /// in the culler.
    Bounds resource_bounds;
    uint32_t resource_id = 0;
    Magnum::Matrix4 transform;
    Magnum::Color4 colour;
    std::shared_ptr<tes::MeshSet> owner;
    /// Index of this part id in the @c owner parts.
    unsigned part_id = 0;
    /// State flags.
    DrawableFlag flags = DrawableFlag::Zero;
  };

  struct MeshItem
  {
    std::shared_ptr<tes::MeshSet> shape;
    DrawableFlag flags = DrawableFlag::Zero;
  };

  bool createDrawables(const std::shared_ptr<tes::MeshSet> &shape);
  bool calculateBounds(Drawable &drawable) const;
  bool updateShape(uint32_t shape_id, const PendingAction::Update &update);
  bool destroyShape(uint32_t shape_id, const PendingAction::Destroy &destroy);

  /// Mutex locked whenever touching @c _shapes or @c _transients.
  mutable std::mutex _mutex;
  std::shared_ptr<BoundsCuller> _culler;
  std::shared_ptr<MeshResource> _resources;
  /// Used to marshal draw requests for the @c _resources. We use two to allow a single pass to
  /// collect single [0] and two sided [1] drawing into separate sets.
  std::array<std::vector<MeshResource::DrawItem>, 2> _draw_sets;
  /// Active drawables, persistent and transient. A single @c MeshSet can have multiple drawables.
  /// Using a vector won't scale well. Perhaps a multi_map?
  std::vector<Drawable> _drawables;
  /// Active transient shapes.
  std::vector<std::shared_ptr<tes::MeshSet>> _transients;
  /// Active persistent shapes, by ID.
  std::unordered_map<uint32_t, std::shared_ptr<tes::MeshSet>> _shapes;

  /// Shapes currently being created. We may currently be receiving data messages for these shapes.
  ///
  /// Persistent and transient shapes. Migrated to @c _pending_shapes on @c endFrame() .
  ///
  /// It's possible for items to be marked for death here before migration (though that may be a
  /// server side bug as the shape is never visualised).
  std::vector<std::shared_ptr<tes::MeshSet>> _creation_list;

  /// Pending actions, in order they arrived.
  std::vector<PendingAction> _pending_actions;

  /// The last frame we handled.
  FrameNumber _last_frame = 0;
};
}  // namespace tes::view::handler

#endif  // TES_VIEW_HANDLER_MESH_SET_H
