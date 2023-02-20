
#ifndef TES_VIEW_HANDLER_MESH_SHAPE_H
#define TES_VIEW_HANDLER_MESH_SHAPE_H

#include <3esview/ViewConfig.h>

#include "Message.h"

#include <3esview/BoundsCuller.h>
#include <3esview/util/PendingActionQueue.h>

#include <3escore/shapes/Id.h>

#include <Magnum/GL/Mesh.h>
#include <Magnum/Shaders/VertexColor.h>

#include <iosfwd>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>

namespace tes
{
class MeshShape;
}

namespace tes::view::shaders
{
class Shader;
class ShaderLibrary;
}  // namespace tes::view::shaders

namespace tes::view::handler
{
/// The message handler for mesh shape messages and rendering.
///
/// This handles simple meshes of draw types covering triangles, lines and points. This is the
/// general case and specialised handlers exist for meshes with parts - @c MeshSet - and point
/// clouds - @c PointCloud - including points rendered using a voxel representation. Note these two
/// also rely in the @c Mesh handler which decoders mesh resource definitions.
class TES_VIEWER_API MeshShape : public Message
{
public:
  MeshShape(std::shared_ptr<BoundsCuller> culler,
            std::shared_ptr<shaders::ShaderLibrary> shader_library);

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
  /// @return The matrix transformation for the shape.
  virtual void decomposeTransform(const Magnum::Matrix4 &transform, ObjectAttributes &attrs) const;

protected:
  virtual bool handleCreate(PacketReader &reader);
  virtual bool handleUpdate(PacketReader &reader);
  virtual bool handleDestroy(const DestroyMessage &msg, PacketReader &reader);
  virtual bool handleData(PacketReader &reader);

  struct RenderMesh
  {
    BoundsId bounds_id = BoundsCuller::kInvalidId;
    Bounds bounds = {};
    std::shared_ptr<tes::MeshShape> shape;
    Magnum::Matrix4 transform = {};
    /// The mesh to render.
    /// @note Cannot be created on the background thread with OpenGL. Maybe with Vulkan.
    std::shared_ptr<Magnum::GL::Mesh> mesh;
    /// The shader used to draw this mesh.
    /// @todo Evaluate if collating rendering by shader provides any performance benefits.
    std::shared_ptr<shaders::Shader> shader;
  };

  using RenderMeshPtr = std::shared_ptr<RenderMesh>;
  using PendingQueue = util::PendingActionQueue<std::shared_ptr<tes::MeshShape>>;

  /// Create a @c RenderMesh entry for @p shape in @p _pending_shapes.
  /// @param shape The shape data to create for.
  RenderMeshPtr create(std::shared_ptr<tes::MeshShape> shape);

  /// Update the shape matching @p shape_id with the given @p update .
  /// @param shape_id The ID of the shape to update.
  /// @param update The update details.
  /// @return True if @p shape_id is valid for update.
  bool updateShape(uint32_t shape_id, const PendingQueue::Action::Update &update);

  /// Get the queued @c RenderMesh shape entry for the given ID.
  ///
  /// This is for the background thread to manage data messages for shapes which have yet to
  /// activate. As such it only searches the @c _pending_actions queue.
  ///
  /// This makes a number of assumptions.
  /// - @c _shapes_mutex must not be locked when calling this function or a deadlock will ensue.
  /// - If @p id is transient, then we fetch the last transient item from @c _pending_shapes.
  ///   Commited shapes cannot be retrieved.
  /// - Non transient shapes may be retrieved from either @p _pending_shapes (preferred), or
  ///   @c _shapes.
  ///
  /// This is only intended for use from @c handleData(), @c handleUpdate() or @c handleDestroy().
  ///
  /// @param id The ID of the shape to fetch.
  /// @return A pointer to the @c tes::MeshShape for @p id or null on failure.
  std::shared_ptr<tes::MeshShape> getQueuedRenderMesh(const Id &id);

  /// Create all the pending render assets. Must be called on the main thread ( @c prepareFrame() ).
  ///
  /// Main thread only, @c _shapes_mutex must be locked.
  void updateRenderAssets();
  /// Create or update the render resources for @p render_mesh.
  ///
  /// Main thread only, @c _shapes_mutex must be locked.
  ///
  /// @param render_mesh Mesh data to update.
  void updateRenderResources(RenderMesh &render_mesh);

  /// Update bounds for the given @p render_mesh. Assumes the transform is up to date.
  /// @param render_mesh Mesh data to update.
  void updateBounds(RenderMesh &render_mesh);

  /// Mutex locked whenever touching @c _shapes or @c _transients.
  mutable std::mutex _shapes_mutex;
  std::unordered_map<Id, RenderMeshPtr> _shapes;
  /// A buffer for items to be added to _shapes on the next @c prepareFrame() call.
  /// For details, see the large comment block in @c create().
  PendingQueue _pending_queue;
  /// List of IDs for items in @c _shapes which need their render assets updated.
  std::vector<Id> _needs_render_asset_list;
  /// Transient shapes. The last item is the most current which is returned when requesting a
  /// transient shape.
  std::vector<RenderMeshPtr> _transients;
  unsigned _active_transients_index = 0;
  std::shared_ptr<BoundsCuller> _culler;
  /// Garbage list populated on @c reset() from background thread so main thread can release on
  /// @c prepareFrame().
  std::vector<RenderMeshPtr> _garbage_list;
  std::shared_ptr<shaders::ShaderLibrary> _shader_library;
};
}  // namespace tes::view::handler

#endif  // TES_VIEW_HANDLER_MESH_SHAPE_H
