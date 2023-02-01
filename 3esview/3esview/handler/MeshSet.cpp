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
  std::lock_guard guard(_mutex);
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


void MeshSet::beginFrame(const FrameStamp &stamp)
{
  (void)stamp;
  std::lock_guard guard(_mutex);
  // Update meshes and bounds.
  // @note: MeshResource handler has to beginFrame() first.

  for (auto &drawable : _drawables)
  {
    beginFrameForDrawable(drawable);
  }

  for (auto &shape : _transients)
  {
    shape.flags &= ~(DrawableFlag::Pending | DrawableFlag::Dirty);
  }

  for (auto &[id, shape] : _shapes)
  {
    shape.flags &= ~(DrawableFlag::Pending | DrawableFlag::Dirty);
  }
}


void MeshSet::endFrame(const FrameStamp &stamp)
{
  (void)stamp;
  std::lock_guard guard(_mutex);

  // We can clean up marked for death here, in the background thread.
  for (size_t i = 0; i < _drawables.size(); ++i)
  {
    // Remove marked for death, but not pending. Transients have both set.
    if ((_drawables[i].flags & (DrawableFlag::MarkForDeath | DrawableFlag::Pending)) == DrawableFlag::MarkForDeath)
    {
      // Item marked for removal. Remove/swap last and decrement i for the next iteration.
      std::swap(_drawables[i], _drawables.back());
      _culler->release(_drawables.back().bounds_id);
      _drawables.pop_back();
      --i;  // May underflow, then overflow again. Net result is fine.
    }
  }

  for (size_t i = 0; i < _transients.size(); ++i)
  {
    if ((_transients[i].flags & (DrawableFlag::MarkForDeath | DrawableFlag::Pending)) == DrawableFlag::MarkForDeath)
    {
      std::swap(_transients[i], _transients.back());
      _transients.pop_back();
      --i;  // May underflow, then overflow again. Net result is fine.
    }
  }
}


void MeshSet::draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params)
{
  (void)stamp;
  std::lock_guard guard(_mutex);
  _draw_sets[0].clear();
  _draw_sets[1].clear();
  for (const auto &drawable : _drawables)
  {
    if ((drawable.flags & (DrawableFlag::Pending | DrawableFlag::MarkForDeath)) != DrawableFlag::Zero)
    {
      continue;
    }
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

  const MeshResource::DrawFlag flags =
    (pass == DrawPass::Transparent) ? MeshResource::DrawFlag::Transparent : MeshResource::DrawFlag::Zero;
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
    log::error(name(), " : unhandled shape message type: ", unsigned(reader.messageId()));
    logged = true;
    break;
  }

  if (!ok && !logged)
  {
    log::error(name(), " : failed to decode message type: ", unsigned(reader.messageId()));
  }
}


void MeshSet::serialise(Connection &out, ServerInfoMessage &info)
{
  (void)info;
  std::lock_guard guard(_mutex);

  const auto serialise_shape = [&out](const MeshItem &mesh) {
    const auto pending_and_dead = mesh.flags & (DrawableFlag::Pending | DrawableFlag::MarkForDeath);
    // Save non-pending, non-dead shapes. Items marked with both are transient, yet to be draw.
    if (pending_and_dead == DrawableFlag::Zero ||
        pending_and_dead == (DrawableFlag::Pending | DrawableFlag::MarkForDeath))
    {
      if (mesh.shape)
      {
        out.create(*mesh.shape);
      }
    }
  };

  for (auto &[id, shape] : _shapes)
  {
    serialise_shape(shape);
  }

  for (auto &shape : _transients)
  {
    serialise_shape(shape);
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
  attrs.position[0] = Magnum::Float(tes_transform.position().x());
  attrs.position[1] = Magnum::Float(tes_transform.position().y());
  attrs.position[2] = Magnum::Float(tes_transform.position().z());
  attrs.rotation[0] = Magnum::Float(tes_transform.rotation().x());
  attrs.rotation[1] = Magnum::Float(tes_transform.rotation().y());
  attrs.rotation[2] = Magnum::Float(tes_transform.rotation().z());
  attrs.rotation[3] = Magnum::Float(tes_transform.rotation().w());
  attrs.scale[0] = Magnum::Float(tes_transform.scale().x());
  attrs.scale[1] = Magnum::Float(tes_transform.scale().y());
  attrs.scale[2] = Magnum::Float(tes_transform.scale().z());
  return composeTransform(attrs);
}


void MeshSet::decomposeTransform(const Magnum::Matrix4 &transform, tes::Transform &tes_transform) const
{
  ObjectAttributes attrs = {};
  decomposeTransform(transform, attrs);
  tes_transform.setPosition(tes::Vector3<Magnum::Float>(attrs.position[0], attrs.position[1], attrs.position[2]));
  tes_transform.setRotation(
    tes::Quaternion<Magnum::Float>(attrs.rotation[0], attrs.rotation[1], attrs.rotation[2], attrs.rotation[3]));
  tes_transform.setScale(tes::Vector3<Magnum::Float>(attrs.scale[0], attrs.scale[1], attrs.scale[2]));
}


bool MeshSet::handleCreate(PacketReader &reader)
{
  auto shape = std::make_shared<tes::MeshSet>();
  if (!shape->readCreate(reader))
  {
    log::error("Error reading mesh create.");
    return false;
  }

  return create(shape);
}


bool MeshSet::handleUpdate(PacketReader &reader)
{
  std::lock_guard guard(_mutex);
  uint32_t id = 0;

  if (!reader.peek(reinterpret_cast<uint8_t *>(&id), sizeof(id)))
  {
    return false;
  }

  // Update the mesh set flags. The drawables flags will update on endFrame().
  auto search = _shapes.find(id);
  if (search == _shapes.end())
  {
    return false;
  }

  // Update the transform details.
  auto &shape = search->second.shape;
  bool ok = shape->readUpdate(reader);

  // This won't scale, but good enough to start.
  for (auto &drawable : _drawables)
  {
    if (drawable.owner == shape)
    {
      drawable.flags |= DrawableFlag::DirtyAttributes;
    }
  }

  return ok;
}


bool MeshSet::handleDestroy(const DestroyMessage &msg, PacketReader &reader)
{
  (void)reader;
  std::lock_guard guard(_mutex);

  auto search = _shapes.find(msg.id);
  if (search == _shapes.end())
  {
    return false;
  }

  auto &shape = search->second.shape;
  search->second.flags |= DrawableFlag::MarkForDeath;

  // This won't scale, but good enough to start.
  for (auto &drawable : _drawables)
  {
    if (drawable.owner == shape)
    {
      drawable.flags |= DrawableFlag::MarkForDeath;
    }
  }

  return true;
}


bool MeshSet::create(const std::shared_ptr<tes::MeshSet> &shape)
{
  std::lock_guard guard(_mutex);
  const bool transient = shape->id() == 0;
  for (unsigned i = 0; i < shape->partCount(); ++i)
  {
    Drawable &drawable = _drawables.emplace_back();
    drawable.part_id = i;
    drawable.resource_id = shape->partResource(i)->id();
    drawable.transform = composeTransform(shape->attributes()) * composeTransform(shape->partTransform(i));
    const auto colour = shape->colour() * shape->partColour(i);
    drawable.colour = tes::view::convert(colour);
    drawable.owner = shape;
    drawable.flags = DrawableFlag::Pending;
    if (transient)
    {
      drawable.flags |= DrawableFlag::MarkForDeath;
    }
  }

  if (transient)
  {
    _transients.emplace_back(MeshItem{ shape, DrawableFlag::Pending | DrawableFlag::MarkForDeath });
  }
  else
  {
    _shapes[shape->id()] = MeshItem{ shape, DrawableFlag::Pending };
  }

  return true;
}


void MeshSet::beginFrameForDrawable(Drawable &drawable)
{
  if ((drawable.flags & (DrawableFlag::Pending | DrawableFlag::MarkForDeath)) == DrawableFlag::MarkForDeath)
  {
    // Marked for death only (also pending => transient yet to be displayed)
    // Will be removed in end frame, so we kind of shouldn't be here.
    return;
  }

  if ((drawable.flags & (DrawableFlag::Pending | DrawableFlag::DirtyAttributes)) != DrawableFlag::Zero)
  {
    auto resource = _resources->get(drawable.resource_id);
    if (!resource.isValid())
    {
      // Missing resoure. Don't update anything, except we'll clear the pending flag
      // for transient items where marked for death is true. Otherwise we wont' cleanup.
      if ((drawable.flags & DrawableFlag::MarkForDeath) == DrawableFlag::MarkForDeath)
      {
        drawable.flags &= ~DrawableFlag::Pending;
      }
      return;
    }

    drawable.transform = composeTransform(drawable.owner->attributes()) *
                         composeTransform(drawable.owner->partTransform(drawable.part_id));
    const auto bounds = resource.bounds();
    // We assume the extends are spherical rather than defined an AABB and just transloate the centre.
    drawable.bounds = Bounds::fromCentreHalfExtents((drawable.transform * Magnum::Vector4(bounds.centre(), 1)).xyz(),
                                                    bounds.halfExtents());

    if ((drawable.flags & DrawableFlag::Pending) != DrawableFlag::Zero)
    {
      // New shape. Resolve mesh and bounds.
      drawable.bounds_id = _culler->allocate(drawable.bounds);
      drawable.flags &= ~(DrawableFlag::Pending | DrawableFlag::Dirty);
    }

    if ((drawable.flags & DrawableFlag::DirtyAttributes) != DrawableFlag::Zero)
    {
      // Just update bounds.
      _culler->update(drawable.bounds_id, drawable.bounds);
      drawable.flags &= ~DrawableFlag::Dirty;
    }
  }
}
}  // namespace tes::view::handler
