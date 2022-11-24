#include "3esmeshshape.h"

#include "mesh/3esconverter.h"
#include "util/3esenum.h"

#include <3esconnection.h>
#include <3escolour.h>
#include <3esdebug.h>
#include <3eslog.h>
#include <3espacketreader.h>
#include <shapes/3esmeshshape.h>

#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Quaternion.h>

namespace tes::viewer::handler
{
TES_ENUM_FLAGS(MeshShape::Flag, unsigned);


MeshShape::MeshShape(std::shared_ptr<BoundsCuller> culler)
  : Message(SIdMeshShape, "mesh shape")
  , _culler(std::move(culler))
{
  _opaque_shader = std::make_shared<Magnum::Shaders::VertexColor3D>();
}


void MeshShape::initialise()
{}


void MeshShape::reset()
{}


void MeshShape::beginFrame(const FrameStamp &stamp)
{
  updateRenderAssets();
}


void MeshShape::endFrame(const FrameStamp &stamp)
{
  // Note: it would be ideal to do the render mesh creation here, but that happens on the background thread and we
  // can't create OpenGL resources from there. Instead, we do the work in beginFrame().
  // FIXME(KS): unfortunately this means there is a race condition where one frame ends, then a mesh may be updated
  // before being commited for rendering.
}


void MeshShape::draw(DrawPass pass, const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix)
{
  std::lock_guard guard(_shapes_mutex);

  auto &transients = _transients[_active_transients_index];
  for (auto &transient : transients)
  {
    // All this locking may prove very slow :S
    std::lock_guard guard2(transient->mutex);
    if (_culler->isVisible(transient->bounds_id) && transient->mesh)
    {
      _opaque_shader->setTransformationProjectionMatrix(projection_matrix * transient->transform)
        .draw(*transient->mesh);
    }
  }

  for (auto &[id, render_mesh] : _shapes)
  {
    std::lock_guard guard2(render_mesh->mutex);
    if (_culler->isVisible(render_mesh->bounds_id) && render_mesh->mesh)
    {
      _opaque_shader->setTransformationProjectionMatrix(projection_matrix * render_mesh->transform)
        .draw(*render_mesh->mesh);
    }
  }
}


void MeshShape::readMessage(PacketReader &reader)
{
  TES_ASSERT(reader.routingId() == routingId());
  ObjectAttributes attrs = {};
  bool ok = false;
  bool logged = false;
  switch (reader.messageId())
  {
  case OIdCreate:
    ok = handleCreate(reader);
    break;
  case OIdDestroy: {
    DestroyMessage msg;
    ok = msg.read(reader) && handleDestroy(msg, reader);
    break;
  }
  case OIdUpdate:
    ok = handleUpdate(reader);
    break;
  case OIdData:
    ok = handleData(reader);
    break;
  default:
    log::error(name(), " : unhandled shape message type: ", unsigned(reader.messageId()));
    logged = true;
    break;
  }

  if (!ok && !logged)
  {
    log::error(name(), " : failed to decode message type: ", unsigned(reader.messageId()));
  }
}


void MeshShape::serialise(Connection &out, ServerInfoMessage &info)
{
  info = _server_info;
  std::array<uint8_t, (1u << 16u) - 1> buffer;
  PacketWriter writer(buffer.data(), buffer.size());
  CreateMessage create = {};
  ObjectAttributes attrs = {};
}


Magnum::Matrix4 MeshShape::composeTransform(const ObjectAttributes &attrs) const
{
  return Magnum::Matrix4::translation(Magnum::Vector3(attrs.position[0], attrs.position[1], attrs.position[2])) *
         Magnum::Matrix4(Magnum::Quaternion(Magnum::Vector3(attrs.rotation[0], attrs.rotation[1], attrs.rotation[2]),
                                            attrs.rotation[3])
                           .toMatrix()) *
         Magnum::Matrix4::scaling(Magnum::Vector3(attrs.scale[0], attrs.scale[1], attrs.scale[2]));
}


void MeshShape::decomposeTransform(const Magnum::Matrix4 &transform, ObjectAttributes &attrs) const
{
  const auto position = transform[3].xyz();
  attrs.position[0] = position[0];
  attrs.position[1] = position[1];
  attrs.position[2] = position[2];
  const auto rotation = Magnum::Quaternion::fromMatrix(transform.rotation());
  attrs.rotation[0] = rotation.vector()[0];
  attrs.rotation[1] = rotation.vector()[1];
  attrs.rotation[2] = rotation.vector()[2];
  attrs.rotation[3] = rotation.scalar();
  attrs.scale[0] = transform[0].xyz().length();
  attrs.scale[1] = transform[1].xyz().length();
  attrs.scale[2] = transform[2].xyz().length();
}


bool MeshShape::handleCreate(PacketReader &reader)
{
  // Start by modifying the _shapes set
  auto shape = std::make_shared<tes::MeshShape>();
  if (!shape->readCreate(reader))
  {
    log::error("Error reading mesh create.");
    return false;
  }

  create(shape);
  return true;
}


bool MeshShape::handleUpdate(PacketReader &reader)
{
  uint32_t id = 0;
  reader.peek(reinterpret_cast<uint8_t *>(&id), sizeof(id));

  auto data = getData(Id(id));
  if (!data)
  {
    log::error("Invalid mesh shape id for update message: ", id);
    return false;
  }

  std::lock_guard guard(data->mutex);
  if (!data->shape->readUpdate(reader))
  {
    return false;
  }

  data->flags |= Flag::DirtyAttributes;
  return true;
}


bool MeshShape::handleDestroy(const DestroyMessage &msg, PacketReader &reader)
{
  auto data = getData(Id(msg.id));
  if (!data)
  {
    return false;
  }
  std::lock_guard guard(data->mutex);
  data->flags |= Flag::MarkForDeath;
  return true;
}


bool MeshShape::handleData(PacketReader &reader)
{
  uint32_t id = 0;
  reader.peek(reinterpret_cast<uint8_t *>(&id), sizeof(id));

  auto data = getData(Id(id));
  if (!data)
  {
    log::error("Invalid mesh shape id for data message: ", id);
    return false;
  }

  std::lock_guard guard(data->mutex);
  if (!data->shape->readData(reader))
  {
    return false;
  }

  data->flags |= Flag::DirtyMesh;
  return true;
}


std::shared_ptr<MeshShape::RenderMesh> MeshShape::create(std::shared_ptr<tes::MeshShape> shape)
{
  const Id id = shape->id();
  if (id.isTransient())
  {
    auto new_entry = std::make_shared<RenderMesh>();
    new_entry->shape = shape;
    new_entry->flags |= Flag::Pending;
    new_entry->flags &= ~Flag::MarkForDeath;
    // No need to lock until here.
    std::lock_guard guard(_shapes_mutex);
    _transients[1 - _active_transients_index].emplace_back(new_entry);
    return new_entry;
  }

  std::lock_guard guard(_shapes_mutex);
  // Search the map.
  auto search = _shapes.find(id);
  if (search != _shapes.end())
  {
    std::lock_guard guard2(search->second->mutex);
    search->second->shape = shape;
    search->second->flags |= Flag::Dirty;
    return search->second;
  }

  auto new_entry = std::make_shared<RenderMesh>();
  new_entry->shape = shape;
  new_entry->flags |= Flag::Pending;
  _shapes.emplace(id, new_entry);
  return new_entry;
}


std::shared_ptr<MeshShape::RenderMesh> MeshShape::getData(const Id &id)
{
  std::lock_guard guard(_shapes_mutex);
  if (id.isTransient())
  {
    // No need to lock until here.
    auto &transients = _transients[1 - _active_transients_index];
    if (!transients.empty())
    {
      return transients.back();
    }
    return {};
  }

  // Search the map.
  auto search = _shapes.find(id);
  if (search != _shapes.end())
  {
    return search->second;
  }

  return {};
}


void MeshShape::updateRenderAssets()
{
  std::lock_guard guard(_shapes_mutex);
  for (auto &&transient : _transients[_active_transients_index])
  {
    std::lock_guard guard2(transient->mutex);
    transient->shape = nullptr;
  }
  _transients[_active_transients_index].clear();
  _active_transients_index = 1 - _active_transients_index;
  for (auto &&transient : _transients[_active_transients_index])
  {
    std::lock_guard guard2(transient->mutex);
    transient->flags &= ~Flag::Pending;
    updateRenderResources(*transient);
  }

  for (auto iter = _shapes.begin(); iter != _shapes.end();)
  {
    auto data = iter->second;
    std::lock_guard guard2(data->mutex);
    if ((data->flags & Flag::MarkForDeath) != Flag::MarkForDeath)
    {
      if ((data->flags & (Flag::Pending | Flag::DirtyMesh)) != Flag::Zero)
      {
        updateRenderResources(*data);
      }
      else if ((data->flags & Flag::DirtyAttributes) != Flag::Zero)
      {
        data->transform = composeTransform(data->shape->attributes());
        _culler->update(data->bounds_id, data->cullBounds());
      }

      data->flags &= ~(Flag::Pending | Flag::Dirty);

      ++iter;
    }
    else
    {
      iter = _shapes.erase(iter);
    }
  }
}


void MeshShape::updateRenderResources(RenderMesh &render_mesh)
{
  if (render_mesh.shape)
  {
    mesh::ConvertOptions options = {};

    render_mesh.mesh = std::make_unique<Magnum::GL::Mesh>(
      mesh::convert(tes::MeshShape::Resource(*render_mesh.shape, 0), render_mesh.bounds, options));
    render_mesh.transform = composeTransform(render_mesh.shape->attributes());

    if (render_mesh.bounds_id == BoundsCuller::kInvalidId)
    {
      // TODO(KS): transform the bounds.
      render_mesh.bounds_id = _culler->allocate(render_mesh.cullBounds());
    }
    else
    {
      _culler->update(render_mesh.bounds_id, render_mesh.cullBounds());
    }
  }
}
}  // namespace tes::viewer::handler
