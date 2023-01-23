//
// Author: Kazys Stepanas
//
#include "MeshResource.h"

#include <3esview/mesh/Converter.h>
#include <3esview/shaders/PointGeom.h>
#include <3esview/shaders/Shader.h>
#include <3esview/shaders/ShaderLibrary.h>
#include <3esview/util/Enum.h>

#include <3escore/Connection.h>
#include <3escore/Log.h>
#include <3escore/MeshMessages.h>

#include <Magnum/GL/Renderer.h>

namespace tes::viewer::handler
{
TES_ENUM_FLAGS(MeshResource::ResourceFlag, unsigned);

MeshResource::MeshResource(std::shared_ptr<shaders::ShaderLibrary> shader_library)
  : Message(MtMesh, "mesh resource")
  , _shader_library(std::move(shader_library))
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
  (void)stamp;
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
{
  (void)stamp;
}


void MeshResource::draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params)
{
  // This handler does not drawing, it just holds resources.
  (void)pass;
  (void)stamp;
  (void)params;
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
      // Note(KS): we can get a redefine message before we've ever rendered the item.
      // In this case, current will be null, so we just modify the pending item.
      // We can also get multiple redefines after having rendered at least once. In this case, both current and pending
      // will be valid. However, the Ready flag will only be set the first time, so this becomes our cloning condition.
      // Note: this cloning can become very expensive.
      if (search->second.current && (search->second.flags & ResourceFlag::Ready) == ResourceFlag::Ready)
      {
        search->second.pending = std::shared_ptr<SimpleMesh>(search->second.current->clone());
      }
      search->second.flags &= ~ResourceFlag::Ready;
      MeshRedefineMessage msg = {};
      ObjectAttributesd attributes;
      if (!msg.read(reader, attributes))
      {
        log::error("Error reading mesh redefine message: ", mesh_id);
        break;
      }

      if (!search->second.pending)
      {
        log::error("Error no resource created yet for mesh: ", mesh_id);
        break;
      }

      auto resource = search->second.pending;
      resource->setVertexCount(msg.vertexCount);
      resource->setIndexCount(msg.indexCount);
      resource->setDrawType(DrawType(msg.drawType));
      Transform transform = Transform(Vector3d(attributes.position), Quaterniond(attributes.rotation),
                                      Vector3d(attributes.scale), msg.flags & McfDoublePrecision);

      resource->setTransform(transform);
      resource->setTint(attributes.colour);
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
  (void)info;
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


unsigned MeshResource::draw(const DrawParams &params, const std::vector<DrawItem> &drawables, DrawFlag flags)
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

  // Update the known shader matrices.
  const auto update_shader_matrices = [&params](std::shared_ptr<shaders::Shader> &shader) {
    if (shader)
    {
      shader->setProjectionMatrix(params.projection_matrix)
        .setViewMatrix(params.view_matrix)
        .setClipPlanes(params.camera.clip_near, params.camera.clip_far)
        .setViewportSize(params.view_size);
    }
  };
  update_shader_matrices(_shader_library->lookupForDrawType(DtPoints));
  update_shader_matrices(_shader_library->lookupForDrawType(DtLines));
  update_shader_matrices(_shader_library->lookupForDrawType(DtTriangles));
  update_shader_matrices(_shader_library->lookupForDrawType(DtVoxels));

  unsigned drawn = 0;
  for (const auto &item : drawables)
  {
    const auto search = _resources.find(item.resource_id);
    if (search != _resources.end() && search->second.mesh && search->second.shader)
    {
      search->second.shader
        ->  //
        setDrawScale(0)
        .setModelMatrix(item.model_matrix)
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
        resource.shader = _shader_library->lookupForDrawType(DrawType(resource.current->drawType(0)));
      }
      resource.flags &= ~ResourceFlag::Ready;
    }
  }
}
}  // namespace tes::viewer::handler
