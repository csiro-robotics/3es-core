//
// author: Kazys Stepanas
//
#include "3essimplemesh.h"

#include "3esrotation.h"
#include "3esspinlock.h"

#include <mutex>
#include <vector>

using namespace tes;

namespace tes
{
struct UV
{
  float u, v;
};

struct SimpleMeshImp
{
  SpinLock lock;
  std::vector<Vector3f> vertices;
  std::vector<uint32_t> indices;
  std::vector<uint32_t> colours;
  std::vector<Vector3f> normals;
  std::vector<UV> uvs;
  Transform transform;
  uint32_t id;
  uint32_t tint;
  unsigned components;
  unsigned references;
  DrawType drawType;

  inline SimpleMeshImp(unsigned components)
    : id(0)
    , tint(0xffffffffu)
    , components(components)
    , references(1)
    , drawType(DtTriangles)
  {
    transform = Transform::identity(false);
  }


  inline SimpleMeshImp *clone() const
  {
    SimpleMeshImp *copy = new SimpleMeshImp(this->components);
    copy->vertices = vertices;
    copy->indices = indices;
    copy->colours = colours;
    copy->normals = normals;
    copy->uvs = uvs;
    copy->transform = transform;
    copy->tint = tint;
    copy->components = components;
    copy->drawType = drawType;
    copy->references = 1;
    return copy;
  }

  inline void clear(unsigned componentFlags)
  {
    clearArrays();
    transform = Transform::identity(false);
    id = 0;
    tint = 0xffffffffu;
    components = componentFlags;
    drawType = DtTriangles;
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
}  // namespace tes


SimpleMesh::SimpleMesh(uint32_t id, size_t vertexCount, size_t indexCount, DrawType drawType, unsigned components)
  : _imp(new SimpleMeshImp(components))
{
  _imp->id = id;
  _imp->drawType = drawType;
  _imp->transform = Transform::identity(false);
  _imp->tint = 0xffffffff;

  if (vertexCount)
  {
    setVertexCount(vertexCount);
  }

  if (indexCount && (components & Index))
  {
    setIndexCount(indexCount);
  }
}


SimpleMesh::SimpleMesh(const SimpleMesh &other)
  : _imp(other._imp)
{
  std::unique_lock<SpinLock> guard(_imp->lock);
  ++_imp->references;
}


SimpleMesh::~SimpleMesh()
{
  std::unique_lock<SpinLock> guard(_imp->lock);
  if (_imp->references == 1)
  {
    // Unlock for delete.
    guard.unlock();
    delete _imp;
  }
  else
  {
    --_imp->references;
  }
}


void SimpleMesh::clear()
{
  // Note: _imp may change before leaving this function, but the guard will hold
  // a reference to the correct lock.
  std::unique_lock<SpinLock> guard(_imp->lock);
  if (_imp->references == 1)
  {
    _imp->clear(Vertex | Index);
  }
  else
  {
    --_imp->references;
    _imp = new SimpleMeshImp(Vertex | Index);
  }
}


void SimpleMesh::clearData()
{
  // Note: _imp may change before leaving this function, but the guard will hold
  // a reference to the correct lock.
  std::unique_lock<SpinLock> guard(_imp->lock);
  if (_imp->references == 1)
  {
    _imp->clearArrays();
  }
  else
  {
    SimpleMeshImp *old = _imp;
    --_imp->references;
    _imp = new SimpleMeshImp(Vertex | Index);
    *_imp = *old;
    _imp->references = 1;
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


SimpleMesh *SimpleMesh::clone() const
{
  return new SimpleMesh(*this);
}


uint8_t SimpleMesh::drawType(int /*stream*/) const
{
  return _imp->drawType;
}


DrawType SimpleMesh::getDrawType() const
{
  return _imp->drawType;
}


void SimpleMesh::setDrawType(DrawType type)
{
  copyOnWrite();
  _imp->drawType = type;
}


unsigned SimpleMesh::components() const
{
  return _imp->components;
}


void SimpleMesh::setComponents(unsigned comps)
{
  copyOnWrite();
  _imp->components = comps | Vertex;
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


unsigned SimpleMesh::vertexCount() const
{
  return unsigned(_imp->vertices.size());
}


unsigned SimpleMesh::vertexCount(int stream) const
{
  if (stream == 0)
  {
    return unsigned(_imp->vertices.size());
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
  size_t offset = _imp->vertices.size();
  setVertexCount(unsigned(_imp->vertices.size() + count));
  for (unsigned i = 0; i < count; ++i)
  {
    _imp->vertices[offset + i] = v[i];
  }
  return unsigned(offset);
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


const Vector3f *SimpleMesh::vertices() const
{
  return _imp->vertices.data();
}


VertexBuffer SimpleMesh::vertices(int stream) const
{
  if (stream == 0 && !_imp->vertices.empty())
  {
    return VertexBuffer(_imp->vertices);
  }
  return VertexBuffer();
}


unsigned SimpleMesh::indexCount() const
{
  return unsigned(_imp->indices.size());
}


unsigned SimpleMesh::indexCount(int stream) const
{
  if (!stream && (_imp->components & Index) && !_imp->indices.empty())
  {
    return unsigned(_imp->indices.size());
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
  size_t offset = _imp->indices.size();
  setIndexCount(unsigned(count + offset));
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


const uint32_t *SimpleMesh::indices() const
{
  return _imp->indices.data();
}


VertexBuffer SimpleMesh::indices(int stream) const
{
  if (stream == 0 && (_imp->components & Index) && !_imp->indices.empty())
  {
    return VertexBuffer(_imp->indices);
  }
  return VertexBuffer();
}


unsigned SimpleMesh::setNormals(size_t at, const Vector3f *n, size_t count)
{
  copyOnWrite();
  unsigned set = 0;
  if (!(_imp->components & Normal) && _imp->vertices.size())
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


const Vector3f *SimpleMesh::normals() const
{
  return _imp->normals.data();
}


VertexBuffer SimpleMesh::normals(int stream) const
{
  if (stream == 0 && (_imp->components & Normal) && !_imp->normals.empty())
  {
    return VertexBuffer(_imp->normals);
  }
  return VertexBuffer();
}


unsigned SimpleMesh::setColours(size_t at, const uint32_t *c, size_t count)
{
  copyOnWrite();
  unsigned set = 0;
  if (!(_imp->components & Colour) && _imp->vertices.size())
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


const uint32_t *SimpleMesh::colours() const
{
  return _imp->colours.data();
}


VertexBuffer SimpleMesh::colours(int stream) const
{
  if (stream == 0 && (_imp->components & Colour) && !_imp->colours.empty())
  {
    return VertexBuffer(_imp->colours);
  }
  return VertexBuffer();
}


unsigned SimpleMesh::setUvs(size_t at, const float *uvs, size_t count)
{
  copyOnWrite();
  unsigned set = 0;
  if (!(_imp->components & Uv) && _imp->vertices.size())
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


const float *SimpleMesh::uvs() const
{
  if (!_imp->uvs.empty())
  {
    return &_imp->uvs[0].u;
  }
  return nullptr;
}


VertexBuffer SimpleMesh::uvs(int stream) const
{
  if (stream == 0 && (_imp->components & Uv) && !_imp->uvs.empty())
  {
    return VertexBuffer(&_imp->uvs.data()->u, _imp->uvs.size(), 2);
  }
  return VertexBuffer();
}


void SimpleMesh::copyOnWrite()
{
  std::unique_lock<SpinLock> guard(_imp->lock);
  if (_imp->references > 1)
  {
    --_imp->references;
    _imp = _imp->clone();
  }
}


bool SimpleMesh::processCreate(const MeshCreateMessage &msg, const ObjectAttributesd &attributes)
{
  copyOnWrite();
  _imp->id = msg.meshId;
  setVertexCount(msg.vertexCount);
  setIndexCount(msg.indexCount);
  setDrawType((DrawType)msg.drawType);

  Transform transform = Transform(Vector3d(attributes.position), Quaterniond(attributes.rotation),
                                  Vector3d(attributes.scale), msg.flags & McfDoublePrecision);

  setTransform(transform);
  setTint(attributes.colour);
  return true;
}


bool SimpleMesh::processVertices(const MeshComponentMessage &msg, unsigned offset, const VertexBuffer &stream)
{
  copyOnWrite();
  for (unsigned i = 0; i < stream.count() && i + offset < vertexCount(); ++i)
  {
    for (unsigned j = 0; j < 3; ++j)
    {
      _imp->vertices[i + offset][j] = stream.get<float>(i, j);
    }
  }
  return stream.count() + offset < vertexCount();
}


bool SimpleMesh::processIndices(const MeshComponentMessage &msg, unsigned offset, const VertexBuffer &stream)
{
  return setIndices(offset, stream.ptr<uint32_t>(), stream.count());
}


bool SimpleMesh::processColours(const MeshComponentMessage &msg, unsigned offset, const VertexBuffer &stream)
{
  return setColours(offset, stream.ptr<uint32_t>(), stream.count());
}


bool SimpleMesh::processNormals(const MeshComponentMessage &msg, unsigned offset, const VertexBuffer &stream)
{
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
      _imp->normals[i + offset][j] = stream.get<float>(i, j);
    }
  }
  return stream.count() + offset < vertexCount();
}


bool SimpleMesh::processUVs(const MeshComponentMessage &msg, unsigned offset, const VertexBuffer &stream)
{
  copyOnWrite();
  if (!(_imp->components & Normal) && vertexCount())
  {
    _imp->uvs.resize(vertexCount());
    _imp->components |= Normal;
  }
  for (unsigned i = 0; i < stream.count() && i + offset < vertexCount(); ++i)
  {
    _imp->uvs[i + offset].u = stream.get<float>(i, 0);
    _imp->uvs[i + offset].v = stream.get<float>(i, 1);
  }
  return stream.count() + offset < vertexCount();
}
