//
// author: Kazys Stepanas
//
#include "PointCloudShape.h"

#include "MeshPlaceholder.h"

#include <3escore/CoreUtil.h>
#include <3escore/PacketWriter.h>

#include <algorithm>

namespace tes
{
PointCloudShape::~PointCloudShape()
{
  freeIndices(_indices);
  if (_own_mesh)
  {
    delete _mesh;
  }
}


bool PointCloudShape::writeCreate(PacketWriter &stream) const
{
  bool ok = Shape::writeCreate(stream);
  // Write the point cloud ID.
  uint32_t value_u32 = _mesh->id();
  ok = stream.writeElement(value_u32) == sizeof(value_u32) && ok;
  // Write the index count.
  value_u32 = _index_count;
  ok = stream.writeElement(value_u32) == sizeof(value_u32) && ok;
  // Write point size.
  ok = stream.writeElement(_point_scale) == sizeof(_point_scale) && ok;
  return ok;
}


int PointCloudShape::writeData(PacketWriter &stream, unsigned &progress_marker) const
{
  // Max items based on packet size of 0xffff, minus some overhead divide by index size.
  const uint32_t max_items = ((0xffffu - 256u) / 4u);
  DataMessage msg;
  bool ok = true;
  stream.reset(routingId(), DataMessage::MessageId);
  msg.id = id();
  msg.write(stream);

  // Write indices for this view into the cloud.
  const uint32_t offset = progress_marker;
  uint32_t count = _index_count - progress_marker;
  if (count > max_items)
  {
    count = max_items;
  }

  // Use 32-bits for both values though count will never need greater than 16-bit.
  ok = stream.writeElement(offset) == sizeof(offset) && ok;
  ok = stream.writeElement(count) == sizeof(count) && ok;

  if (count)
  {
    ok = stream.writeArray(_indices + offset, count) == count && ok;
  }

  if (!ok)
  {
    return -1;
  }

  progress_marker += count;
  return (progress_marker < _index_count) ? 1 : 0;
}


bool PointCloudShape::readCreate(PacketReader &stream)
{
  if (!Shape::readCreate(stream))
  {
    return false;
  }

  bool ok = true;

  uint32_t value_u32 = 0;

  // Mesh ID.
  ok = ok && stream.readElement(value_u32) == sizeof(value_u32);
  if (_own_mesh)
  {
    delete _mesh;
  }
  _mesh = new MeshPlaceholder(value_u32);
  _own_mesh = true;

  // Index count.
  ok = ok && stream.readElement(value_u32) == sizeof(value_u32);
  if (_index_count < value_u32)
  {
    freeIndices(_indices);
    _indices = allocateIndices(value_u32);
  }
  _index_count = value_u32;

  // Point size.
  if (stream.versionMajor() > 0 || stream.versionMajor() == 0 && stream.versionMinor() >= 2)
  {
    ok = ok && stream.readElement(_point_scale) == sizeof(_point_scale);
  }
  else
  {
    // Legacy support.
    uint8_t point_size = 0;
    ok = ok && stream.readElement(point_size) == sizeof(point_size);
    _point_scale = static_cast<float>(point_size);
  }

  return ok;
}


bool PointCloudShape::readData(PacketReader &stream)
{
  DataMessage msg;
  bool ok = true;

  ok = msg.read(stream);

  if (ok)
  {
    setId(msg.id);
  }

  uint32_t offset = 0;
  uint32_t count = 0;

  ok = ok && stream.readElement(offset) == sizeof(offset);
  ok = ok && stream.readElement(count) == sizeof(count);

  if (count)
  {
    if (count + offset > _index_count)
    {
      reallocateIndices(count + offset);
      _index_count = count + offset;
    }

    ok = ok && stream.readArray(_indices + offset, count) == count;
  }

  return ok;
}


unsigned PointCloudShape::enumerateResources(const Resource **resources, unsigned capacity,
                                             unsigned fetch_offset) const
{
  if (!resources || !capacity)
  {
    return 1;
  }

  if (fetch_offset == 0)
  {
    resources[0] = _mesh;
    return 1;
  }

  return 0;
}


Shape *PointCloudShape::clone() const
{
  auto *copy = new PointCloudShape(_mesh);
  onClone(copy);
  return copy;
}


void PointCloudShape::onClone(PointCloudShape *copy) const
{
  Shape::onClone(copy);
  if (_index_count)
  {
    copy->_indices = allocateIndices(_index_count);
    std::memcpy(copy->_indices, _indices, sizeof(*_indices) * _index_count);
    copy->_index_count = _index_count;
  }
  copy->_mesh = _mesh;
  copy->_point_scale = _point_scale;
}


void PointCloudShape::reallocateIndices(uint32_t count)
{
  if (count)
  {
    uint32_t *new_indices = allocateIndices(count);
    if (_indices)
    {
      if (_index_count)
      {
        std::memcpy(new_indices, _indices, sizeof(*_indices) * std::min(count, _index_count));
      }
      freeIndices(_indices);
    }
    _indices = new_indices;
  }
  else
  {
    freeIndices(_indices);
    _indices = nullptr;
  }
  _index_count = count;
}


uint32_t *PointCloudShape::allocateIndices(uint32_t count)
{
  // Hidden for consistent allocator usage.
  return new uint32_t[count];
}


void PointCloudShape::freeIndices(const uint32_t *indices)
{
  // Hidden for consistent allocator usage.
  delete[] indices;
}
}  // namespace tes
