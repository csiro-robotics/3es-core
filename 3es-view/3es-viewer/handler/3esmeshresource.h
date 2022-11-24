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
#include <Magnum/Shaders/VertexColor.h>

#include <mutex>
#include <unordered_map>
#include <vector>

namespace tes::viewer::handler
{
class MeshResource : public Message
{
  struct Resource;

public:
  class ResourceReference
  {
  public:
    inline ResourceReference() = default;
    inline ResourceReference(std::shared_ptr<const tes::MeshResource> resource, std::shared_ptr<Magnum::GL::Mesh> mesh)
      : _resource(std::move(resource))
      , _mesh(std::move(mesh))
    {}
    inline ResourceReference(ResourceReference &&other) = default;
    ResourceReference(const ResourceReference &other) = delete;

    ResourceReference &operator=(ResourceReference &&other) = default;
    ResourceReference &operator=(const ResourceReference &other) = delete;

    ~ResourceReference() = default;

    inline bool isValid() const { return _resource != nullptr && _mesh != nullptr; }
    inline operator bool() const { return isValid(); }

    inline const std::shared_ptr<const tes::MeshResource> &resource() const { return _resource; }
    inline std::shared_ptr<Magnum::GL::Mesh> mesh() const { return _mesh; }

  private:
    std::shared_ptr<const tes::MeshResource> _resource = nullptr;
    std::shared_ptr<Magnum::GL::Mesh> _mesh = nullptr;
  };

  MeshResource();

  ResourceReference get(uint32_t id) const;

  void beginFrame(const FrameStamp &stamp) override;
  void endFrame(const FrameStamp &stamp) override;
  void draw(DrawPass pass, const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix) override;
  void readMessage(PacketReader &reader) override;
  void serialise(Connection &out, ServerInfoMessage &info) override;

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
    Bounds bounds = {};
    /// The current mesh resource data. This is what the main thread will render.
    std::shared_ptr<SimpleMesh> current;
    /// Pending mesh resource data. This will move to current on the next @c beginFrame() call.
    std::shared_ptr<SimpleMesh> pending;
    /// The current renderable mesh.
    std::shared_ptr<Magnum::GL::Mesh> mesh;
    ResourceFlag flags = ResourceFlag::Zero;
  };

  mutable std::mutex _resource_lock;
  std::unordered_map<uint32_t, Resource> _resources;
  std::unordered_map<uint32_t, Resource> _pending;
};


inline MeshResource::ResourceReference MeshResource::get(uint32_t id) const
{
  std::lock_guard guard(_resource_lock);
  auto search = _resources.find(id);
  if (search != _resources.end())
  {
    ResourceReference ref(search->second.current, search->second.mesh);
    return ref;
  }
  return ResourceReference();
}
}  // namespace tes::viewer::handler

#endif  // TES_VIEW_3ES_VIEWER_HANDLER_MESHRESOURCE_H
