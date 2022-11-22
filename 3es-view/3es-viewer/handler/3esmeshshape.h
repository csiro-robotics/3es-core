
#ifndef TES_VIEWER_HANDLER_MESH_SHAPE_H
#define TES_VIEWER_HANDLER_MESH_SHAPE_H

#include "3es-viewer.h"

#include "3esboundsculler.h"
#include "3esmessage.h"

#include <Magnum/GL/Mesh.h>

#include <iosfwd>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace tes
{
class MeshShape;
}

namespace tes::viewer::handler
{
/// The message handler for mesh shape messages and rendering.
///
/// This handles simple meshes of draw types covering triangles, lines and points. This is the general case and
/// specialised handlers exist for meshes with parts - @c MeshSet - and point clouds - @c PointCloud - including
/// points rendered using a voxel representation. Note these two also rely in the @c Mesh handler which decoders
/// mesh resource definitions.
class MeshShape : public Message
{
public:
  using ObjectAttributes = tes::ObjectAttributes<float>;

  MeshShape(std::shared_ptr<BoundsCuller> culler);

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
  /// @return The matrix transformation for the shape.
  virtual void decomposeTransform(const Magnum::Matrix4 &transform, ObjectAttributes &attrs) const;

protected:
  virtual bool handleCreate(PacketReader &reader);
  virtual bool handleUpdate(PacketReader &reader);
  virtual bool handleDestroy(const DestroyMessage &msg, PacketReader &reader);
  virtual bool handleData(PacketReader &reader);

private:
  enum class Flag : unsigned
  {
    Zero = 0u,
    Pending = 1u << 0u,
    MarkForDeath = 1u << 1u,
    DirtyAttributes = 1u << 2u,
    Dirty = 1u << 3u,
  };

  struct RenderMesh
  {
    BoundsId bounds_id = ;
    std::shared_ptr<tes::MeshShape> mesh;
    Flag flags = Flag::Zero;
    Magnum::GL::Mesh render_mesh;
    std::mutex mutex;
  };

  std::shared_ptr<RenderMesh> create(std::shared_ptr<tes::MeshShape> mesh);
  std::shared_ptr<RenderMesh> getData(const Id &id);
  /// Create or update the render resources for @p render_mesh.
  /// @param render_mesh Mesh data to update.
  void updateRenderResources(RenderMesh &render_mesh);

  mutable std::mutex _shapes_mutex;
  std::unordered_map<Id, std::shared_ptr<RenderMesh>> _shapes;
  /// Transient shapes. The last item is the most current which is returned when requesting a transient shape.
  std::array<std::vector<std::shared_ptr<RenderMesh>>, 2> _transients;
  unsigned _active_transients_index = 0;
  std::shared_ptr<BoundsCuller> _culler;
};
}  // namespace tes::viewer::handler

#endif  // TES_VIEWER_HANDLER_MESH_SHAPE_H
