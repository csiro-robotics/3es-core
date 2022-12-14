//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_3ES_VIEWER_HANDLER_MESHRESOURCE_H
#define TES_VIEW_3ES_VIEWER_HANDLER_MESHRESOURCE_H

#include "3es-viewer.h"

#include "3esboundsculler.h"
#include "3esmessage.h"

#include <shapes/3essimplemesh.h>

#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Shaders/VertexColor.h>

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace tes::viewer::shaders
{
class Shader;
class ShaderLibrary;
}  // namespace tes::viewer::shaders

namespace tes::viewer::handler
{
class TES_VIEWER_API MeshResource : public Message
{
  struct Resource;

public:
  class TES_VIEWER_API ResourceReference
  {
  public:
    inline ResourceReference() = default;
    inline ResourceReference(const Bounds &bounds, std::shared_ptr<const tes::MeshResource> resource,
                             std::shared_ptr<Magnum::GL::Mesh> mesh)
      : _bounds(bounds)
      , _resource(std::move(resource))
      , _mesh(std::move(mesh))
    {}
    inline ResourceReference(ResourceReference &&other) = default;
    ResourceReference(const ResourceReference &other) = delete;

    ResourceReference &operator=(ResourceReference &&other) = default;
    ResourceReference &operator=(const ResourceReference &other) = delete;

    ~ResourceReference() = default;

    inline bool isValid() const { return _resource != nullptr && _mesh != nullptr; }
    inline operator bool() const { return isValid(); }

    /// Get the mesh bounds.
    ///
    /// The bounds are spherical in nature rather than an AABB, with the half extends being equal on all axes. This
    /// obviates the need to traverse the mesh in order to calculate tight bounds for each instance of this mesh.
    ///
    /// @return The mesh bounds.
    inline const Bounds &bounds() const { return _bounds; }
    inline const std::shared_ptr<const tes::MeshResource> &resource() const { return _resource; }
    inline std::shared_ptr<Magnum::GL::Mesh> mesh() const { return _mesh; }

  private:
    Bounds _bounds = {};
    std::shared_ptr<const tes::MeshResource> _resource = nullptr;
    std::shared_ptr<Magnum::GL::Mesh> _mesh = nullptr;
  };

  /// A drawable item for @c draw() calls.
  struct TES_VIEWER_API DrawItem
  {
    /// The mesh resource ID to draw.
    uint32_t resource_id = 0;
    /// Model matrix to draw with.
    Magnum::Matrix4 model_matrix;
    /// Mesh tint to apply (NYI).
    Magnum::Color4 tint = { 1, 1, 1, 1 };
  };

  enum class DrawFlag : unsigned
  {
    Zero = 0,
    Wireframe = OFWire,
    Transparent = OFTransparent,
    TwoSided = OFTwoSided
  };

  MeshResource(std::shared_ptr<shaders::ShaderLibrary> shader_library);

  ResourceReference get(uint32_t id) const;

  void initialise() override;
  void reset() override;

  void beginFrame(const FrameStamp &stamp) override;
  void endFrame(const FrameStamp &stamp) override;
  void draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params) override;
  void readMessage(PacketReader &reader) override;
  void serialise(Connection &out, ServerInfoMessage &info) override;

  /// Draw any number of mesh resource. Does not consider culling (cull before calling).
  ///
  /// Draws each resource identified in @p drawables.
  ///
  /// @param projection_matrix Current projection matrix.
  /// @param drawables Defines what to draw.
  /// @return The number of resources successfully resolved and drawn from @p drawables.
  unsigned draw(const Magnum::Matrix4 &projection_matrix, const std::vector<DrawItem> &drawables,
                DrawFlag flags = DrawFlag::Zero);

  enum class ResourceFlag : unsigned
  {
    Zero = 0u,
    Ready = (1u << 0u),
    MarkForDeath = (1u << 1u)
  };

private:
  void updateResources();

  /// A resource entry.
  struct Resource
  {
    /// Mesh bounds. The semantics are spherical rather than defining an AABB since we don't know how it will be
    /// transformed and don't want to traverse the mesh just to get tight instance bounds.
    Bounds bounds = {};
    /// The current mesh resource data. This is what the main thread will render.
    std::shared_ptr<SimpleMesh> current;
    /// Pending mesh resource data. This will move to current on the next @c beginFrame() call.
    std::shared_ptr<SimpleMesh> pending;
    /// The current renderable mesh.
    std::shared_ptr<Magnum::GL::Mesh> mesh;
    ResourceFlag flags = ResourceFlag::Zero;
    std::shared_ptr<shaders::Shader> shader;
  };

  mutable std::mutex _resource_lock;
  std::unordered_map<uint32_t, Resource> _resources;
  std::unordered_map<uint32_t, Resource> _pending;
  /// Garbage list populated on @c reset() from background thread so main thread can release on @c beginFrame().
  std::vector<std::shared_ptr<Magnum::GL::Mesh>> _garbage_list;
  std::shared_ptr<shaders::ShaderLibrary> _shader_library;
};


TES_ENUM_FLAGS(MeshResource::DrawFlag, unsigned);

inline MeshResource::ResourceReference MeshResource::get(uint32_t id) const
{
  std::lock_guard guard(_resource_lock);
  auto search = _resources.find(id);
  if (search != _resources.end())
  {
    ResourceReference ref(search->second.bounds, search->second.current, search->second.mesh);
    return ref;
  }
  return ResourceReference();
}
}  // namespace tes::viewer::handler

#endif  // TES_VIEW_3ES_VIEWER_HANDLER_MESHRESOURCE_H
