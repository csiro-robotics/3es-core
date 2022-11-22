#include "3esmeshshape.h"

#include "mesh/3esconverter.h"

#include <3esconnection.h>
#include <3escolour.h>
#include <3esdebug.h>
#include <3eslog.h>
#include <3espacketreader.h>
#include <shapes/3esmeshshape.h>

#include <Magnum/Color4.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Quaternion.h>

namespace tes::viewer::handler
{
inline MeshShape::Flag operator|(MeshShape::Flag a, MeshShape::Flag b)
{
  return MeshShape::Flag(unsigned(a) | unsigned(b));
}

inline MeshShape::Flag operator&(MeshShape::Flag a, MeshShape::Flag b)
{
  return MeshShape::Flag(unsigned(a) & unsigned(b));
}

inline MeshShape::Flag &operator|=(MeshShape::Flag &a, MeshShape::Flag b)
{
  a = a | b;
  return a;
}

inline MeshShape::Flag &operator&=(MeshShape::Flag &a, MeshShape::Flag b)
{
  a = a & b;
  return a;
}

inline MeshShape::Flag operator~(MeshShape::Flag a)
{
  return MeshShape::Flag(~unsigned(a));
}


MeshShape::MeshShape(std::shared_ptr<BoundsCuller> culler)
  : Message(SIdMeshShape, "mesh shape")
  , _culler(std::move(culler))
{}


void MeshShape::initialise()
{}


void MeshShape::reset()
{}


void MeshShape::beginFrame(const FrameStamp &stamp)
{}


void MeshShape::endFrame(const FrameStamp &stamp)
{
  std::lock_guard guard(_shapes_mutex);
  for (auto &&transient : _transients[_active_transients_index])
  {
    std::lock_guard guard2(transient->mutex);
    transient->mesh = nullptr;
  }
  _transients[_active_transients_index].clear();
  _active_transients_index = 1 - _active_transients_index;
  for (auto &&transient : _transients[_active_transients_index])
  {
    transient->flags &= ~Flag::Pending;
    updateRenderResources(transient);
  }

  for (auto iter = _mesh_shapes.begin(); iter != _mesh_shapes.end();)
  {
    auto data = iter->second;
    std::lock_guard guard2(data->mutex);
    if ((data->flags & Flag::MarkForDeath) != Flag::MarkForDeath)
    {
      if ((data->flags & Flag::Pending) != Flag::Pending)
      {
        updateRenderResources(transient);
      }
      if ((data->flags & Flag::Dirty) != Flag::Dirty)
      {
        updateRenderResources(transient);
        data->flags |= Flag::Dirty;
      }

      data->flags &= ~(Flag::Pending | Flag::Dirty);

      ++iter;
    }
    else
    {
      iter = _mesh_shapes.erase(iter);
    }
  }
}


void MeshShape::draw(DrawPass pass, const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix)
{
  switch (pass)
  {
  case DrawPass::Opaque:
    break;
  case DrawPass::Transparent:
    break;
  default:
    break;
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
  case OIdUpdate: {
    UpdateMessage msg;
    ok = msg.read(reader, attrs) && handleUpdate(msg, attrs, reader);
    break;
  }
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
  auto mesh = std::make_shared<tes::MeshShape>();
  if (!mesh->readCreate(reader))
  {
    log::error("Error reading mesh create.");
    return false;
  }

  create(mesh);
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
  if (!data->mesh->readData(packet))
  {
    return false;
  }
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
  if (!data->mesh->readData(reader))
  {
    return false;
  }

  mesh_data->flags |= Flag::Dirty;
  return true;
}


std::shared_ptr<MeshShape::RenderMesh> MeshShape::create(std::shared_ptr<tes::MeshShape> mesh);
{
  const Id id = mesh->id();
  if (id.isTransient())
  {
    auto new_entry = std::make_shared<RenderMesh>();
    new_entry->mesh = mesh;
    new_entry->flags |= Flag::Pending;
    new_entry->flags &= ~Flag::MarkForDeath;
    // No need to lock until here.
    std::lock_guard guard(_shapes_mutex);
    _transient.emplace_back(new_entry);
    return new_entry;
  }

  std::lock_guard guard(_shapes_mutex);
  // Search the map.
  auto search = _shapes.find(id);
  if (search != _shapes.end())
  {
    search->second->mesh = mesh;
    new_entry->flags |= Flag::Dirty;
    return search->second;
  }

  auto new_entry = std::make_shared<RenderMesh>();
  new_entry->mesh = mesh;
  new_entry->flags |= Flag::Pending;
  _shapes.emplace(id, new_entry);
  return new_entry;
}


std::shared_ptr<MeshShape::RenderMesh> MeshShape::getEntry(const Id &id)
{
  std::lock_guard guard(_shapes_mutex);
  if (id.isTransient())
  {
    // No need to lock until here.
    if (!_transient.empty())
    {
      return _transient.back();
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


void MeshShape::updateRenderResources(RenderMesh &render_mesh)
{
  if (render_mesh->mesh)
  {
    Bounds bounds;
    // TODO(KS): calculate normals if requested.
    render_mesh.render_mesh = mesh::convert(tes::MeshShape::Resource(*render_mesh->mesh), bounds);
    if (render_mesh.bounds_id == BoundsCuller::kInvalidId)
    {
      render_mesh.bounds_id = _culler->allocate(bounds);
    }
    else
    {
      _culler->update(render_mesh.bounds_id, bounds);
    }
  }
}
}  // namespace tes::viewer::handler
