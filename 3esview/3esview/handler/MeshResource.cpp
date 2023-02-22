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
#include <3escore/TriGeom.h>

#include <Magnum/GL/Renderer.h>

namespace tes::view::handler
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
  const std::lock_guard guard(_resource_lock);
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


void MeshResource::prepareFrame(const FrameStamp &stamp)
{
  (void)stamp;
  {
    const std::lock_guard guard(_resource_lock);
    _garbage_list.clear();
    // As we begin a frame, we need to commit resources.
    // For OpenGL this must be on prepareFrame() as this is the main thread.
    // With Vulkan we could do it in endFrame().

    // Move resources from the pending list. This may replace existing items, such as when we
    // redefine an existing mesh.
    for (auto iter = _pending.begin(); iter != _pending.end();)
    {
      auto &[id, resource] = *iter;
      if (resource.marked)
      {
        _resources[id] = resource;
        iter = _pending.erase(iter);
      }
      else
      {
        ++iter;
      }
    }
  }
  updateResources();
}


void MeshResource::endFrame(const FrameStamp &stamp)
{
  (void)stamp;
  const std::lock_guard guard(_resource_lock);
  // Mark pending items to be migrated.
  for (auto &[id, resource] : _pending)
  {
    resource.marked = true;
  }
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

  const std::lock_guard guard(_resource_lock);

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
      // We can also get multiple redefines after having rendered at least once. In this case, both
      // current and pending will be valid. However, the Ready flag will only be set the first time,
      // so this becomes our cloning condition. Note: this cloning can become very expensive.
      if (search->second.current &&
          (search->second.flags & ResourceFlag::Ready) == ResourceFlag::Ready)
      {
        search->second.pending =
          std::dynamic_pointer_cast<SimpleMesh>(search->second.current->clone());
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
      resource->setVertexCount(msg.vertex_count);
      resource->setIndexCount(msg.index_count);
      resource->setDrawType(static_cast<DrawType>(msg.draw_type));
      const Transform transform =
        Transform(Vector3d(attributes.position), Quaterniond(attributes.rotation),
                  Vector3d(attributes.scale), msg.flags & McfDoublePrecision);

      resource->setTransform(transform);
      resource->setTint(attributes.colour);
    }
    break;
  case MmtFinalise:
    if (found)
    {
      MeshFinaliseMessage msg = {};
      if (!msg.read(reader))
      {
        log::error("Error reading mesh finalisation message: ", mesh_id);
        break;
      }

      if (msg.flags & (MffCalculateNormals))
      {
        calculateNormals(*search->second.pending, true);
      }

      if (msg.flags & (MffColourByX | MffColourByY | MffColourByZ))
      {
        int axis = 0;
        axis = (msg.flags & MffColourByY) ? 1 : axis;
        axis = (msg.flags & MffColourByZ) ? 2 : axis;
        colourByAxis(*search->second.pending, axis);
      }

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
  const std::lock_guard guard(_resource_lock);

  for (auto &[id, resource] : _resources)
  {
    if (resource.current)
    {
      out.referenceResource(Ptr<const tes::Resource>(resource.current));
      if (out.updateTransfers(0) == -1)
      {
        log::error("Error serialising mesh resource: ", resource.current->id());
      }
    }
  }
}


unsigned MeshResource::draw(const DrawParams &params, const std::vector<DrawItem> &drawables,
                            DrawFlag flags)
{
  const std::lock_guard guard(_resource_lock);

  if ((flags & DrawFlag::TwoSided) != DrawFlag::Zero)
  {
    Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::FaceCulling);
  }

  if ((flags & DrawFlag::Transparent) != DrawFlag::Zero)
  {
    Magnum::GL::Renderer::setBlendFunction(
      Magnum::GL::Renderer::BlendFunction::SourceAlpha,
      Magnum::GL::Renderer::BlendFunction::OneMinusSourceAlpha);
  }

  // Update the known shader matrices.
  const auto update_shader_matrices = [&params](std::shared_ptr<shaders::Shader> &&shader) {
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
        ->setDrawScale(search->second.current->drawScale())  //
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
  const std::lock_guard guard(_resource_lock);
  const mesh::ConvertOptions options = {};
  for (auto &[id, resource] : _resources)
  {
    // Note: this is a very inefficient way to manage large meshes with changing sub-sections as we
    // duplicate and recreate the entire mesh. Better would be to only touch the changed sections,
    // but that can wait.
    if ((resource.flags & ResourceFlag::Ready) != ResourceFlag::Zero)
    {
      if (resource.pending)
      {
        resource.current = resource.pending;
        resource.mesh = std::make_shared<Magnum::GL::Mesh>(
          mesh::convert(*resource.current, resource.bounds, options));
        // Update to spherical bounds.
        resource.bounds.convertToSpherical();
        resource.shader =
          _shader_library->lookupForDrawType(static_cast<DrawType>(resource.current->drawType(0)));
      }
      resource.flags &= ~ResourceFlag::Ready;
    }
  }
}


void MeshResource::calculateNormals(SimpleMesh &mesh, bool force)
{
  if (!force && mesh.rawNormals() != nullptr)
  {
    return;
  }

  if (mesh.drawType() != DtTriangles)
  {
    return;
  }

  const auto *vertices = mesh.rawVertices();
  const auto *indices = mesh.rawIndices();

  if (!vertices || !indices)
  {
    return;
  }

  std::vector<Vector3f> normals(mesh.vertexCount());

  // Loop the triangles
  const auto index_count = mesh.indexCount();
  std::array<Vector3f, 3> tri;
  for (unsigned i = 0; i < index_count; i += 3)
  {
    tri[0] = vertices[indices[i + 0]];
    tri[1] = vertices[indices[i + 1]];
    tri[2] = vertices[indices[i + 2]];

    // Sum the normal from this trinagle into the respective triangle normals.
    const auto normal = trigeom::normal(tri);
    normals[indices[i + 0]] += normal;
    normals[indices[i + 1]] += normal;
    normals[indices[i + 2]] += normal;
  }

  // Normalise the results.
  for (auto &normal : normals)
  {
    normal.normalise();
  }

  // Write the results.
  mesh.setNormals(0, normals.data(), normals.size());
}


void MeshResource::colourByAxis(SimpleMesh &mesh, int axis)
{
  if (mesh.rawColours() != nullptr)
  {
    return;
  }

  // Ensure axis is in range.
  axis = std::max(0, std::min(axis, 2));

  // Calculate extents.
  const auto *vertices = mesh.rawVertices();
  const unsigned vertex_count = mesh.vertexCount();

  if (!vertex_count)
  {
    // No vertices.
    return;
  }

  // Seed min/max
  float min_value = vertices[0][axis];
  float max_value = vertices[0][axis];
  for (unsigned i = 1; i < vertex_count; ++i)
  {
    min_value = std::min(vertices[i][axis], min_value);
    max_value = std::max(vertices[i][axis], max_value);
  }

  // Set the colours.
  const Colour colour_from(128, 255, 0);
  const Colour colour_to(120, 0, 255);
  const float range_inv = (max_value != min_value) ? 1.0f / max_value - min_value : 0.0f;

  for (unsigned i = 1; i < vertex_count; ++i)
  {
    const float factor = (vertices[i][axis] - min_value) * range_inv;
    mesh.setColour(i, Colour::lerp(colour_from, colour_to, factor).colour32());
  }
}
}  // namespace tes::view::handler
