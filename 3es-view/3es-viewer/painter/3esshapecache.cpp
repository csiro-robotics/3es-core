#include "3esshapecache.h"

#include "3esboundsculler.h"

#include <cassert>

namespace tes::viewer::painter
{
constexpr size_t ShapeCache::kListEnd;

ShapeCacheShader::~ShapeCacheShader() = default;

ShapeCacheShaderFlat::ShapeCacheShaderFlat()
  : _shader(Magnum::Shaders::Flat3D::Flag::VertexColor | Magnum::Shaders::Flat3D::Flag::InstancedTransformation)
{}


ShapeCacheShaderFlat::~ShapeCacheShaderFlat() = default;


void ShapeCacheShaderFlat::setProjectionMatrix(const Magnum::Matrix4 &projection)
{
  _shader.setTransformationProjectionMatrix(projection);
}

void ShapeCacheShaderFlat::draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer, size_t instance_count)
{
  mesh.setInstanceCount(instance_count)
    .addVertexBufferInstanced(buffer, 1, 0, Magnum::Shaders::Flat3D::TransformationMatrix{},
                              Magnum::Shaders::Flat3D::Color4{});
  _shader.draw(mesh);
}


void ShapeCache::calcSphericalBounds(const Magnum::Matrix4 &transform, Magnum::Vector3 &centre,
                                     Magnum::Vector3 &halfExtents)
{
  halfExtents[0] = transform[0].xyz().length();
  halfExtents[1] = transform[1].xyz().length();
  halfExtents[2] = transform[2].xyz().length();
  centre = transform[3].xyz();
}


void ShapeCache::calcCylindricalBounds(const Magnum::Matrix4 &transform, float radius, float length,
                                       Magnum::Vector3 &centre, Magnum::Vector3 &halfExtents)
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

  Magnum::Vector3 minExt = boxVertices[0];
  Magnum::Vector3 maxExt = boxVertices[0];
  centre = {};
  for (const auto &v : boxVertices)
  {
    centre += v;
    minExt.x() = std::min(v.x(), minExt.x());
    minExt.y() = std::min(v.y(), minExt.y());
    minExt.z() = std::min(v.z(), minExt.z());
    maxExt.x() = std::max(v.x(), maxExt.x());
    maxExt.y() = std::max(v.y(), maxExt.y());
    maxExt.z() = std::max(v.z(), maxExt.z());
  }
  centre /= float(boxVertices.size());
  halfExtents = 0.5f * (maxExt - minExt);
}


ShapeCache::ShapeCache(std::shared_ptr<BoundsCuller> culler, const Part &part,
                       std::shared_ptr<ShapeCacheShader> &&shader, BoundsCalculator bounds_calculator)
  : ShapeCache(std::move(culler), { part }, std::move(shader), std::move(bounds_calculator))
{}

ShapeCache::ShapeCache(std::shared_ptr<BoundsCuller> culler, const std::vector<Part> &parts,
                       std::shared_ptr<ShapeCacheShader> &&shader, BoundsCalculator bounds_calculator)
  : _culler(std::move(culler))
  , _parts(parts)
  , _shader(std::move(shader))
  , _bounds_calculator(std::move(bounds_calculator))
{
  _instance_buffers.emplace_back(InstanceBuffer{ Magnum::GL::Buffer{}, 0 });
}

ShapeCache::ShapeCache(std::shared_ptr<BoundsCuller> culler, std::initializer_list<Part> parts,
                       std::shared_ptr<ShapeCacheShader> &&shader, BoundsCalculator bounds_calculator)
  : _culler(std::move(culler))
  , _parts(parts)
  , _shader(std::move(shader))
  , _bounds_calculator(std::move(bounds_calculator))
{
  _instance_buffers.emplace_back(InstanceBuffer{ Magnum::GL::Buffer{}, 0 });
}

void ShapeCache::calcBounds(const Magnum::Matrix4 &transform, Magnum::Vector3 &centre, Magnum::Vector3 &halfExtents)
{
  _bounds_calculator(transform, centre, halfExtents);
}

util::ResourceListId ShapeCache::add(const ViewableWindow &window, const Magnum::Matrix4 &transform,
                                     const Magnum::Color4 &colour, util::ResourceListId parent_index)
{
  auto shape = _shapes.allocate();

  Magnum::Vector3 centre;
  Magnum::Vector3 halfExtents;
  _bounds_calculator(transform, centre, halfExtents);

  const auto bounds_id = _culler->allocate(centre, halfExtents);
  _viewables.emplace_back(ShapeViewable{ { transform, colour }, window, bounds_id, kListEnd });

  shape->viewable_head = shape->viewable_tail = _viewables.size() - 1u;
  shape->window = window;

  shape->bounds_id = bounds_id;
  shape->parent_index = parent_index;
  shape->next = kListEnd;

  if (parent_index != kListEnd)
  {
    // Add to a shape chain.
    auto chain_head = _shapes.at(parent_index);
    assert(chain_head.isValid());
    shape->next = chain_head.id();
    chain_head->next = shape.id();
  }

  return shape.id();
}

bool ShapeCache::endShape(util::ResourceListId id, FrameNumber frame_number)
{
  if (id < _shapes.size())
  {
    // End shapes while valid to the end of the chain.
    // The first item, specified by @p id, must not be part of a chain.
    util::ResourceListId remove_next = id;
    auto shape = _shapes.at(id);
    // Only remove valid shapes which are not parented (we can only remove parent shapes).
    if (shape.isValid() && shape->parent_index == kListEnd)
    {
      bool removed = false;
      auto next_shape = _shapes.at(shape->next);
      while (remove_next != kListEnd && next_shape.isValid())
      {
        const auto next_id = next_shape.id();
        // Set to disable on the next frame, while keeping the same current frame.
        next_shape->window =
          ViewableWindow(next_shape->window.startFrame(), frame_number, ViewableWindow::Interval::Absolute);
        // Update the tail viewable window.
        auto last_viewable_window = _viewables[next_shape->viewable_tail].window;
        last_viewable_window =
          ViewableWindow(last_viewable_window.startFrame(), frame_number, ViewableWindow::Interval::Absolute);
        _viewables[next_shape->viewable_tail].window = last_viewable_window;
        removed = true;
        next_shape = _shapes.at(next_id);
      }
      return removed;
    }
  }
  return false;
}

bool ShapeCache::update(util::ResourceListId id, FrameNumber frame_number, const Magnum::Matrix4 &transform,
                        const Magnum::Color4 &colour)
{
  if (id < _shapes.size())
  {
    auto shape = _shapes.at(id);
    if (shape.isValid())
    {
      // Notes on updating shapes.
      // When we update a shape, we add a new ShapeViewable to the shape, referenced via Shape::viewable_tail. This
      // viewable represents the state of the shape at the given frame_number.
      //
      // However, this is predicated on the frame_number representing a new state for the shape, temporally speaking.
      // This may not always be the case, as can happen when we rewind to a previous frame, then receive an update() for
      // which has already been processed as the same update message is repeated.
      //
      // We essentially make the assumption that an update() call is redundant if its frame_number occurs at or before
      // the latest presentation of the shape. There's one exception which is when the updated transform/colour do not
      // match the latest viewable state, as could happen when creating a shape, then modifying it with an update
      // message in the same frame.
      //
      // Finally, even if the update() is redundant, we do update the shape bounds.

      auto last_viewable_window = _viewables[shape->viewable_tail].window;
      bool redundant_update = false;
      if (frame_number < last_viewable_window.startFrame())
      {
        redundant_update = true;
      }
      else if (frame_number == last_viewable_window.startFrame())
      {
        // Potentially rendundant update. Need to check the contents.
        redundant_update = _viewables[shape->viewable_tail].instance.transform == transform &&
                           _viewables[shape->viewable_tail].instance.colour == colour;
      }

      if (!redundant_update)
      {
        // Not a redundant update. Add a viewable state.
        ShapeViewable &viewable = _viewables.emplace_back();
        // Must fetch the last viewable state after adding to the list in case of reallocation.
        ShapeViewable &last_viewable = _viewables[shape->viewable_tail];
        // Duplicate the last viewable state.
        viewable = last_viewable;
        // Set updated values.
        viewable.instance.transform = transform;
        viewable.instance.colour = colour;
        viewable.next = kListEnd;
        // Update the list tail.
        shape->viewable_tail = last_viewable.next = _viewables.size() - 1u;
        // Update the viewable windows.
        last_viewable.window =
          ViewableWindow(last_viewable.window.startFrame(), frame_number, ViewableWindow::Interval::Absolute);
        viewable.window = ViewableWindow(frame_number);
      }
      // else redundant update. Just continue to bounds update.

      Magnum::Vector3 centre;
      Magnum::Vector3 halfExtents;
      _bounds_calculator(transform, centre, halfExtents);
      _culler->update(shape->bounds_id, centre, halfExtents);
      return true;
    }
  }

  return false;
}


bool ShapeCache::get(util::ResourceListId id, FrameNumber frame_number, bool apply_parent_transform,
                     Magnum::Matrix4 &transform, Magnum::Color4 &colour) const
{
  bool found = false;
  transform = Magnum::Matrix4();
  while (id < _shapes.size())
  {
    const auto shape = _shapes.at(id);
    if (shape.isValid())
    {
      found = true;
      // Start with checking the lastest viewable state.
      if (_viewables[shape->viewable_tail].window.overlaps(frame_number))
      {
        // Latest item is the relevant one.
        transform = _viewables[shape->viewable_tail].instance.transform;
        colour = _viewables[shape->viewable_tail].instance.colour;
      }
      else
      {
        // Need to traverse the list.
        size_t next = shape->viewable_head;
        while (next != kListEnd)
        {
          if (_viewables[next].window.overlaps(frame_number))
          {
            // Latest item is the relevant one.
            transform = _viewables[next].instance.transform;
            colour = _viewables[next].instance.colour;
            break;
          }
          next = _viewables[next].next;
        }
      }
      id = (apply_parent_transform) ? shape->parent_index : kListEnd;
    }
  }
  return found;
}


void ShapeCache::clear()
{
  for (const auto &shape : _shapes)
  {
    _culler->release(shape.bounds_id);
  }
  _shapes.clear();
}


void ShapeCache::draw(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix)
{
  buildInstanceBuffers(stamp);
  for (auto &buffer : _instance_buffers)
  {
    if (buffer.count)
    {
      for (const auto &part : _parts)
      {
        const Magnum::Matrix4 projection = projection_matrix * part.transform;
        _shader->setProjectionMatrix(projection);
        _shader->draw(*part.mesh, buffer.buffer, buffer.count);
      }
    }
  }
}


void ShapeCache::expireShapes(FrameNumber before_frame)
{
  for (auto iter = _shapes.begin(); iter != _shapes.end(); ++iter)
  {
    auto &shape = *iter;
    if (shape.parent_index == kListEnd)
    {
      const FrameNumber last_frame = shape.window.endFrame();
      if (last_frame < before_frame)
      {
        // Note: it's actually ok to remove items from the resource list during iteration since it just adds things to
        // a free list.
        // Could be considered flakey though.
        release(iter.id());
      }
    }
  }
}


bool ShapeCache::release(util::ResourceListId id)
{
  // Remove shapes while valid to the end of the chain.
  // The first item, specified by @p index, must not be part of a chain.
  util::ResourceListId remove_next = id;
  auto shape_ref = _shapes.at(id);
  if (shape_ref.isValid() && shape_ref->parent_index == kListEnd)
  {
    bool removed = false;
    do
    {
      _culler->release(shape_ref->bounds_id);
      const auto remove_current = remove_next;
      remove_next = shape_ref->next;
      shape_ref->parent_index = 0u;
      shape_ref->next = kListEnd;
      shape_ref = _shapes.at(remove_next);
      _shapes.release(remove_current);
      removed = true;
    } while (shape_ref.isValid() && remove_next != _shapes.kNull);

    return removed;
  }
  return false;
}


void ShapeCache::buildInstanceBuffers(const FrameStamp &stamp)
{
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
  const auto upload_buffer = [this, &cur_instance_buffer_idx]() {
    // Upload current data.
    _instance_buffers[cur_instance_buffer_idx].buffer.setData(_marshal_buffer, Magnum::GL::BufferUsage::DynamicDraw);
    // Start new buffer.
    if (cur_instance_buffer_idx >= _instance_buffers.size())
    {
      _instance_buffers.emplace_back(InstanceBuffer{ Magnum::GL::Buffer{}, 0 });
    }
    ++cur_instance_buffer_idx;
  };

  // Iterate shapes and marshal/upload.
  for (auto &viewable : _viewables)
  {
    if (viewable.window.overlaps(stamp.frame_number) && culler.isVisible(viewable.bounds_id))
    {
      const unsigned marshal_index = _instance_buffers[cur_instance_buffer_idx].count;
      ++_instance_buffers[cur_instance_buffer_idx].count;
      _marshal_buffer[marshal_index] = viewable.instance;
      // unsigned parent_index = shape.parent_index;
      // TODO(KS): Include the parent transform(s).
      // while (parent_index != ~0u)
      // {
      //   _marshal_buffer[marshal_index].transform =
      //     _shapes[parent_index].instance.transform * _marshal_buffer[marshal_index].transform;
      //   parent_index = _shapes[parent_index].parent_index;
      // }

      // Upload if at limit.
      if (_instance_buffers[cur_instance_buffer_idx].count == _marshal_buffer.size())
      {
        upload_buffer();
      }
    }
  }

  // Upload the last buffer.
  if (_instance_buffers[cur_instance_buffer_idx].count > 0)
  {
    upload_buffer();
  }
}
}  // namespace tes::viewer::painter
