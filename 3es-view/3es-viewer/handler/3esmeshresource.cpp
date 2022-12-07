//
// Author: Kazys Stepanas
//
#include "3esmeshresource.h"

#include "mesh/3esconverter.h"
#include "util/3esenum.h"

#include <3esconnection.h>
#include <3esmeshmessages.h>
#include <3eslog.h>

#include <Magnum/GL/Renderer.h>

namespace tes::viewer::handler
{
TES_ENUM_FLAGS(MeshResource::ResourceFlag, unsigned);

MeshResource::MeshResource()
  : Message(MtMesh, "mesh resource")
{}


void MeshResource::initialise()
{}


void MeshResource::reset()
{
  std::lock_guard guard(_resource_lock);
  for (auto &[id, resource] : _resources)
  {
    _garbage_list.emplace_back(resource.mesh);
    resource.mesh = nullptr;
  }
  for (auto &[id, resource] : _pending)
  {
    _garbage_list.emplace_back(resource.mesh);
    resource.mesh = nullptr;
  }
  _resources.clear();
  _pending.clear();
}


void MeshResource::beginFrame(const FrameStamp &stamp)
{
  _garbage_list.clear();
  // As we begin a frame, we need to commit resources.
  // For OpenGL this must be on beginFrame() as this is the main thread.
  // With Vulkan we could do it in endFrame().

  // Move resources from the pending list. This may replace existing items, such as when we redefine an existing mesh.
  for (auto &[id, resource] : _pending)
  {
    _resources[id] = resource;
  }
  _pending.clear();
  updateResources();
}


void MeshResource::endFrame(const FrameStamp &stamp)
{}


void MeshResource::draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params)
{
  // This handler does not drawing, it just holds resources.
}


void MeshResource::readMessage(PacketReader &reader)
{
  uint32_t mesh_id = 0;
  reader.peek(reinterpret_cast<uint8_t *>(&mesh_id), sizeof(mesh_id));

  std::lock_guard guard(_resource_lock);

  bool found = false;
  auto search = _pending.find(mesh_id);
  if (search == _pending.end())
  {
    search = _resources.find(mesh_id);
    found = search != _resources.end();
  }
  else
  {
    found = true;
  }

  switch (reader.messageId())
  {
  case MmtDestroy:
    if (found)
    {
      search->second.flags |= ResourceFlag::MarkForDeath;
    }
    break;
  case MmtCreate: {
    Resource resource = {};
    resource.pending = std::make_shared<SimpleMesh>(mesh_id);
    if (resource.pending->readCreate(reader))
    {
      _pending.emplace(mesh_id, resource);
    }
    else
    {
      log::error("Error reading mesh resource create: ", mesh_id);
    }
  }
  break;
  case MmtVertex:
  case MmtIndex:
  case MmtVertexColour:
  case MmtNormal:
  case MmtUv:
  case MmtSetMaterial:
    if (found && search->second.pending)
    {
      if (!search->second.pending->readTransfer(reader.messageId(), reader))
      {
        log::error("Error reading mesh transfer message for ", mesh_id, " : ", reader.messageId());
      }
    }
    break;
  case MmtRedefine:
    if (found)
    {
      search->second.pending = std::shared_ptr<SimpleMesh>(search->second.current->clone());
      search->second.flags &= ~ResourceFlag::Ready;
    }
    break;
  case MmtFinalise:
    if (found)
    {
      search->second.flags |= ResourceFlag::Ready;
    }
    break;
  default:
    log::error("Invalid mesh message id: ", reader.messageId());
    break;
  }
}


void MeshResource::serialise(Connection &out, ServerInfoMessage &info)
{
  std::lock_guard guard(_resource_lock);

  for (auto &[id, resource] : _resources)
  {
    if (resource.current)
    {
      out.referenceResource(resource.current.get());
      if (out.updateTransfers(0) == -1)
      {
        log::error("Error serialising mesh resource: ", resource.current->id());
      }
    }
  }
}


unsigned MeshResource::draw(const Magnum::Matrix4 &projection_matrix, const std::vector<DrawItem> &drawables,
                            DrawFlag flags)
{
  std::lock_guard guard(_resource_lock);

  if ((flags & DrawFlag::TwoSided) != DrawFlag::Zero)
  {
    Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::FaceCulling);
  }

  if ((flags & DrawFlag::Transparent) != DrawFlag::Zero)
  {
    Magnum::GL::Renderer::setBlendFunction(Magnum::GL::Renderer::BlendFunction::SourceAlpha,
                                           Magnum::GL::Renderer::BlendFunction::OneMinusSourceAlpha);
  }

  unsigned drawn = 0;
  for (const auto &item : drawables)
  {
    const auto search = _resources.find(item.resource_id);
    if (search != _resources.end() && search->second.mesh)
    {
      _opaque_shader.setTransformationProjectionMatrix(projection_matrix * item.model_matrix)
        .draw(*search->second.mesh);
      ++drawn;
    }
  }

  if ((flags & DrawFlag::Transparent) != DrawFlag::Zero)
  {
    Magnum::GL::Renderer::setBlendFunction(Magnum::GL::Renderer::BlendFunction::One,
                                           Magnum::GL::Renderer::BlendFunction::Zero);
  }

  if ((flags & DrawFlag::TwoSided) != DrawFlag::Zero)
  {
    Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::FaceCulling);
  }

  return drawn;
}


void MeshResource::updateResources()
{
  std::lock_guard guard(_resource_lock);
  mesh::ConvertOptions options = {};
  for (auto &[id, resource] : _resources)
  {
    // Note: this is a very inefficient way to manage large meshes with changing sub-sections as we duplicate and
    // recreate the entire mesh. Better would be to only touch the changed sections, but that can wait.
    if ((resource.flags & ResourceFlag::Ready) != ResourceFlag::Zero)
    {
      if (resource.pending)
      {
        resource.current = resource.pending;
        resource.mesh = std::make_shared<Magnum::GL::Mesh>(mesh::convert(*resource.current, resource.bounds, options));
        // Update to spherical bounds.
        resource.bounds.convertToSpherical();
      }
      resource.flags &= ~ResourceFlag::Ready;
    }
  }
}


}  // namespace tes::viewer::handler
