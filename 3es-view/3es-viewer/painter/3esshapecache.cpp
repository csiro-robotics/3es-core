#include "3esshapecache.h"

#include "3esboundsculler.h"

namespace tes::viewer::painter
{
constexpr unsigned ShapeCache::kFreeListEnd;

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


void ShapeCache::defaultCalcBounds(const Magnum::Matrix4 &transform, Magnum::Vector3 &centre,
                                   Magnum::Vector3 &half_extents)
{
  half_extents[0] = transform[0].xyz().length();
  half_extents[1] = transform[1].xyz().length();
  half_extents[2] = transform[2].xyz().length();
  centre = transform[3].xyz();
}


ShapeCache::ShapeCache(std::shared_ptr<BoundsCuller> culler, const Part &part,
                       std::unique_ptr<ShapeCacheShaderFlat> &&shader, BoundsCalculator bounds_calculator)
  : ShapeCache(std::move(culler), { part }, std::move(shader), std::move(bounds_calculator))
{}

ShapeCache::ShapeCache(std::shared_ptr<BoundsCuller> culler, const std::vector<Part> &parts,
                       std::unique_ptr<ShapeCacheShaderFlat> &&shader, BoundsCalculator bounds_calculator)
  : _culler(std::move(culler))
  , _parts(parts)
  , _shader(std::move(shader))
  , _bounds_calculator(std::move(bounds_calculator))
{
  _instance_buffers.emplace_back(InstanceBuffer{ Magnum::GL::Buffer{}, 0 });
}

ShapeCache::ShapeCache(std::shared_ptr<BoundsCuller> culler, std::initializer_list<Part> parts,
                       std::unique_ptr<ShapeCacheShaderFlat> &&shader, BoundsCalculator bounds_calculator)
  : _culler(std::move(culler))
  , _parts(parts)
  , _shader(std::move(shader))
  , _bounds_calculator(std::move(bounds_calculator))
{
  _instance_buffers.emplace_back(InstanceBuffer{ Magnum::GL::Buffer{}, 0 });
}

void ShapeCache::calcBounds(const Magnum::Matrix4 &transform, Magnum::Vector3 &centre, Magnum::Vector3 &half_extents)
{
  _bounds_calculator(transform, centre, half_extents);
}

unsigned ShapeCache::add(const Magnum::Matrix4 &transform, const Magnum::Color4 &colour)
{
  unsigned id;
  Shape *shape = {};
  if (_free_list != kFreeListEnd)
  {
    id = _free_list;
    shape = &_shapes[id];
    _free_list = shape->free_next;
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
  Magnum::Vector3 half_extents;
  _bounds_calculator(transform, centre, half_extents);
  shape->bounds_id = _culler->allocate(centre, half_extents);
  shape->free_next = kFreeListEnd;
  return id;
}

bool ShapeCache::remove(unsigned id)
{
  if (id < _shapes.size())
  {
    Shape &shape = _shapes[id];
    if (shape.free_next == kFreeListEnd)
    {
      _culler->release(shape.bounds_id);
      shape.bounds_id = _free_list;
      _free_list = id;
      return true;
    }
  }
  return false;
}

bool ShapeCache::update(unsigned id, const Magnum::Matrix4 &transform, const Magnum::Color4 &colour)
{
  if (id < _shapes.size())
  {
    Shape &shape = _shapes[id];
    if (shape.free_next == kFreeListEnd)
    {
      shape.instance.transform = transform;
      shape.instance.colour = colour;

      Magnum::Vector3 centre;
      Magnum::Vector3 half_extents;
      _bounds_calculator(transform, centre, half_extents);
      _culler->update(shape.bounds_id, centre, half_extents);
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
    if (shape.free_next == kFreeListEnd)
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
  _free_list = kFreeListEnd;
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

  for (auto &shape : _shapes)
  {
    if (shape.free_next == kFreeListEnd && culler.isVisible(shape.bounds_id))
    {
      _marshal_buffer[_instance_buffers[cur_instance_buffer_idx].count++] = shape.instance;

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
