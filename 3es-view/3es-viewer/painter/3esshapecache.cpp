#include "3esshapecache.h"

#include "3esboundsculler.h"

#include <cassert>

namespace tes::viewer::painter
{
constexpr unsigned ShapeCache::kListEnd;

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

unsigned ShapeCache::add(const Magnum::Matrix4 &transform, const Magnum::Color4 &colour, unsigned parent_index,
                         unsigned chain_index)
{
  unsigned id;
  Shape *shape = {};
  if (_free_list != kListEnd)
  {
    id = _free_list;
    shape = &_shapes[id];
    _free_list = shape->next;
  }
  else
  {
    id = unsigned(_shapes.size());
    _shapes.emplace_back();
    shape = &_shapes.back();
  }

  shape->instance.transform = transform;
  shape->instance.colour = colour;

  Magnum::Vector3 centre;
  Magnum::Vector3 halfExtents;
  _bounds_calculator(transform, centre, halfExtents);
  shape->bounds_id = _culler->allocate(centre, halfExtents);
  shape->flags = unsigned(ShapeFlag::Valid) + !!(parent_index != ~0u) * unsigned(ShapeFlag::Parented);
  shape->parent_index = parent_index;
  shape->next = kListEnd;

  if (chain_index != kListEnd)
  {
    // Add to a shape chain.
    assert(chain_index < _shapes.size());
    Shape &chain_head = _shapes[chain_index];
    shape->next = chain_head.next;
    chain_head.next = id;
    shape->flags |= unsigned(ShapeFlag::ChainSegment);
  }

  return id;
}

bool ShapeCache::remove(unsigned id)
{
  if (id < _shapes.size())
  {
    // Remove shapes while valid to the end of the chain.
    // The first item, specified by @p id, must not be part of a chain.
    unsigned remove_next = id;
    if ((_shapes[remove_next].flags & (unsigned(ShapeFlag::Valid) | unsigned(ShapeFlag::ChainSegment))) == 0u)
    {
      bool removed = false;
      while (remove_next != kListEnd && (_shapes[remove_next].flags & unsigned(ShapeFlag::Valid)) == 0)
      {
        Shape &shape = _shapes[remove_next];
        if (shape.flags & (unsigned(ShapeFlag::Valid)) == 0u)
        {
          _culler->release(shape.bounds_id);
          shape.flags = 0u;
          remove_next = shape.next;
          shape.next = _free_list;
          _free_list = remove_next;
          removed = true;
        }
      }
      return removed;
    }
  }
  return false;
}

bool ShapeCache::update(unsigned id, const Magnum::Matrix4 &transform, const Magnum::Color4 &colour)
{
  if (id < _shapes.size())
  {
    Shape &shape = _shapes[id];
    if (shape.flags & unsigned(ShapeFlag::Valid))
    {
      shape.instance.transform = transform;
      shape.instance.colour = colour;

      Magnum::Vector3 centre;
      Magnum::Vector3 halfExtents;
      _bounds_calculator(transform, centre, halfExtents);
      _culler->update(shape.bounds_id, centre, halfExtents);
      return true;
    }
  }

  return false;
}


bool ShapeCache::get(unsigned id, Magnum::Matrix4 &transform, Magnum::Color4 &colour) const
{
  if (id < _shapes.size())
  {
    const Shape &shape = _shapes[id];
    if (shape.flags & unsigned(ShapeFlag::Valid))
    {
      transform = shape.instance.transform;
      colour = shape.instance.colour;
    }
  }
  return false;
}


void ShapeCache::clear()
{
  for (const auto &shape : _shapes)
  {
    _culler->release(shape.bounds_id);
  }
  _shapes.clear();
  _free_list = kListEnd;
}


void ShapeCache::draw(unsigned render_mark, const Magnum::Matrix4 &projection_matrix)
{
  buildInstanceBuffers(render_mark);
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


void ShapeCache::buildInstanceBuffers(unsigned render_mark)
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
  for (auto &shape : _shapes)
  {
    if ((shape.flags & unsigned(ShapeFlag::Valid)) && culler.isVisible(shape.bounds_id))
    {
      const unsigned marshal_index = _instance_buffers[cur_instance_buffer_idx].count;
      ++_instance_buffers[cur_instance_buffer_idx].count;
      _marshal_buffer[marshal_index] = shape.instance;
      unsigned parent_index = shape.parent_index;
      // Include the parent transform(s).
      while (parent_index != ~0u)
      {
        _marshal_buffer[marshal_index].transform =
          _shapes[parent_index].instance.transform * _marshal_buffer[marshal_index].transform;
        parent_index = _shapes[parent_index].parent_index;
      }

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
