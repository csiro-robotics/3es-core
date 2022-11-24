#include "3esmeshshape.h"

#include "mesh/3esconverter.h"
#include "util/3esenum.h"

#include <3esconnection.h>
#include <3escolour.h>
#include <3esdebug.h>
#include <3eslog.h>
#include <3espacketreader.h>
#include <shapes/3esmeshshape.h>

#include <Magnum/GL/Renderer.h>
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
  // Note(KS): there would be a race condition here if a mesh shape is allowed to update it's data after it's been
  // created and a frame boundary occurs. However, that is not allowed. We do, though, have to deal with a destroy/
  // recreate case.
}


void MeshShape::draw(DrawPass pass, const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix)
{
  std::lock_guard guard(_shapes_mutex);

  const auto draw_mesh = [this, &projection_matrix](RenderMesh &render_mesh) {
    // All this locking may prove very slow :S
    std::lock_guard guard2(render_mesh.mutex);
    if (_culler->isVisible(render_mesh.bounds_id) && render_mesh.mesh)
    {
      // TODO(KS): Move default draw scales to shared settings.
      switch (render_mesh.shape->drawType())
      {
      case DtPoints: {
        const float draw_scale = (render_mesh.shape->drawScale() > 0) ? render_mesh.shape->drawScale() : 8.0f;
        Magnum::GL::Renderer::setPointSize(draw_scale);
        break;
      }
      case DtLines: {
        const float draw_scale = (render_mesh.shape->drawScale() > 0) ? render_mesh.shape->drawScale() : 2.0f;
        Magnum::GL::Renderer::setLineWidth(draw_scale);
        break;
      }
      default:
        break;
      }
      _opaque_shader->setTransformationProjectionMatrix(projection_matrix * render_mesh.transform)
        .draw(*render_mesh.mesh);
    }
  };

  for (auto &transient : _transients)
  {
    draw_mesh(*transient);
  }

  for (auto &[id, render_mesh] : _shapes)
  {
    draw_mesh(*render_mesh);
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

  const auto check = [](int error) {
    if (error)
    {
      log::error("Error code serialising mesh: ", error);
    }
  };

  for (auto &transient : _transients)
  {
    check(out.create(*transient->shape));
  }

  for (auto &[id, render_mesh] : _shapes)
  {
    check(out.create(*render_mesh->shape));
  }
}


Magnum::Matrix4 MeshShape::composeTransform(const ObjectAttributes &attrs) const
{
  return Message::composeTransform(attrs);
}


void MeshShape::decomposeTransform(const Magnum::Matrix4 &transform, ObjectAttributes &attrs) const
{
  Message::decomposeTransform(transform, attrs);
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

  auto data = getRenderMesh(Id(id));
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

  data->transform = composeTransform(data->shape->attributes());
  data->flags |= Flag::DirtyAttributes;
  return true;
}


bool MeshShape::handleDestroy(const DestroyMessage &msg, PacketReader &reader)
{
  auto data = getRenderMesh(Id(msg.id));
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

  auto data = getRenderMesh(Id(id));
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


MeshShape::RenderMeshPtr MeshShape::create(std::shared_ptr<tes::MeshShape> shape)
{
  const Id id = shape->id();

  // Note: this comment is referenced from the header documentation for _pending_shapes.
  // We have an existing shape. That is valid, but poses a potential race condition. Consider the following event
  // streams.
  //
  // | Data Thread    | Render Thread |
  // | ------------   | ------------- |
  // | create mesh 1  |               |
  // | update frame 0 |               |
  // |                | begin frame 0 |
  // | destroy 1      |               |
  // | create 2       |               |
  // | update frame 1 |               |
  // | end frame 0    |               |
  // | destroy 2 *    |               |
  // | create 2  *    |               |
  // |                | begin frame 1 |
  // | update frame 2 |               |
  // | end frame 2    |               |
  //
  // Frame 0 proceeds fine. On frame 1, the render thread marks frame 1 as being complete, but calls
  // handler::Message::endFrame(0) from the data thread. On the next render thread update, it will call
  // handler::Message::beginFrame(1), which will display mesh 2.
  //
  // Before we start frame 1 and display mesh 2, the data thread already routes a message to destroy mesh 2 and
  // recreate it. So the RenderMesh::shape data will change before the render thread can create
  // RenderMesh::render_thread from beginFrame(1). By the time that is called, we are displaying the new state of
  // mesh 2 a frame early.
  //
  // Now we can safely assume we only need to buffer for one frame ahead - either the render thread will show the
  // frame or not, but we can't show the wrong data on a frame.
  //
  // Options:
  // - Keep a second shape in RenderMesh for this exact case. We still instantiate the same memory, we just buffer it
  //   differently.
  // - Buffer pending additions to _shapes in a different list, to be added during the beginFrame() call, like a
  //   command queue.
  //
  // For this reason we always add shapes to _pending_shapes rather than to _transients or _shapes directly.
  auto new_entry = std::make_shared<RenderMesh>();
  new_entry->shape = shape;
  new_entry->flags |= Flag::Pending;
  new_entry->flags &= ~Flag::MarkForDeath;
  // No need to lock until here.
  std::lock_guard guard(_shapes_mutex);
  _pending_shapes.emplace_back(id, new_entry);
  return new_entry;
}


MeshShape::RenderMeshPtr MeshShape::getRenderMesh(const Id &id)
{
  std::lock_guard guard(_shapes_mutex);
  if (id.isTransient())
  {
    // For a transient shape, we may only fetch the last transient item from _pending_shapes. _transients is already
    // commited and cannot be changed.
    for (auto iter = _pending_shapes.rbegin(); iter != _pending_shapes.rend(); ++iter)
    {
      if (iter->first.isTransient())
      {
        return iter->second;
      }
    }
    return {};
  }

  // Search pending items first.
  // We expect this list to always be small-ish.
  for (auto iter = _pending_shapes.begin(); iter != _pending_shapes.end(); ++iter)
  {
    // Ignore category in comparison.
    if (id.id() == iter->first.id())
    {
      return iter->second;
    }
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

  // Clear previous transients.
  _transients.clear();

  // Remove expired shapes and update transforms for persistent shapes.
  for (auto iter = _shapes.begin(); iter != _shapes.end();)
  {
    auto data = iter->second;
    std::lock_guard guard2(data->mutex);
    if ((data->flags & Flag::MarkForDeath) != Flag::MarkForDeath)
    {
      if ((data->flags & Flag::DirtyAttributes) != Flag::Zero)
      {
        _culler->update(data->bounds_id, data->cullBounds());
      }

      data->flags &= ~(Flag::DirtyAttributes);

      ++iter;
    }
    else
    {
      iter = _shapes.erase(iter);
    }
  }

  // Process and commit pending assets.
  for (auto &[id, render_mesh] : _pending_shapes)
  {
    std::lock_guard guard2(render_mesh->mutex);
    if ((render_mesh->flags & Flag::MarkForDeath) != Flag::Zero)
    {
      // Already deleted. Skip this item.
      continue;
    }

    updateRenderResources(*render_mesh);
    render_mesh->flags &= ~(Flag::Pending | Flag::Dirty);
    if (id.isTransient())
    {
      _transients.emplace_back(render_mesh);
    }
    else
    {
      _shapes.emplace(id, render_mesh);
    }
  }
  _pending_shapes.clear();
}


void MeshShape::updateRenderResources(RenderMesh &render_mesh)
{
  if (render_mesh.shape)
  {
    mesh::ConvertOptions options = {};
    options.auto_colour = true;

    render_mesh.mesh = std::make_unique<Magnum::GL::Mesh>(
      mesh::convert(tes::MeshShape::Resource(*render_mesh.shape, 0), render_mesh.bounds, options));
    render_mesh.transform = composeTransform(render_mesh.shape->attributes());

    if (render_mesh.bounds_id == BoundsCuller::kInvalidId)
    {
      render_mesh.bounds_id = _culler->allocate(render_mesh.cullBounds());
    }
    else
    {
      _culler->update(render_mesh.bounds_id, render_mesh.cullBounds());
    }
  }
}
}  // namespace tes::viewer::handler
