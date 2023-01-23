
#ifndef TES_VIEWER_HANDLER_MESH_SHAPE_H
#define TES_VIEWER_HANDLER_MESH_SHAPE_H

#include "3es-viewer.h"

#include "3esboundsculler.h"
#include "3esmessage.h"

#include <shapes/3esid.h>

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

namespace tes::viewer::shaders
{
class Shader;
class ShaderLibrary;
}  // namespace tes::viewer::shaders

namespace tes::viewer::handler
{
/// The message handler for mesh shape messages and rendering.
///
/// This handles simple meshes of draw types covering triangles, lines and points. This is the general case and
/// specialised handlers exist for meshes with parts - @c MeshSet - and point clouds - @c PointCloud - including
/// points rendered using a voxel representation. Note these two also rely in the @c Mesh handler which decoders
/// mesh resource definitions.
class TES_VIEWER_API MeshShape : public Message
{
public:
  using Flag = DrawableFlag;
  MeshShape(std::shared_ptr<BoundsCuller> culler, std::shared_ptr<shaders::ShaderLibrary> shader_library);

  void initialise() override;
  void reset() override;
  void beginFrame(const FrameStamp &stamp) override;
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
    Flag flags = Flag::Zero;
    /// The mesh to render.
    /// @note Cannot be created on the background thread with OpenGL. Maybe with Vulkan.
    std::shared_ptr<Magnum::GL::Mesh> mesh;
    /// The shader used to draw this mesh.
    /// @todo Evaluate if collating rendering by shader provides any performance benefits.
    std::shared_ptr<shaders::Shader> shader;
    std::mutex mutex;

    /// Calculate bounds used for rendering.
    /// @return Culling bounds
    inline Bounds cullBounds() const
    {
      // The accurate approach would be to recalculate the bounds with the transform applied to each vertex.
      // That could be inefficient for moving meshes with many vertices. The simple option is to make the
      // bounds pseudo spherical and just translate them.
      const auto centre = bounds.centre() + transform[3].xyz();
      auto half_extents = bounds.halfExtents();
      half_extents.x() = half_extents.y() = half_extents.z() =
        std::max(half_extents.x(), std::max(half_extents.y(), half_extents.z()));
      return Bounds::fromCentreHalfExtents(centre, half_extents);
    }
  };

  using RenderMeshPtr = std::shared_ptr<RenderMesh>;

  /// Create a @c RenderMesh entry for @p shape in @p _pending_shapes.
  /// @param shape The shape data to create for.
  RenderMeshPtr create(std::shared_ptr<tes::MeshShape> shape);

  /// Get the @c RenderMesh shape entry for the given ID.
  ///
  /// This makes a number of assumptions.
  /// - @c _shapes_mutex must not be locked when calling this function or a deadlock will ensue.
  /// - If @p id is transient, then we fetch the last transient item from @c _pending_shapes. Commited transients
  ///   cannot be retrieved.
  /// - Non transient shapes may be retrieved from either @p _pending_shapes (preferred), or @c _shapes.
  ///
  /// This is only intended for use from @c handleData(), @c handleUpdate() or @c handleDestroy().
  ///
  /// @param id The ID of the shape to fetch.
  /// @return A pointer to the @c RenderMesh for @p id or null on failure.
  RenderMeshPtr getRenderMesh(const Id &id);

  /// Create all the pending render assets. Must be called on the main thread ( @c beginFrame() ).
  void updateRenderAssets();
  /// Create or update the render resources for @p render_mesh.
  /// @param render_mesh Mesh data to update.
  void updateRenderResources(RenderMesh &render_mesh);

  /// Mutex locked whenever touching @c _shapes or @c _transients.
  mutable std::mutex _shapes_mutex;
  std::unordered_map<Id, RenderMeshPtr> _shapes;
  /// A buffer for items to be added to _shapes on the next @c beginFrame() call.
  /// For details, see the large comment block in @c create().
  std::vector<std::pair<Id, RenderMeshPtr>> _pending_shapes;
  /// Transient shapes. The last item is the most current which is returned when requesting a transient shape.
  std::vector<RenderMeshPtr> _transients;
  unsigned _active_transients_index = 0;
  std::shared_ptr<BoundsCuller> _culler;
  /// Garbage list populated on @c reset() from background thread so main thread can release on @c beginFrame().
  std::vector<RenderMeshPtr> _garbage_list;
  std::shared_ptr<shaders::ShaderLibrary> _shader_library;
};
}  // namespace tes::viewer::handler

#endif  // TES_VIEWER_HANDLER_MESH_SHAPE_H
