#include "3esshapecache.h"

#include "3esboundsculler.h"

#include "shaders/3esshader.h"

#include <3esdebug.h>

namespace tes::viewer::painter
{
constexpr size_t ShapeCache::kListEnd;

void ShapeCache::calcSphericalBounds(const Magnum::Matrix4 &transform, Bounds &bounds)
{
  bounds = Bounds::fromCentreHalfExtents(
    transform[3].xyz(),
    Magnum::Vector3(transform[0].xyz().length(), transform[1].xyz().length(), transform[2].xyz().length()));
}


void ShapeCache::calcCylindricalBounds(const Magnum::Matrix4 &transform, float radius, float length, Bounds &bounds)
{
  // Scale and rotate an AABB then recalculate bounds from that.
  // Note: assumes the axis.
  const std::array<Magnum::Vector3, 8> boxVertices = {
    (transform * Magnum::Vector4(-radius, -radius, 0.5f * length, 1)).xyz(),
    (transform * Magnum::Vector4(radius, -radius, 0.5f * length, 1)).xyz(),
    (transform * Magnum::Vector4(radius, radius, 0.5f * length, 1)).xyz(),
    (transform * Magnum::Vector4(-radius, radius, 0.5f * length, 1)).xyz(),
    (transform * Magnum::Vector4(-radius, -radius, -0.5f * length, 1)).xyz(),
    (transform * Magnum::Vector4(radius, -radius, -0.5f * length, 1)).xyz(),
    (transform * Magnum::Vector4(radius, radius, -0.5f * length, 1)).xyz(),
    (transform * Magnum::Vector4(-radius, radius, -0.5f * length, 1)).xyz(),
  };

  bounds = Bounds(boxVertices[0]);
  for (const auto &v : boxVertices)
  {
    bounds.expand(v);
  }
}


ShapeCache::ShapeCache(std::shared_ptr<BoundsCuller> culler, std::shared_ptr<shaders::Shader> shader, const Part &part,
                       BoundsCalculator bounds_calculator)
  : ShapeCache(std::move(culler), std::move(shader), { part }, std::move(bounds_calculator))
{}

ShapeCache::ShapeCache(std::shared_ptr<BoundsCuller> culler, std::shared_ptr<shaders::Shader> shader,
                       const std::vector<Part> &parts, BoundsCalculator bounds_calculator)
  : _culler(std::move(culler))
  , _shader(std::move(shader))
  , _parts(parts)
  , _bounds_calculator(std::move(bounds_calculator))
{
  _instance_buffers.emplace_back(InstanceBuffer{ Magnum::GL::Buffer{}, 0 });
}

ShapeCache::ShapeCache(std::shared_ptr<BoundsCuller> culler, std::shared_ptr<shaders::Shader> shader,
                       std::initializer_list<Part> parts, BoundsCalculator bounds_calculator)
  : _culler(std::move(culler))
  , _shader(std::move(shader))
  , _parts(parts)
  , _bounds_calculator(std::move(bounds_calculator))
{
  _instance_buffers.emplace_back(InstanceBuffer{ Magnum::GL::Buffer{}, 0 });
}

void ShapeCache::calcBounds(const Magnum::Matrix4 &transform, Bounds &bounds) const
{
  _bounds_calculator(transform, bounds);
}

util::ResourceListId ShapeCache::add(const tes::Id &shape_id, const Magnum::Matrix4 &transform,
                                     const Magnum::Color4 &colour, ShapeFlag flags, util::ResourceListId parent_rid,
                                     unsigned *child_index)
{
  auto shape = _shapes.allocate();

  shape->flags = flags | ShapeFlag::Pending;
  shape->current.transform = transform;
  shape->current.colour = colour;
  shape->parent_rid = parent_rid;
  shape->next = kListEnd;
  shape->shape_id = shape_id;

  Bounds bounds;
  calcBoundsForShape(*shape, bounds);
  const auto bounds_id = _culler->allocate(bounds);
  shape->bounds_id = bounds_id;

  if (parent_rid != kListEnd)
  {
    // Add to a shape chain.
    auto parent = _shapes.at(parent_rid);
    // To assert or validate?
    TES_ASSERT(parent.isValid());
    if (parent.isValid())
    {
      shape->parent_rid = parent.id();
      shape->next = parent->next;
      parent->next = shape.id();
      if (child_index)
      {
        *child_index = parent->child_count;
      }
      ++parent->child_count;
    }
  }

  return shape.id();
}

bool ShapeCache::endShape(util::ResourceListId id)
{
  if (id < _shapes.size())
  {
    // End shapes while valid to the end of the chain.
    // The first item, specified by @p id, must not be part of a chain.
    util::ResourceListId remove_next = id;
    auto shape = _shapes.at(id);
    // Only remove valid shapes which are not parented (we can only remove parent shapes).
    if (shape.isValid() && shape->parent_rid == kListEnd)
    {
      while (shape.isValid())
      {
        const auto next_id = shape->next;
        // Mark as transient to remove on the next commit.
        shape->flags |= ShapeFlag::Transient;
        // Clear pending flag in case it was added the same update.
        shape->flags & ~ShapeFlag::Pending;
        shape = _shapes.at(next_id);
      }
      return true;
    }
  }
  return false;
}

bool ShapeCache::update(util::ResourceListId id, const Magnum::Matrix4 &transform, const Magnum::Color4 &colour)
{
  if (id < _shapes.size())
  {
    auto shape = _shapes.at(id);
    if (shape.isValid())
    {
      shape->updated.transform = transform;
      shape->updated.colour = colour;
      shape->flags |= ShapeFlag::Dirty;
      // Don't update bounds now. That will be done during the commit().
      return true;
    }
  }

  return false;
}


bool ShapeCache::get(util::ResourceListId id, bool apply_parent_transform, Magnum::Matrix4 &transform,
                     Magnum::Color4 &colour) const
{
  bool found = false;
  transform = Magnum::Matrix4();
  if (id < _shapes.size())
  {
    const auto shape = _shapes.at(id);
    if (shape.isValid())
    {
      found = (shape->flags & ShapeFlag::Pending) == ShapeFlag::None;
      transform = shape->current.transform;
      colour = shape->current.colour;

      if (apply_parent_transform && shape->parent_rid != kListEnd)
      {
        auto parent = _shapes.at(shape->parent_rid);
        while (parent.isValid())
        {
          transform = parent->current.transform * transform;
          // Should really modulate colour values squared to be "correct" (gamma space I think?).
          colour = parent->current.colour * colour;
          parent = _shapes.at(parent->parent_rid);
        }
      }
    }
  }
  return found;
}


util::ResourceListId ShapeCache::getChildId(util::ResourceListId parent_id, unsigned child_index) const
{
  auto parent = _shapes.at(parent_id);

  if (!parent.isValid() || parent->child_count <= child_index)
  {
    return util::kNullResource;
  }

  // Children appear in reverse order on the parent list.
  auto child = _shapes.at(parent->next);
  const auto child_count = parent->child_count;
  parent.release();
  for (unsigned i = child_count - 1 - child_index; i > 0 && child.isValid(); --i)
  {
    child = _shapes.at(child->next);
  }

  if (!child.isValid())
  {
    return util::kNullResource;
  }

  return child.id();
}


void ShapeCache::commit()
{
  Bounds bounds;
  for (auto iter = _shapes.begin(); iter != _shapes.end(); ++iter)
  {
    // Update bounds if changed.
    if ((iter->flags & ShapeFlag::Dirty) != ShapeFlag::None)
    {
      iter->current = iter->updated;
      calcBoundsForShape(*iter, bounds);
      _culler->update(iter->bounds_id, bounds);
    }

    // Effect removal, based on Transient flag. We skip Transient and Pending items as this is the initial state for
    // transient shapes yet to be commited.
    if ((iter->flags & (ShapeFlag::Transient | ShapeFlag::Pending)) == ShapeFlag::Transient)
    {
      release(iter.id());
    }
    else
    {
      // Clear pending and dirty flags to effect visibility.
      iter->flags &= ~(ShapeFlag::Pending | ShapeFlag::Dirty);
    }
  }
}


void ShapeCache::draw(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix,
                      const Magnum::Matrix4 &view_matrix)
{
  _shader->setProjectionMatrix(projection_matrix);
  _shader->setViewMatrix(view_matrix);
  _shader->setModelMatrix({});
  buildInstanceBuffers(stamp);
  for (auto &buffer : _instance_buffers)
  {
    if (buffer.count)
    {
      // for (const auto &part : _parts)
      for (size_t i = 0; i < _parts.size(); ++i)
      {
        const auto &part = _parts[i];
        // TODO(KS): see if we can enable this part transform usage.
        // _shader->setModelMatrix(part.transform);
        _shader->setColour(part.colour);
        _shader->draw(*part.mesh, buffer.buffer, buffer.count);
      }
    }
  }
}


void ShapeCache::clear()
{
  for (const auto &shape : _shapes)
  {
    _culler->release(shape.bounds_id);
  }
  _shapes.clear();
}


void ShapeCache::calcBoundsForShape(const Shape &shape, Bounds &bounds) const
{
  bool bounds_ready = false;

  const auto shape_transform =
    ((shape.flags & ShapeFlag::Dirty) == ShapeFlag::None) ? shape.current.transform : shape.updated.transform;

  if (shape.parent_rid != kListEnd)
  {
    // Need to include the parent transform to calculate bounds.
    auto parent_ref = _shapes.at(shape.parent_rid);
    if (parent_ref.isValid())
    {
      // We assume the parent is updated before the child.
      const auto parent_transform = ((parent_ref->flags & ShapeFlag::Dirty) == ShapeFlag::None) ?
                                      parent_ref->current.transform :
                                      parent_ref->updated.transform;
      parent_ref.release();
      calcBounds(parent_transform * shape_transform, bounds);
      bounds_ready = true;
    }
  }
  if (!bounds_ready)
  {
    calcBounds(shape_transform, bounds);
  }
}


bool ShapeCache::release(util::ResourceListId id)
{
  // Remove shapes while valid to the end of the chain.
  // The first item, specified by @p index, must not be part of a chain.
  util::ResourceListId remove_next = id;
  auto shape_ref = _shapes.at(id);
  if (shape_ref.isValid() && shape_ref->parent_rid == kListEnd)
  {
    bool removed = false;
    do
    {
      _culler->release(shape_ref->bounds_id);
      const auto remove_current = remove_next;
      remove_next = shape_ref->next;
      shape_ref->parent_rid = 0u;
      shape_ref->next = kListEnd;
      shape_ref = _shapes.at(remove_next);
      _shapes.release(remove_current);
      removed = true;
    } while (shape_ref.isValid() && remove_next != util::kNullResource);

    return removed;
  }
  return false;
}


void ShapeCache::buildInstanceBuffers(const FrameStamp &stamp)
{
  (void)stamp;
  // Clear previous results.
  for (auto &buffer : _instance_buffers)
  {
    buffer.count = 0;
  }

  if (!_culler)
  {
    return;
  }

  // Work through the instance list collecting visible items.
  auto &culler = *_culler;
  unsigned cur_instance_buffer_idx = 0;

  // Function to upload the contents of the marshalling buffer to the GPU.
  const auto upload_buffer = [this, &cur_instance_buffer_idx](bool add_buffer_on_full) {
    // Upload current data.
    _instance_buffers[cur_instance_buffer_idx].buffer.setData(_marshal_buffer, Magnum::GL::BufferUsage::StaticDraw);
    ++cur_instance_buffer_idx;
    // Start new buffer if required.
    if (add_buffer_on_full && cur_instance_buffer_idx >= _instance_buffers.size())
    {
      _instance_buffers.emplace_back(InstanceBuffer{ Magnum::GL::Buffer{}, 0 });
    }
  };

  const bool have_transform_modifier = bool(_transform_modifier);

  // Iterate shapes and marshal/upload.
  for (auto iter = _shapes.begin(); iter != _shapes.end(); ++iter)
  {
    if ((iter->flags & (ShapeFlag::Pending | ShapeFlag::Hidden)) == ShapeFlag::None &&
        culler.isVisible(iter->bounds_id))
    {
      const unsigned marshal_index = _instance_buffers[cur_instance_buffer_idx].count;
      ++_instance_buffers[cur_instance_buffer_idx].count;
      if (iter->parent_rid == kListEnd)
      {
        _marshal_buffer[marshal_index] = iter->current;
      }
      else
      {
        // Child shape. Include parent transforms.
        get(iter.id(), true, _marshal_buffer[marshal_index].transform, _marshal_buffer[marshal_index].colour);
      }

      if (have_transform_modifier)
      {
        _transform_modifier(_marshal_buffer[marshal_index].transform);
      }

      // Upload if at limit.
      if (_instance_buffers[cur_instance_buffer_idx].count == _marshal_buffer.size())
      {
        upload_buffer(true);
      }
    }
  }

  // Upload the last buffer.
  if (_instance_buffers[cur_instance_buffer_idx].count > 0)
  {
    upload_buffer(false);
  }
}
}  // namespace tes::viewer::painter
