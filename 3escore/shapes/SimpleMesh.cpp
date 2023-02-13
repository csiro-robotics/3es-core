//
// author: Kazys Stepanas
//
#include "SimpleMesh.h"

#include <3escore/Rotation.h>

#include <mutex>
#include <vector>

namespace tes
{
struct UV
{
  float u, v;
};

struct SimpleMeshImp
{
  std::mutex lock;
  std::vector<Vector3f> vertices;
  std::vector<uint32_t> indices;
  std::vector<uint32_t> colours;
  std::vector<Vector3f> normals;
  std::vector<UV> uvs;
  Transform transform = Transform::identity(false);
  uint32_t id = 0;
  uint32_t tint = 0xffffffffu;
  unsigned components = 0;
  DrawType draw_type = DtTriangles;

  inline SimpleMeshImp(unsigned components)
    : components(components)
  {}


  [[nodiscard]] inline std::shared_ptr<SimpleMeshImp> clone() const
  {
    auto copy = std::make_shared<SimpleMeshImp>(this->components);
    copy->vertices = vertices;
    copy->indices = indices;
    copy->colours = colours;
    copy->normals = normals;
    copy->uvs = uvs;
    copy->transform = transform;
    copy->id = id;
    copy->tint = tint;
    copy->components = components;
    copy->draw_type = draw_type;
    return copy;
  }

  inline void clear(unsigned component_flags)
  {
    clearArrays();
    transform = Transform::identity(false);
    id = 0;
    tint = 0xffffffffu;
    components = component_flags;
    draw_type = DtTriangles;
  }

  inline void clearArrays()
  {
    // Should only be called if the reference count is 1.
    vertices.clear();
    indices.clear();
    colours.clear();
    normals.clear();
    uvs.clear();
  }
};


SimpleMesh::SimpleMesh(uint32_t id, size_t vertex_count, size_t index_count, DrawType draw_type,
                       unsigned components)
  : _imp(std::make_shared<SimpleMeshImp>(components))
{
  _imp->id = id;
  _imp->draw_type = draw_type;
  _imp->transform = Transform::identity(false);
  _imp->tint = 0xffffffff;

  if (vertex_count)
  {
    setVertexCount(vertex_count);
  }

  if (index_count && (components & Index))
  {
    setIndexCount(index_count);
  }
}


SimpleMesh::SimpleMesh(const SimpleMesh &other)
{
  const std::scoped_lock guard(other._imp->lock);
  _imp = other._imp;
}


SimpleMesh::~SimpleMesh() = default;


void SimpleMesh::clear()
{
  // Note: _imp may change before leaving this function, but the guard will hold
  // a reference to the correct lock.
  const std::scoped_lock guard(_imp->lock);
  if (_imp.use_count() == 1)
  {
    _imp->clear(Vertex | Index);
  }
  else
  {
    _imp = std::make_shared<SimpleMeshImp>(Vertex | Index);
  }
}


void SimpleMesh::clearData()
{
  // Note: _imp may change before leaving this function, but the guard will hold
  // a reference to the correct lock.
  const std::scoped_lock guard(_imp->lock);
  if (_imp.use_count() == 1)
  {
    _imp->clearArrays();
  }
  else
  {
    auto old = _imp;
    _imp = std::make_shared<SimpleMeshImp>(Vertex | Index);
    _imp->transform = old->transform;
    _imp->id = old->id;
    _imp->tint = old->tint;
    _imp->draw_type = old->draw_type;
    _imp->clearArrays();
  }
}


uint32_t SimpleMesh::id() const
{
  return _imp->id;
}


Transform SimpleMesh::transform() const
{
  return _imp->transform;
}


void SimpleMesh::setTransform(const Transform &transform)
{
  copyOnWrite();
  _imp->transform = transform;
}


uint32_t SimpleMesh::tint() const
{
  return _imp->tint;
}


void SimpleMesh::setTint(uint32_t tint)
{
  copyOnWrite();
  _imp->tint = tint;
}


std::shared_ptr<Resource> SimpleMesh::clone() const
{
  return std::make_shared<SimpleMesh>(*this);
}


uint8_t SimpleMesh::drawType(int /*stream*/) const
{
  return _imp->draw_type;
}


DrawType SimpleMesh::getDrawType() const
{
  return _imp->draw_type;
}


void SimpleMesh::setDrawType(DrawType type)
{
  copyOnWrite();
  _imp->draw_type = type;
}


unsigned SimpleMesh::components() const
{
  return _imp->components;
}


void SimpleMesh::setComponents(unsigned components)
{
  copyOnWrite();
  _imp->components = components | Vertex;
  // Fix up discrepencies.
  if (!(_imp->components & Index) && !_imp->indices.empty())
  {
    _imp->indices.clear();
  }

  if ((_imp->components & Colour) && _imp->colours.empty())
  {
    _imp->colours.resize(_imp->vertices.size());
  }
  else if (!(_imp->components & Colour) && !_imp->colours.empty())
  {
    _imp->colours.clear();
  }

  if ((_imp->components & Normal) && _imp->normals.empty())
  {
    _imp->normals.resize(_imp->vertices.size());
  }
  else if (!(_imp->components & Normal) && !_imp->normals.empty())
  {
    _imp->normals.clear();
  }

  if ((_imp->components & Uv) && _imp->uvs.empty())
  {
    _imp->uvs.resize(_imp->vertices.size());
  }
  else if (!(_imp->components & Uv) && !_imp->uvs.empty())
  {
    _imp->uvs.clear();
  }
}


unsigned SimpleMesh::vertexCount(int stream) const
{
  if (stream == 0)
  {
    return int_cast<unsigned>(_imp->vertices.size());
  }
  return 0;
}


void SimpleMesh::setVertexCount(size_t count)
{
  copyOnWrite();
  _imp->vertices.resize(count);
  if ((_imp->components & Colour))
  {
    _imp->colours.resize(_imp->vertices.size());
  }

  if ((_imp->components & Normal))
  {
    _imp->normals.resize(_imp->vertices.size());
  }

  if ((_imp->components & Uv))
  {
    _imp->uvs.resize(_imp->vertices.size());
  }
}


void SimpleMesh::reserveVertexCount(size_t count)
{
  copyOnWrite();
  _imp->vertices.reserve(count);
}


unsigned SimpleMesh::addVertices(const Vector3f *v, size_t count)
{
  copyOnWrite();
  const size_t offset = _imp->vertices.size();
  setVertexCount(int_cast<unsigned>(_imp->vertices.size() + count));
  for (unsigned i = 0; i < count; ++i)
  {
    _imp->vertices[offset + i] = v[i];
  }
  return int_cast<unsigned>(offset);
}


unsigned SimpleMesh::setVertices(size_t at, const Vector3f *v, size_t count)
{
  copyOnWrite();
  unsigned set = 0;
  for (size_t i = at; i < at + count && i < _imp->vertices.size(); ++i)
  {
    _imp->vertices[i] = v[i - at];
    ++set;
  }
  return set;
}


const Vector3f *SimpleMesh::rawVertices() const
{
  return _imp->vertices.data();
}


DataBuffer SimpleMesh::vertices(int stream) const
{
  if (stream == 0 && !_imp->vertices.empty())
  {
    return { _imp->vertices };
  }
  return {};
}


unsigned SimpleMesh::indexCount(int stream) const
{
  if (!stream && (_imp->components & Index) && !_imp->indices.empty())
  {
    return int_cast<unsigned>(_imp->indices.size());
  }
  return 0;
}


void SimpleMesh::setIndexCount(size_t count)
{
  copyOnWrite();
  _imp->indices.resize(count);
  if (count)
  {
    _imp->components |= Index;
  }
}


void SimpleMesh::reserveIndexCount(size_t count)
{
  copyOnWrite();
  _imp->indices.reserve(count);
}


void SimpleMesh::addIndices(const uint32_t *idx, size_t count)
{
  copyOnWrite();
  const size_t offset = _imp->indices.size();
  setIndexCount(int_cast<unsigned>(count + offset));
  for (unsigned i = 0; i < count; ++i)
  {
    _imp->indices[i + offset] = idx[i];
  }
}


unsigned SimpleMesh::setIndices(size_t at, const uint32_t *idx, size_t count)
{
  copyOnWrite();
  unsigned set = 0;
  for (size_t i = at; i < at + count && i < _imp->indices.size(); ++i)
  {
    _imp->indices[i] = idx[set++];
  }
  return set;
}


const uint32_t *SimpleMesh::rawIndices() const
{
  return _imp->indices.data();
}


DataBuffer SimpleMesh::indices(int stream) const
{
  if (stream == 0 && (_imp->components & Index) && !_imp->indices.empty())
  {
    return { _imp->indices };
  }
  return {};
}


unsigned SimpleMesh::setNormals(size_t at, const Vector3f *n, size_t count)
{
  copyOnWrite();
  unsigned set = 0;
  if (!(_imp->components & Normal) && !_imp->vertices.empty())
  {
    _imp->normals.resize(_imp->vertices.size());
    _imp->components |= Normal;
  }
  for (size_t i = at; i < at + count && i < _imp->normals.size(); ++i)
  {
    _imp->normals[i] = n[set++];
  }
  return set;
}


const Vector3f *SimpleMesh::rawNormals() const
{
  return _imp->normals.data();
}


DataBuffer SimpleMesh::normals(int stream) const
{
  if (stream == 0 && (_imp->components & Normal) && !_imp->normals.empty())
  {
    return { _imp->normals };
  }
  return {};
}


unsigned SimpleMesh::setColours(size_t at, const uint32_t *c, size_t count)
{
  copyOnWrite();
  unsigned set = 0;
  if (!(_imp->components & Colour) && !_imp->vertices.empty())
  {
    _imp->colours.resize(_imp->vertices.size());
    _imp->components |= Colour;
  }

  for (size_t i = at; i < at + count && i < _imp->colours.size(); ++i)
  {
    _imp->colours[i] = c[i - at];
    ++set;
  }
  return set;
}


const uint32_t *SimpleMesh::rawColours() const
{
  return _imp->colours.data();
}


DataBuffer SimpleMesh::colours(int stream) const
{
  if (stream == 0 && (_imp->components & Colour) && !_imp->colours.empty())
  {
    return { _imp->colours };
  }
  return {};
}


unsigned SimpleMesh::setUvs(size_t at, const float *uvs, size_t count)
{
  copyOnWrite();
  unsigned set = 0;
  if (!(_imp->components & Uv) && !_imp->vertices.empty())
  {
    _imp->uvs.resize(_imp->vertices.size());
    _imp->components |= Uv;
  }
  for (size_t i = at; i < at + count && i < _imp->uvs.size(); ++i)
  {
    const UV uv = { uvs[(i - at) * 2 + 0], uvs[(i - at) * 2 + 1] };
    _imp->uvs[i] = uv;
    ++set;
  }
  return set;
}


const float *SimpleMesh::rawUvs() const
{
  if (!_imp->uvs.empty())
  {
    return &_imp->uvs[0].u;
  }
  return nullptr;
}


DataBuffer SimpleMesh::uvs(int stream) const
{
  if (stream == 0 && (_imp->components & Uv) && !_imp->uvs.empty())
  {
    return { &_imp->uvs.data()->u, _imp->uvs.size(), 2 };
  }
  return {};
}


void SimpleMesh::copyOnWrite()
{
  const std::scoped_lock guard(_imp->lock);
  if (_imp.use_count() > 1)
  {
    _imp = _imp->clone();
  }
}


bool SimpleMesh::processCreate(const MeshCreateMessage &msg, const ObjectAttributesd &attributes)
{
  copyOnWrite();
  _imp->id = msg.mesh_id;
  setVertexCount(msg.vertex_count);
  setIndexCount(msg.index_count);
  setDrawType(static_cast<DrawType>(msg.draw_type));

  const Transform transform =
    Transform(Vector3d(attributes.position), Quaterniond(attributes.rotation),
              Vector3d(attributes.scale), msg.flags & McfDoublePrecision);

  setTransform(transform);
  setTint(attributes.colour);
  return true;
}


bool SimpleMesh::processVertices(const MeshComponentMessage &msg, unsigned offset,
                                 const DataBuffer &stream)
{
  TES_UNUSED(msg);
  copyOnWrite();
  const auto stream_count = stream.count();
  const auto vertex_count = vertexCount();
  for (size_t i = 0; i < stream_count && i + offset < vertex_count; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      _imp->vertices[i + offset][j] = stream.get<float>(i, j);
    }
  }
  return stream_count + offset <= vertex_count;
}


bool SimpleMesh::processIndices(const MeshComponentMessage &msg, unsigned offset,
                                const DataBuffer &stream)
{
  TES_UNUSED(msg);
  return setIndices(offset, stream.ptr<uint32_t>(), stream.count()) == stream.count();
}


bool SimpleMesh::processColours(const MeshComponentMessage &msg, unsigned offset,
                                const DataBuffer &stream)
{
  TES_UNUSED(msg);
  if (stream.type() == DctUInt32)
  {
    return setColours(offset, stream.ptr<uint32_t>(), stream.count()) == stream.count();
  }

  // Read RGBA byte arrays.
  if (stream.type() == DctUInt8 && stream.componentCount() == 4)
  {
    copyOnWrite();
    if (!(_imp->components & Colour) && vertexCount())
    {
      _imp->colours.resize(vertexCount());
      _imp->components |= Colour;
    }

    std::array<uint8_t, 4> rgba;
    for (unsigned i = 0; i < stream.count() && i + offset < vertexCount(); ++i)
    {
      for (unsigned j = 0; j < rgba.size(); ++j)
      {
        rgba[j] = stream.get<uint8_t>(i, j);
      }

      _imp->colours[i] = tes::Colour(rgba).colour32();
    }

    return stream.count() + offset <= vertexCount();
  }

  return false;
}


bool SimpleMesh::processNormals(const MeshComponentMessage &msg, unsigned offset,
                                const DataBuffer &stream)
{
  TES_UNUSED(msg);
  copyOnWrite();
  if (!(_imp->components & Normal) && vertexCount())
  {
    _imp->normals.resize(vertexCount());
    _imp->components |= Normal;
  }
  for (unsigned i = 0; i < stream.count() && i + offset < vertexCount(); ++i)
  {
    for (unsigned j = 0; j < 3; ++j)
    {
      _imp->normals[i + static_cast<size_t>(offset)][j] = stream.get<float>(i, j);
    }
  }
  return stream.count() + offset <= vertexCount();
}


bool SimpleMesh::processUVs(const MeshComponentMessage &msg, unsigned offset,
                            const DataBuffer &stream)
{
  TES_UNUSED(msg);
  copyOnWrite();
  if (!(_imp->components & Normal) && vertexCount())
  {
    _imp->uvs.resize(vertexCount());
    _imp->components |= Normal;
  }
  for (unsigned i = 0; i < stream.count() && i + offset < vertexCount(); ++i)
  {
    _imp->uvs[i + static_cast<size_t>(offset)].u = stream.get<float>(i, 0);
    _imp->uvs[i + static_cast<size_t>(offset)].v = stream.get<float>(i, 1);
  }
  return stream.count() + offset <= vertexCount();
}
}  // namespace tes
