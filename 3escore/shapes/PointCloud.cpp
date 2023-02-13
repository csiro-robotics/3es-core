//
// author: Kazys Stepanas
//
#include "PointCloud.h"

#include <3escore/MeshMessages.h>
#include <3escore/Rotation.h>

#include <algorithm>
#include <cstring>
#include <iterator>
#include <mutex>

namespace tes
{
struct PointCloudImp
{
  std::mutex lock;
  std::vector<Vector3f> vertices;
  std::vector<Vector3f> normals;
  std::vector<Colour> colours;
  uint32_t id;

  PointCloudImp(uint32_t id)
    : id(id)
  {}


  [[nodiscard]] std::shared_ptr<PointCloudImp> clone() const
  {
    auto copy = std::make_shared<PointCloudImp>(this->id);
    copy->id = id;
    copy->vertices = vertices;
    copy->normals = normals;
    copy->colours = colours;
    return copy;
  }
};

PointCloud::PointCloud(uint32_t id)
  : _imp(std::make_shared<PointCloudImp>(id))
{}


PointCloud::PointCloud(const PointCloud &other)
{
  const std::scoped_lock guard(other._imp->lock);
  _imp = other._imp;
}


PointCloud::~PointCloud() = default;


uint32_t PointCloud::id() const
{
  return _imp->id;
}


std::shared_ptr<Resource> PointCloud::clone() const
{
  auto copy = std::make_shared<PointCloud>(*this);
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
  const std::scoped_lock guard(_imp->lock);
  if (_imp->vertices.capacity() < size.i)
  {
    setCapacity(size);
  }
}


void PointCloud::resize(const UIntArg &count)
{
  const std::scoped_lock guard(_imp->lock);
  _imp->vertices.resize(count.i);
  _imp->normals.resize(count.i);
  _imp->colours.resize(count.i);
}


void PointCloud::squeeze()
{
  const std::scoped_lock guard(_imp->lock);
  _imp->vertices.shrink_to_fit();
  _imp->normals.shrink_to_fit();
  _imp->colours.shrink_to_fit();
}


unsigned PointCloud::capacity() const
{
  const std::scoped_lock guard(_imp->lock);
  return int_cast<unsigned>(_imp->vertices.capacity());
}


unsigned PointCloud::vertexCount(int stream) const
{
  const std::scoped_lock guard(_imp->lock);
  TES_UNUSED(stream);
  return int_cast<unsigned>(_imp->vertices.size());
}


DataBuffer PointCloud::vertices(int stream) const
{
  const std::scoped_lock guard(_imp->lock);
  TES_UNUSED(stream);
  return { _imp->vertices };
}


const Vector3f *PointCloud::rawVertices() const
{
  const std::scoped_lock guard(_imp->lock);
  return _imp->vertices.data();
}


unsigned PointCloud::indexCount(int stream) const
{
  TES_UNUSED(stream);
  return 0;
}


DataBuffer PointCloud::indices(int stream) const
{
  TES_UNUSED(stream);
  return {};
}


DataBuffer PointCloud::normals(int stream) const
{
  const std::scoped_lock guard(_imp->lock);
  TES_UNUSED(stream);
  return { _imp->normals };
}


const Vector3f *PointCloud::rawNormals() const
{
  const std::scoped_lock guard(_imp->lock);
  return _imp->normals.data();
}


DataBuffer PointCloud::colours(int stream) const
{
  const std::scoped_lock guard(_imp->lock);
  TES_UNUSED(stream);
  return { _imp->colours };
}


const Colour *PointCloud::rawColours() const
{
  const std::scoped_lock guard(_imp->lock);
  return _imp->colours.data();
}


DataBuffer PointCloud::uvs(int /*stream*/) const
{
  return {};
}


void PointCloud::addPoints(const Vector3f *points, const UIntArg &count)
{
  if (count)
  {
    const std::scoped_lock guard(_imp->lock);
    copyOnWrite();

    const Colour white = Colour(Colour::White);
    for (size_t i = 0; i < count.i; ++i)
    {
      _imp->vertices.emplace_back(points[i]);
      _imp->normals.emplace_back(Vector3f::Zero);
      _imp->colours.emplace_back(white);
    }
  }
}


void PointCloud::addPoints(const Vector3f *points, const Vector3f *normals, const UIntArg &count)
{
  if (count)
  {
    const std::scoped_lock guard(_imp->lock);
    copyOnWrite();

    const Colour white = Colour(Colour::White);
    for (size_t i = 0; i < count.i; ++i)
    {
      _imp->vertices.emplace_back(points[i]);
      _imp->normals.emplace_back(normals[i]);
      _imp->colours.emplace_back(white);
    }
  }
}


void PointCloud::addPoints(const Vector3f *points, const Vector3f *normals, const Colour *colours,
                           const UIntArg &count)
{
  if (count)
  {
    const std::scoped_lock guard(_imp->lock);
    copyOnWrite();

    for (size_t i = 0; i < count.i; ++i)
    {
      _imp->vertices.emplace_back(points[i]);
      _imp->normals.emplace_back(normals[i]);
      _imp->colours.emplace_back(colours[i]);
    }
  }
}


void PointCloud::setNormal(const UIntArg &index, const Vector3f &normal)
{
  const std::scoped_lock guard(_imp->lock);
  if (index.i < _imp->normals.size())
  {
    copyOnWrite();
    _imp->normals[index.i] = normal;
  }
}


void PointCloud::setColour(const UIntArg &index, const Colour &colour)
{
  const std::scoped_lock guard(_imp->lock);
  if (index.i < _imp->colours.size())
  {
    copyOnWrite();
    _imp->colours[index.i] = colour;
  }
}


void PointCloud::setPoints(const UIntArg &index, const Vector3f *points, const UIntArg &count)
{
  const std::scoped_lock guard(_imp->lock);
  if (index.i >= _imp->vertices.size())
  {
    return;
  }

  size_t limited_count = count.i;
  if (index.i + limited_count > _imp->vertices.size())
  {
    limited_count = index.i + count.i - _imp->vertices.size();
  }

  if (!limited_count)
  {
    return;
  }

  copyOnWrite();
  std::copy(points, points + limited_count, _imp->vertices.begin() + index.i);
}


void PointCloud::setPoints(const UIntArg &index, const Vector3f *points, const Vector3f *normals,
                           const UIntArg &count)
{
  const std::scoped_lock guard(_imp->lock);
  if (index.i >= _imp->vertices.size())
  {
    return;
  }

  size_t limited_count = count.i;
  if (index.i + limited_count > _imp->vertices.size())
  {
    limited_count = index.i + count.i - _imp->vertices.size();
  }

  if (!limited_count)
  {
    return;
  }

  copyOnWrite();
  std::copy(points, points + limited_count, _imp->vertices.begin() + index.i);
  std::copy(normals, normals + limited_count, _imp->normals.begin() + index.i);
}


void PointCloud::setPoints(const UIntArg &index, const Vector3f *points, const Vector3f *normals,
                           const Colour *colours, const UIntArg &count)
{
  const std::scoped_lock guard(_imp->lock);
  if (index.i >= _imp->vertices.size())
  {
    return;
  }

  size_t limited_count = count.i;
  if (index.i + limited_count > _imp->vertices.size())
  {
    limited_count = index.i + count.i - _imp->vertices.size();
  }

  if (!limited_count)
  {
    return;
  }

  copyOnWrite();
  std::copy(points, points + limited_count, _imp->vertices.begin() + index.i);
  std::copy(normals, normals + limited_count, _imp->normals.begin() + index.i);
  std::copy(colours, colours + limited_count, _imp->colours.begin() + index.i);
}


void PointCloud::setCapacity(unsigned capacity)
{
  copyOnWrite();
  // Check capacity again. The copyOnWrite() may have set them to be the same.
  if (_imp->vertices.capacity() != capacity)
  {
    _imp->vertices.reserve(capacity);
    _imp->normals.reserve(capacity);
    _imp->colours.reserve(capacity);
  }
}


void PointCloud::copyOnWrite()
{
  if (_imp.use_count() > 1)
  {
    _imp = _imp->clone();
  }
}


bool PointCloud::processCreate(const MeshCreateMessage &msg,
                               const ObjectAttributes<double> &attributes)
{
  if (msg.draw_type != DtPoints)
  {
    return false;
  }

  const std::scoped_lock guard(_imp->lock);
  copyOnWrite();
  _imp->id = msg.mesh_id;

  _imp->vertices.resize(msg.vertex_count);
  _imp->normals.resize(msg.vertex_count);
  _imp->colours.resize(msg.vertex_count);

  const Transform transform(Vector3d(attributes.position), Quaterniond(attributes.rotation),
                            Vector3d(attributes.scale), msg.flags & McfDoublePrecision);

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


bool PointCloud::processVertices(const MeshComponentMessage &msg, unsigned offset,
                                 const DataBuffer &stream)
{
  TES_UNUSED(msg);
  static_assert(sizeof(Vector3f) == sizeof(float) * 3, "Vertex size mismatch");
  const std::scoped_lock guard(_imp->lock);
  copyOnWrite();
  unsigned wrote = 0;

  for (size_t i = 0; i + offset < _imp->vertices.size() && i < stream.count(); ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      _imp->vertices[i + offset][j] = stream.get<float>(i, j);
    }
    ++wrote;
  }

  return wrote == stream.count();
}


bool PointCloud::processColours(const MeshComponentMessage &msg, unsigned offset,
                                const DataBuffer &stream)
{
  TES_UNUSED(msg);
  const std::scoped_lock guard(_imp->lock);
  copyOnWrite();
  unsigned wrote = 0;

  for (size_t i = 0; i + offset < _imp->vertices.size() && i < stream.count(); ++i)
  {
    _imp->colours[i + offset] = Colour(stream.get<uint8_t>(i, 0), stream.get<uint8_t>(i, 1),
                                       stream.get<uint8_t>(i, 2), stream.get<uint8_t>(i, 3));
    ++wrote;
  }

  return wrote == stream.count();
}


bool PointCloud::processNormals(const MeshComponentMessage &msg, unsigned offset,
                                const DataBuffer &stream)
{
  TES_UNUSED(msg);
  static_assert(sizeof(Vector3f) == sizeof(float) * 3, "Normal size mismatch");

  const std::scoped_lock guard(_imp->lock);
  copyOnWrite();
  unsigned wrote = 0;

  for (size_t i = 0; i + offset < _imp->vertices.size() && i < stream.count(); ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      _imp->normals[i + offset][j] = stream.get<float>(i, j);
    }
    ++wrote;
  }

  return wrote == stream.count();
}
}  // namespace tes
