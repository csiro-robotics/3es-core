#include "3esshapecache.h"

#include "3esbounds.h"

namespace tes
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
                              Magnum::Shaders::Flat3D::Color3{});
  _shader.draw(mesh);
}


ShapeCacheShaderWireframe::ShapeCacheShaderWireframe()
  : _shader(Magnum::Shaders::Flat3D::Flag::VertexColor | Magnum::Shaders::Flat3D::Flag::InstancedTransformation)
{}


ShapeCacheShaderWireframe::~ShapeCacheShaderWireframe() = default;


void ShapeCacheShaderWireframe::setProjectionMatrix(const Magnum::Matrix4 &projection)
{
  _shader.setTransformationProjectionMatrix(projection);
}

void ShapeCacheShaderWireframe::draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer, size_t instance_count)
{
  mesh.setInstanceCount(instance_count)
    .addVertexBufferInstanced(buffer, 1, 0, Magnum::Shaders::Flat3D::TransformationMatrix{},
                              Magnum::Shaders::Flat3D::Color3{});
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


ShapeCache::ShapeCache(Type type, std::shared_ptr<BoundsCuller> culler, Magnum::GL::Mesh &&mesh,
                       const Magnum::Matrix4 &mesh_transform, const Magnum::Vector3 &half_extents,
                       std::unique_ptr<ShapeCacheShaderFlat> &&shader, BoundsCalculator bounds_calculator)
  : _culler(std::move(culler))
  , _mesh(std::move(mesh))
  , _mesh_transform(mesh_transform)
  , _half_extents(half_extents)
  , _shader(std::move(shader))
  , _bounds_calculator(std::move(bounds_calculator))
  , _type(type)
{
  _instance_buffers.emplace_back(InstanceBuffer{ Magnum::GL::Buffer{}, 0 });
}

void ShapeCache::calcBounds(const Magnum::Matrix4 &transform, Magnum::Vector3 &centre, Magnum::Vector3 &half_extents)
{
  _bounds_calculator(transform, centre, half_extents);
}

unsigned ShapeCache::add(const Magnum::Matrix4 &transform, const Magnum::Color3 &colour)
{
  unsigned id;
  Shape *shape = {};
  if (_free_list != kFreeListEnd)
  {
    id = _free_list;
    shape = &_shapes[id];
    _free_list = freeListNext(*shape);
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

void ShapeCache::remove(unsigned id)
{
  Shape &shape = _shapes.at(id);
  _culler->release(shape.bounds_id);
  shape.bounds_id = _free_list;
  _free_list = id;
}

void ShapeCache::update(unsigned id, const Magnum::Matrix4 &transform, const Magnum::Color3 &colour)
{
  Shape &shape = _shapes.at(id);
  shape.instance.transform = transform;
  shape.instance.colour = colour;

  Magnum::Vector3 centre;
  Magnum::Vector3 half_extents;
  _bounds_calculator(transform, centre, half_extents);
  _culler->update(shape.bounds_id, centre, half_extents);
}

void ShapeCache::draw(unsigned render_mark, const Magnum::Matrix4 &projection_matrix)
{
  buildInstanceBuffers(render_mark);
  const Magnum::Matrix4 projection = projection_matrix * _mesh_transform;
  _shader->setProjectionMatrix(projection);
  for (auto &buffer : _instance_buffers)
  {
    if (buffer.count)
    {
      _shader->draw(_mesh, buffer.buffer, buffer.count);
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
}  // namespace tes
}  // namespace tes
