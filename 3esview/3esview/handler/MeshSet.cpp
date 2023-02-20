//
// Author: Kazys Stepanas
//
#include "MeshSet.h"

#include "MeshResource.h"

#include <3esview/MagnumColour.h>

#include <3escore/Connection.h>
#include <3escore/Log.h>
#include <3escore/PacketReader.h>
#include <3escore/shapes/MeshSet.h>

namespace tes::view::handler
{
MeshSet::MeshSet(std::shared_ptr<BoundsCuller> culler, std::shared_ptr<MeshResource> resources)
  : Message(SIdMeshSet, "mesh set")
  , _culler(std::move(culler))
  , _resources(std::move(resources))
{}


void MeshSet::initialise()
{}


void MeshSet::reset()
{
  const std::lock_guard guard(_mutex);
  for (auto &drawable : _drawables)
  {
    _culler->release(drawable.bounds_id);
  }
  _drawables.clear();
  _draw_sets[0].clear();
  _draw_sets[1].clear();
  _transients.clear();
  _shapes.clear();
}


void MeshSet::prepareFrame(const FrameStamp &stamp)
{
  (void)stamp;
  const std::lock_guard guard(_mutex);

  // Update meshes and bounds.
  // @note: MeshResource handler has to prepareFrame() first so it's resources are available.
  for (size_t i = 0; i < _drawables.size(); ++i)
  {
    auto &drawable = _drawables[i];
    // Handle drawables marked for death (transients).
    if ((drawable.flags & DrawableFlag::MarkForDeath) == DrawableFlag::MarkForDeath)
    {
      // Remove this drawable by swapping with the last drawable.
      std::swap(drawable, _drawables.back());
      _drawables.resize(_drawables.size() - 1);
      --i;  // Decrement i as we've moved a new item into this index.
      continue;
    }

    // TODO(KS): performance evaluation of managing drawable transforms and bounds in this way.
    // Recalculate the transform.
    const auto transform = composeTransform(drawable.owner->attributes()) *
                           composeTransform(drawable.owner->partTransform(drawable.part_id));

    // Create bounds if required.
    if (drawable.bounds_id == BoundsCuller::kInvalidId)
    {
      calculateBounds(drawable);
    }
    // Update bounds if changed.
    else if (transform != drawable.transform)
    {
      drawable.transform = transform;
      _culler->update(drawable.bounds_id, drawable.resource_bounds.calculateLooseBounds(transform));
    }
    // TODO(KS): colour calculation is suspect. Is straight multiplication correct?
    const auto colour = drawable.owner->colour() * drawable.owner->partColour(drawable.part_id);
    drawable.colour = tes::view::convert(colour);

    // Mark transient drawables for death next frame.
    drawable.flags |=
      (drawable.owner->isTransient()) ? DrawableFlag::MarkForDeath : DrawableFlag::Zero;
  }
}


void MeshSet::endFrame(const FrameStamp &stamp)
{
  (void)stamp;
  const std::lock_guard guard(_mutex);
  // Clear the existing transients. We can do that off thread as we aren't releasing any render
  // resources.
  _transients.clear();
  _pending_actions.mark(stamp.frame_number);

  // Handle the pending actions. Order is preserved as the actions are intermingled in the queue.
  for (auto &action : _pending_actions.view(stamp.frame_number))
  {
    switch (action.action)
    {
    default:
    case PendingQueue::ActionKind::None:
      break;
    case PendingQueue::ActionKind::Create:
      createDrawables(action.create.shape);
      break;
    case PendingQueue::ActionKind::Update:
      if (action.shape_id)  // Only consider persistent IDs.
      {
        updateShape(action.shape_id, action.update);
      }
      break;
    case PendingQueue::ActionKind::Destroy:
      if (action.shape_id)  // Only consider persistent IDs.
      {
        destroyShape(action.shape_id, action.destroy);
      }
      break;
    }
  }
}


void MeshSet::draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params)
{
  (void)stamp;
  const std::lock_guard guard(_mutex);
  _draw_sets[0].clear();
  _draw_sets[1].clear();
  for (const auto &drawable : _drawables)
  {
    if (drawable.bounds_id == BoundsCuller::kInvalidId)
    {
      continue;
    }

    if (drawable.owner->transparent() != (pass == DrawPass::Transparent))
    {
      continue;
    }

    if (_culler->isVisible(drawable.bounds_id))
    {
      const unsigned set_idx = (!drawable.owner->twoSided()) ? 0 : 1;
      _draw_sets[set_idx].push_back({ drawable.resource_id, drawable.transform, drawable.colour });
    }
  }

  const MeshResource::DrawFlag flags = (pass == DrawPass::Transparent) ?
                                         MeshResource::DrawFlag::Transparent :
                                         MeshResource::DrawFlag::Zero;
  if (!_draw_sets[0].empty())
  {
    _resources->draw(params, _draw_sets[0], flags);
  }

  if (!_draw_sets[1].empty())
  {
    _resources->draw(params, _draw_sets[1], flags | MeshResource::DrawFlag::TwoSided);
  }
}


void MeshSet::readMessage(PacketReader &reader)
{
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
  default:
    log::error(name(),
               " : unhandled shape message type: ", static_cast<unsigned>(reader.messageId()));
    logged = true;
    break;
  }

  if (!ok && !logged)
  {
    log::error(name(),
               " : failed to decode message type: ", static_cast<unsigned>(reader.messageId()));
  }
}


void MeshSet::serialise(Connection &out, ServerInfoMessage &info)
{
  (void)info;
  const std::lock_guard guard(_mutex);

  for (auto &[id, shape] : _shapes)
  {
    out.create(*shape);
  }

  for (auto &shape : _transients)
  {
    out.create(*shape);
  }
}


Magnum::Matrix4 MeshSet::composeTransform(const ObjectAttributes &attrs) const
{
  return Message::composeTransform(attrs);
}


void MeshSet::decomposeTransform(const Magnum::Matrix4 &transform, ObjectAttributes &attrs) const
{
  Message::decomposeTransform(transform, attrs);
}


Magnum::Matrix4 MeshSet::composeTransform(const tes::Transform &tes_transform) const
{
  ObjectAttributes attrs = {};
  attrs.position[0] = static_cast<Magnum::Float>(tes_transform.position().x());
  attrs.position[1] = static_cast<Magnum::Float>(tes_transform.position().y());
  attrs.position[2] = static_cast<Magnum::Float>(tes_transform.position().z());
  attrs.rotation[0] = static_cast<Magnum::Float>(tes_transform.rotation().x());
  attrs.rotation[1] = static_cast<Magnum::Float>(tes_transform.rotation().y());
  attrs.rotation[2] = static_cast<Magnum::Float>(tes_transform.rotation().z());
  attrs.rotation[3] = static_cast<Magnum::Float>(tes_transform.rotation().w());
  attrs.scale[0] = static_cast<Magnum::Float>(tes_transform.scale().x());
  attrs.scale[1] = static_cast<Magnum::Float>(tes_transform.scale().y());
  attrs.scale[2] = static_cast<Magnum::Float>(tes_transform.scale().z());
  return composeTransform(attrs);
}


void MeshSet::decomposeTransform(const Magnum::Matrix4 &transform,
                                 tes::Transform &tes_transform) const
{
  ObjectAttributes attrs = {};
  decomposeTransform(transform, attrs);
  tes_transform.setPosition(
    tes::Vector3<Magnum::Float>(attrs.position[0], attrs.position[1], attrs.position[2]));
  tes_transform.setRotation(tes::Quaternion<Magnum::Float>(attrs.rotation[0], attrs.rotation[1],
                                                           attrs.rotation[2], attrs.rotation[3]));
  tes_transform.setScale(
    tes::Vector3<Magnum::Float>(attrs.scale[0], attrs.scale[1], attrs.scale[2]));
}


bool MeshSet::handleCreate(PacketReader &reader)
{
  auto shape = std::make_shared<tes::MeshSet>();
  if (!shape->readCreate(reader))
  {
    log::error("Error reading mesh create.");
    return false;
  }

  PendingQueue::Action action(util::ActionKind::Create);
  action.shape_id = shape->id();
  action.create.shape = shape;

  const std::lock_guard guard(_mutex);
  _pending_actions.emplace_back(action);
  return true;
}


bool MeshSet::handleUpdate(PacketReader &reader)
{
  UpdateMessage update;
  ObjectAttributesd attrs;
  if (!update.read(reader, attrs))
  {
    return false;
  }

  PendingQueue::Action action(util::ActionKind::Update);
  action.shape_id = update.id;
  action.update.flags = update.flags;
  action.update.position = Vector3d(attrs.position);
  action.update.rotation = Quaterniond(attrs.rotation);
  action.update.scale = Vector3d(attrs.scale);
  action.update.colour = Colour(attrs.colour);

  const std::lock_guard guard(_mutex);
  _pending_actions.emplace_back(action);
  return true;
}


bool MeshSet::handleDestroy(const DestroyMessage &msg, PacketReader &reader)
{
  (void)reader;

  PendingQueue::Action action(util::ActionKind::Destroy);
  action.shape_id = msg.id;

  const std::lock_guard guard(_mutex);
  _pending_actions.emplace_back(action);
  return true;
}


bool MeshSet::createDrawables(const std::shared_ptr<tes::MeshSet> &shape)
{
  const bool transient = shape->id() == 0;
  for (unsigned i = 0; i < shape->partCount(); ++i)
  {
    Drawable &drawable = _drawables.emplace_back();
    drawable.part_id = i;
    drawable.resource_id = shape->partResource(i)->id();
    drawable.transform =
      composeTransform(shape->attributes()) * composeTransform(shape->partTransform(i));
    const auto colour = shape->colour() * shape->partColour(i);
    drawable.colour = tes::view::convert(colour);
    drawable.owner = shape;
    // Calculate the bounds
    calculateBounds(drawable);
  }

  if (transient)
  {
    _transients.emplace_back(shape);
  }
  else
  {
    _shapes[shape->id()] = shape;
  }

  return true;
}


bool MeshSet::calculateBounds(Drawable &drawable) const
{
  auto resource = _resources->get(drawable.resource_id);
  if (!resource.isValid())
  {
    return false;
  }

  // Transform te resource bounds, then make a new bounds around the transformed box to form a
  // loose bounding box.
  drawable.resource_bounds = resource.bounds();
  const auto loose_bounds = drawable.resource_bounds.calculateLooseBounds(drawable.transform);
  if (drawable.bounds_id == BoundsCuller::kInvalidId)
  {
    drawable.bounds_id = _culler->allocate(loose_bounds);
  }
  else
  {
    _culler->update(drawable.bounds_id, loose_bounds);
  }
  return true;
}


bool MeshSet::updateShape(uint32_t shape_id, const typename PendingQueue::Action::Update &update)
{
  // Fetch the shape
  const auto find = _shapes.find(shape_id);
  if (find == _shapes.end())
  {
    return false;
  }

  auto &[id, shape] = *find;

  const bool update_all = (update.flags & UFUpdateMode) != 0u;
  const bool update_position = (update.flags & UFPosition) != 0u || update_all;
  const bool update_rotation = (update.flags & UFRotation) != 0u || update_all;
  const bool update_scale = (update.flags & UFScale) != 0u || update_all;
  const bool update_colour = (update.flags & UFColour) != 0u || update_all;

  if (update_position)
  {
    shape->setPosition(update.position);
  }
  if (update_rotation)
  {
    shape->setRotation(update.rotation);
  }
  if (update_scale)
  {
    shape->setScale(update.scale);
  }
  if (update_colour)
  {
    shape->setColour(update.colour);
  }

  return true;
}


bool MeshSet::destroyShape(uint32_t shape_id, const typename PendingQueue::Action::Destroy &destroy)
{
  TES_UNUSED(destroy);
  const auto find = _shapes.find(shape_id);
  if (find == _shapes.end() || shape_id == 0)
  {
    return false;
  }

  // Find and remove the drawables.
  const auto &shape = find->second;
  for (size_t i = 0; i < _drawables.size();)
  {
    if (_drawables[i].owner == shape)
    {
      if (i + i < _drawables.size())
      {
        // Swap the last element into index i.
        std::swap(_drawables[i], _drawables.back());
      }
      // Remove the last item.
      _drawables.resize(_drawables.size() - 1);
      // Do not increment i. We've just swapped an item into i.
    }
    else
    {
      ++i;
    }
  }

  // Remove the shape.
  _shapes.erase(find);
  return true;
}
}  // namespace tes::view::handler
