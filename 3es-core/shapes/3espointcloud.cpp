//
// author: Kazys Stepanas
//
#include "3espointcloud.h"

#include "3esspinlock.h"

#include "3esmeshmessages.h"
#include "3esrotation.h"

#include <algorithm>
#include <cstring>
#include <mutex>

using namespace tes;

namespace tes
{
struct PointCloudImp
{
  SpinLock lock;
  Vector3f *vertices;
  Vector3f *normals;
  Colour *colours;
  unsigned vertexCount;
  unsigned capacity;
  uint32_t id;
  unsigned references;

  inline PointCloudImp(uint32_t id)
    : vertices(nullptr)
    , normals(nullptr)
    , colours(nullptr)
    , vertexCount(0)
    , capacity(0)
    , id(id)
    , references(1)
  {}


  inline ~PointCloudImp()
  {
    delete[] vertices;
    delete[] normals;
    delete[] colours;
  }


  inline PointCloudImp *clone() const
  {
    PointCloudImp *copy = new PointCloudImp(this->id);
    copy->vertexCount = copy->capacity = vertexCount;
    copy->id = id;

    copy->vertices = (vertices && vertexCount) ? new Vector3f[vertexCount] : nullptr;
    memcpy(copy->vertices, vertices, sizeof(*vertices) * vertexCount);

    copy->normals = (normals && vertexCount) ? new Vector3f[vertexCount] : nullptr;
    memcpy(copy->normals, normals, sizeof(*normals) * vertexCount);

    copy->colours = (colours && vertexCount) ? new Colour[vertexCount] : nullptr;
    memcpy(copy->colours, colours, sizeof(*colours) * vertexCount);

    copy->references = 1;
    return copy;
  }
};
}  // namespace tes

PointCloud::PointCloud(const PointCloud &other)
  : _imp(other._imp)
{
  std::unique_lock<SpinLock> guard(_imp->lock);
  ++_imp->references;
}


PointCloud::PointCloud(uint32_t id)
  : _imp(new PointCloudImp(id))
{}


PointCloud::~PointCloud()
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


uint32_t PointCloud::id() const
{
  return _imp->id;
}


PointCloud *PointCloud::clone() const
{
  PointCloud *copy = new PointCloud(*this);
  return copy;
}


Transform PointCloud::transform() const
{
  return Transform::identity(false);
}


uint32_t PointCloud::tint() const
{
  return 0xffffffffu;
}


uint8_t PointCloud::drawType(int stream) const
{
  TES_UNUSED(stream);
  return DtPoints;
}


void PointCloud::reserve(const UIntArg &size)
{
  if (_imp->capacity < size)
  {
    setCapacity(size);
  }
}


void PointCloud::resize(const UIntArg &count)
{
  if (_imp->capacity < count)
  {
    reserve(count);
  }

  _imp->vertexCount = count;
}


void PointCloud::squeeze()
{
  if (_imp->capacity > _imp->vertexCount)
  {
    setCapacity(_imp->vertexCount);
  }
}


unsigned PointCloud::capacity() const
{
  return _imp->capacity;
}


unsigned PointCloud::vertexCount(int stream) const
{
  TES_UNUSED(stream);
  return _imp->vertexCount;
}


VertexBuffer PointCloud::vertices(int stream) const
{
  TES_UNUSED(stream);
  return VertexBuffer(_imp->vertices, _imp->vertexCount);
}


const Vector3f *PointCloud::vertices() const
{
  return _imp->vertices;
}


unsigned PointCloud::indexCount(int stream) const
{
  TES_UNUSED(stream);
  return 0;
}


VertexBuffer PointCloud::indices(int stream) const
{
  TES_UNUSED(stream);
  return VertexBuffer();
}


VertexBuffer PointCloud::normals(int stream) const
{
  TES_UNUSED(stream);
  return VertexBuffer(_imp->normals, _imp->normals ? _imp->vertexCount : 0);
}


const Vector3f *PointCloud::normals() const
{
  return _imp->normals;
}


VertexBuffer PointCloud::colours(int stream) const
{
  TES_UNUSED(stream);
  return VertexBuffer(_imp->colours, _imp->colours ? _imp->vertexCount : 0);
}


const Colour *PointCloud::colours() const
{
  return _imp->colours;
}


VertexBuffer PointCloud::uvs(int) const
{
  return VertexBuffer();
}


void PointCloud::addPoints(const Vector3f *points, const UIntArg &count)
{
  if (count)
  {
    copyOnWrite();
    unsigned initial = _imp->vertexCount;
    resize(_imp->vertexCount + count.i);
    memcpy(_imp->vertices + initial, points, sizeof(*points) * count.i);

    // Initialise other data
    for (unsigned i = initial; i < _imp->vertexCount; ++i)
    {
      _imp->normals[i] = Vector3f::zero;
    }

    const Colour c = Colour::Colours[Colour::White];
    for (unsigned i = initial; i < _imp->vertexCount; ++i)
    {
      _imp->colours[i] = c;
    }
  }
}


void PointCloud::addPoints(const Vector3f *points, const Vector3f *normals, const UIntArg &count)
{
  if (count)
  {
    copyOnWrite();
    unsigned initial = _imp->vertexCount;
    resize(_imp->vertexCount + count.i);
    memcpy(_imp->vertices + initial, points, sizeof(*points) * count.i);
    memcpy(_imp->normals + initial, normals, sizeof(*normals) * count.i);

    // Initialise other data
    const Colour c = Colour::Colours[Colour::White];
    for (unsigned i = initial; i < _imp->vertexCount; ++i)
    {
      _imp->colours[i] = c;
    }
  }
}


void PointCloud::addPoints(const Vector3f *points, const Vector3f *normals, const Colour *colours, const UIntArg &count)
{
  if (count)
  {
    copyOnWrite();
    unsigned initial = _imp->vertexCount;
    resize(_imp->vertexCount + count.i);
    memcpy(_imp->vertices + initial, points, sizeof(*points) * count.i);
    memcpy(_imp->normals + initial, normals, sizeof(*normals) * count.i);
    memcpy(_imp->colours + initial, colours, sizeof(*colours) * count.i);
  }
}


void PointCloud::setNormal(const UIntArg &index, const Vector3f &normal)
{
  if (index < _imp->vertexCount)
  {
    copyOnWrite();
    _imp->normals[index.i] = normal;
  }
}


void PointCloud::setColour(const UIntArg &index, const Colour &colour)
{
  if (index < _imp->vertexCount)
  {
    copyOnWrite();
    _imp->colours[index.i] = colour;
  }
}


void PointCloud::setPoints(const UIntArg &index, const Vector3f *points, const UIntArg &count)
{
  if (index >= _imp->vertexCount)
  {
    return;
  }

  unsigned limitedCount = count;
  if (index.i + limitedCount > _imp->vertexCount)
  {
    limitedCount = index.i + count.i - _imp->vertexCount;
  }

  if (!limitedCount)
  {
    return;
  }

  copyOnWrite();
  memcpy(_imp->vertices + index.i, points, sizeof(*points) * limitedCount);
}


void PointCloud::setPoints(const UIntArg &index, const Vector3f *points, const Vector3f *normals, const UIntArg &count)
{
  if (index >= _imp->vertexCount)
  {
    return;
  }

  unsigned limitedCount = count;
  if (index.i + limitedCount > _imp->vertexCount)
  {
    limitedCount = index.i + count.i - _imp->vertexCount;
  }

  if (!limitedCount)
  {
    return;
  }

  copyOnWrite();
  memcpy(_imp->vertices + index.i, points, sizeof(*points) * limitedCount);
  memcpy(_imp->normals + index.i, normals, sizeof(*normals) * limitedCount);
}


void PointCloud::setPoints(const UIntArg &index, const Vector3f *points, const Vector3f *normals, const Colour *colours,
                           const UIntArg &count)
{
  if (index >= _imp->vertexCount)
  {
    return;
  }

  unsigned limitedCount = count;
  if (index.i + limitedCount > _imp->vertexCount)
  {
    limitedCount = index.i + count.i - _imp->vertexCount;
  }

  if (!limitedCount)
  {
    return;
  }

  copyOnWrite();
  memcpy(_imp->vertices + index.i, points, sizeof(*points) * limitedCount);
  memcpy(_imp->normals + index.i, normals, sizeof(*normals) * limitedCount);
  memcpy(_imp->colours + index.i, colours, sizeof(*colours) * limitedCount);
}


void PointCloud::setCapacity(unsigned size)
{
  if (_imp->capacity == size)
  {
    // Already at the requested size.
    return;
  }

  copyOnWrite();
  // Check capacity again. The copyOnWrite() may have set them to be the same.
  if (_imp->capacity != size)
  {
    if (!size)
    {
      delete[] _imp->vertices;
      delete[] _imp->normals;
      delete[] _imp->colours;
      _imp->vertices = _imp->normals = nullptr;
      _imp->colours = nullptr;
      _imp->capacity = 0;
      _imp->vertexCount = 0;
      return;
    }

    Vector3f *points = new Vector3f[size];
    Vector3f *normals = new Vector3f[size];
    Colour *colours = new Colour[size];

    unsigned vertexCount = std::min(_imp->vertexCount, size);
    if (_imp->capacity)
    {
      // Copy existing data.
      if (vertexCount)
      {
        memcpy(points, _imp->vertices, sizeof(*points) * vertexCount);
        memcpy(normals, _imp->normals, sizeof(*normals) * vertexCount);
        memcpy(colours, _imp->colours, sizeof(*colours) * vertexCount);
      }

      delete[] _imp->vertices;
      delete[] _imp->normals;
      delete[] _imp->colours;
    }

    _imp->vertices = points;
    _imp->normals = normals;
    _imp->colours = colours;
    _imp->capacity = size;
    _imp->vertexCount = vertexCount;
  }
}


void PointCloud::copyOnWrite()
{
  std::unique_lock<SpinLock> guard(_imp->lock);
  if (_imp->references > 1)
  {
    --_imp->references;
    _imp = _imp->clone();
  }
}


bool PointCloud::processCreate(const MeshCreateMessage &msg, const ObjectAttributes<double> &attributes)
{
  if (msg.drawType != DtPoints)
  {
    return false;
  }

  copyOnWrite();
  _imp->id = msg.meshId;

  _imp->vertexCount = msg.vertexCount;
  delete _imp->vertices;
  delete _imp->normals;
  delete _imp->colours;
  _imp->capacity = msg.vertexCount;
  _imp->vertices = new Vector3f[msg.vertexCount];
  _imp->normals = nullptr;  // Pending.
  _imp->colours = nullptr;  // Pending

  Transform transform(Vector3d(attributes.position), Quaterniond(attributes.rotation), Vector3d(attributes.scale),
                      msg.flags & McfDoublePrecision);

  // Does not accept a transform.
  if (!transform.isEqual(Transform::identity()))
  {
    return false;
  }

  // Does not accept a tint.
  if (attributes.colour != 0xffffffffu)
  {
    return false;
  }

  return true;
}


bool PointCloud::processVertices(const MeshComponentMessage &msg, const VertexBuffer &stream)
{
  static_assert(sizeof(Vector3f) == sizeof(float) * 3, "Vertex size mismatch");
  copyOnWrite();
  unsigned wrote = 0;

  for (unsigned i = 0; i + msg.offset < _imp->vertexCount && i < msg.count; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      _imp->vertices[i + msg.offset][j] = stream.get<float>(i, j);
    }
    ++wrote;
  }

  return wrote == stream.count();
}


bool PointCloud::processColours(const MeshComponentMessage &msg, const VertexBuffer &stream)
{
  copyOnWrite();
  unsigned wrote = 0;
  if (_imp->colours == nullptr)
  {
    _imp->colours = new Colour[_imp->vertexCount];
  }

  for (unsigned i = 0; i + msg.offset < _imp->vertexCount && i < msg.count; ++i)
  {
    _imp->colours[i + msg.offset] = stream.get<uint32_t>(i);;
    ++wrote;
  }

  return wrote == stream.count();
}


bool PointCloud::processNormals(const MeshComponentMessage &msg, const VertexBuffer &stream)
{
  static_assert(sizeof(Vector3f) == sizeof(float) * 3, "Normal size mismatch");

  copyOnWrite();
  unsigned wrote = 0;
  if (_imp->normals == nullptr)
  {
    _imp->normals = new Vector3f[_imp->vertexCount];
  }

  for (unsigned i = 0; i + msg.offset < _imp->vertexCount && i < msg.count; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      _imp->normals[i + msg.offset][j] = stream.get<float>(i, j);
    }
    ++wrote;
  }

  return wrote == stream.count();
}
