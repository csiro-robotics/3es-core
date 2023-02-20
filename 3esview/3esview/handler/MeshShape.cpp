#include "MeshShape.h"

#include <3esview/mesh/Converter.h>
#include <3esview/shaders/Shader.h>
#include <3esview/shaders/ShaderLibrary.h>

#include <3escore/Connection.h>
#include <3escore/Colour.h>
#include <3escore/Debug.h>
#include <3escore/Log.h>
#include <3escore/PacketReader.h>
#include <3escore/shapes/MeshShape.h>

#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Quaternion.h>

namespace tes::view::handler
{
MeshShape::MeshShape(std::shared_ptr<BoundsCuller> culler,
                     std::shared_ptr<shaders::ShaderLibrary> shader_library)
  : Message(SIdMeshShape, "mesh shape")
  , _culler(std::move(culler))
  , _shader_library(std::move(shader_library))
{}


void MeshShape::initialise()
{}


void MeshShape::reset()
{
  std::lock_guard guard(_shapes_mutex);
  _garbage_list = _transients;
  for (auto &[id, mesh] : _shapes)
  {
    _garbage_list.emplace_back(mesh);
  }
  _shapes.clear();
  _pending_queue.clear();
  _transients.clear();
}


void MeshShape::prepareFrame(const FrameStamp &stamp)
{
  (void)stamp;
  std::lock_guard guard(_shapes_mutex);
  // Release garbage assets.
  _garbage_list.clear();
  updateRenderAssets();
}


void MeshShape::endFrame(const FrameStamp &stamp)
{
  (void)stamp;
  // Note: it would be ideal to do the render mesh creation here, but that happens on the background
  // thread and we can't create OpenGL resources from there. Instead, we do the work in
  // prepareFrame().
  std::lock_guard guard(_shapes_mutex);

  // Move transients to the garbage list for the main thread to clean up.
  std::copy(_transients.begin(), _transients.end(), std::back_inserter(_garbage_list));
  _transients.clear();

  _pending_queue.mark(stamp.frame_number);
  // Effect pending actions.
  for (const auto &action : _pending_queue.view(stamp.frame_number))
  {
    switch (action.action)
    {
    default:
    case util::ActionKind::None:
      break;
    case util::ActionKind::Create:
      if (!Id(action.shape_id).isTransient())
      {
        _shapes[Id(action.shape_id)] = create(action.create.shape);
        _needs_render_asset_list.emplace_back(action.shape_id);
      }
      else
      {
        _transients.emplace_back(create(action.create.shape));
      }
      break;
    case util::ActionKind::Update:
      updateShape(action.shape_id, action.update);
      break;
    case util::ActionKind::Destroy: {
      const auto search = _shapes.find(Id(action.shape_id));
      if (search != _shapes.end())
      {
        // Add to garbage list for the main thread to clean up.
        _garbage_list.emplace_back(search->second);
        _shapes.erase(search);
      }
    }
    break;
    }
  }
}

void MeshShape::draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params)
{
  (void)pass;
  (void)stamp;
  std::lock_guard guard(_shapes_mutex);

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

  const auto draw_mesh = [this, &params](RenderMesh &render_mesh) {
    if (_culler->isVisible(render_mesh.bounds_id) && render_mesh.mesh && render_mesh.shader)
    {
      render_mesh.shader->setDrawScale(render_mesh.shape->drawScale())
        .setModelMatrix(render_mesh.transform)
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
  (void)info;
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

  PendingQueue::Action action(util::ActionKind::Create);
  action.shape_id = shape->id();
  action.create.shape = shape;
  _pending_queue.emplace_back(action);

  return true;
}


bool MeshShape::handleUpdate(PacketReader &reader)
{
  uint32_t id = 0;
  UpdateMessage update = {};
  ObjectAttributesd attrs = {};

  if (!update.read(reader, attrs))
  {
    return false;
  }

  if (update.id == 0)
  {
    // Can't update transient shapes.
    return false;
  }

  PendingQueue::Action action(util::ActionKind::Update);
  action.shape_id = update.id;
  action.update.flags = update.flags;
  action.update.position = Vector3d(attrs.position);
  action.update.rotation = Quaterniond(attrs.rotation);
  action.update.scale = Vector3d(attrs.scale);
  action.update.colour = Colour(attrs.colour);
  _pending_queue.emplace_back(action);
  return true;
}


bool MeshShape::handleDestroy(const DestroyMessage &msg, PacketReader &reader)
{
  (void)reader;
  PendingQueue::Action action(util::ActionKind::Destroy);
  action.shape_id = msg.id;
  _pending_queue.emplace_back(action);
  return true;
}


bool MeshShape::handleData(PacketReader &reader)
{
  uint32_t id = 0;
  reader.peek(reinterpret_cast<uint8_t *>(&id), sizeof(id));

  std::lock_guard guard(_shapes_mutex);
  auto shape = getQueuedRenderMesh(Id(id));
  if (!shape)
  {
    log::error("Invalid mesh shape id for data message: ", id);
    return false;
  }

  return shape->readData(reader);
}


MeshShape::RenderMeshPtr MeshShape::create(std::shared_ptr<tes::MeshShape> shape)
{
  const Id id = shape->id();

  // Note: this comment is referenced from the header documentation for _pending_shapes.
  // We have an existing shape. That is valid, but poses a potential race condition. Consider the
  // following event streams.
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
  // Frame 0 proceeds fine. On frame 1, the render thread marks frame 1 as being complete, but
  // calls handler::Message::endFrame(0) from the data thread. On the next render thread update,
  // it will call handler::Message::prepareFrame(1), which will display mesh 2.
  //
  // Before we start frame 1 and display mesh 2, the data thread already routes a message to
  // destroy mesh 2 and recreate it. So the RenderMesh::shape data will change before the render
  // thread can create RenderMesh::render_thread from prepareFrame(1). By the time that is called,
  // we are displaying the new state of mesh 2 a frame early.
  //
  // Now we can safely assume we only need to buffer for one frame ahead - either the render
  // thread will show the frame or not, but we can't show the wrong data on a frame.
  //
  // Options:
  // - Keep a second shape in RenderMesh for this exact case. We still instantiate the same
  // memory, we just buffer it
  //   differently.
  // - Buffer pending additions to _shapes in a different list, to be added during the
  // prepareFrame() call, like a
  //   command queue.
  //
  // For this reason we always add shapes to _pending_shapes rather than to _transients or _shapes
  // directly.
  auto new_entry = std::make_shared<RenderMesh>();
  new_entry->shape = shape;
  return new_entry;
}


bool MeshShape::updateShape(uint32_t shape_id, const PendingQueue::Action::Update &update)
{
  if (shape_id == 0)
  {
    // Can't update transient objects.
    return false;
  }

  const auto search = _shapes.find(shape_id);
  if (search == _shapes.end())
  {
    return false;
  }

  const auto &[id, render_mesh] = *search;

  const bool update_all = (update.flags & UFUpdateMode) != 0u;
  const bool update_position = (update.flags & UFPosition) != 0u || update_all;
  const bool update_rotation = (update.flags & UFRotation) != 0u || update_all;
  const bool update_scale = (update.flags & UFScale) != 0u || update_all;
  const bool update_colour = (update.flags & UFColour) != 0u || update_all;

  if (update_position)
  {
    render_mesh->shape->setPosition(update.position);
  }
  if (update_rotation)
  {
    render_mesh->shape->setRotation(update.rotation);
  }
  if (update_scale)
  {
    render_mesh->shape->setScale(update.scale);
  }
  if (update_colour)
  {
    render_mesh->shape->setColour(update.colour);
  }

  // Adjust the transform and bounds if required.
  if (update_position || update_rotation || update_scale)
  {
    render_mesh->transform = composeTransform(render_mesh->shape->attributes());
    updateBounds(*render_mesh);
  }

  return true;
}


std::shared_ptr<tes::MeshShape> MeshShape::getQueuedRenderMesh(const Id &id)
{
  // Make sure we use a const view and don't discard things from the action queue.
  std::shared_ptr<tes::MeshShape> shape;
  for (auto &action : _pending_queue.viewConst())
  {
    if (action.action == util::ActionKind::Create)
    {
      if (action.shape_id == id.id())
      {
        shape = action.create.shape;
        // Keep looking. We are after the last matching entry. For non-transient we generally
        // expect it to be the first match. For transients, we must find the last one.
      }
    }
  }

  return shape;
}


void MeshShape::updateRenderAssets()
{
  // Remove expired shapes and update transforms for persistent shapes.
  for (const auto &id : _needs_render_asset_list)
  {
    const auto search = _shapes.find(id);
    if (search != _shapes.end())
    {
      updateRenderResources(*search->second);
    }
  }
  _needs_render_asset_list.clear();

  for (const auto &render_mesh_ptr : _transients)
  {
    updateRenderResources(*render_mesh_ptr);
  }
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
    updateBounds(render_mesh);
    render_mesh.shader = _shader_library->lookupForDrawType(render_mesh.shape->drawType());
  }
}


void MeshShape::updateBounds(RenderMesh &render_mesh)
{
  if (render_mesh.bounds_id == BoundsCuller::kInvalidId)
  {
    render_mesh.bounds_id =
      _culler->allocate(render_mesh.bounds.calculateLooseBounds(render_mesh.transform));
  }
  else
  {
    _culler->update(render_mesh.bounds_id,
                    render_mesh.bounds.calculateLooseBounds(render_mesh.transform));
  }
}
}  // namespace tes::view::handler
