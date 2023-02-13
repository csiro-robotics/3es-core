//
// author: Kazys Stepanas
//
#include "PointCloudShape.h"

#include "MeshPlaceholder.h"

#include <3escore/CoreUtil.h>
#include <3escore/PacketWriter.h>

#include <iterator>

namespace tes
{
PointCloudShape::~PointCloudShape() = default;


bool PointCloudShape::writeCreate(PacketWriter &stream) const
{
  bool ok = Shape::writeCreate(stream);
  // Write the point cloud ID.
  uint32_t value_u32 = _mesh->id();
  ok = stream.writeElement(value_u32) == sizeof(value_u32) && ok;
  // Write the index count.
  value_u32 = int_cast<uint32_t>(_indices.size());
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
  const uint32_t index_count = int_cast<uint32_t>(_indices.size());
  uint32_t count = index_count - progress_marker;
  if (count > max_items)
  {
    count = max_items;
  }

  // Use 32-bits for both values though count will never need greater than 16-bit.
  ok = stream.writeElement(offset) == sizeof(offset) && ok;
  ok = stream.writeElement(count) == sizeof(count) && ok;

  if (count)
  {
    ok = stream.writeArray(_indices.data() + offset, count) == count && ok;
  }

  if (!ok)
  {
    return -1;
  }

  progress_marker += count;
  return (progress_marker < index_count) ? 1 : 0;
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
  _mesh = std::make_shared<MeshPlaceholder>(value_u32);

  // Index count.
  ok = ok && stream.readElement(value_u32) == sizeof(value_u32);
  _indices.resize(value_u32);

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
    if (count + offset > _indices.size())
    {
      _indices.resize(count + offset);
    }

    ok = ok && stream.readArray(_indices.data() + offset, count) == count;
  }

  return ok;
}


unsigned PointCloudShape::enumerateResources(std::vector<ResourcePtr> &resources) const
{
  resources.emplace_back(_mesh);
  return 1;
}


std::shared_ptr<Shape> PointCloudShape::clone() const
{
  auto copy = std::make_shared<PointCloudShape>(_mesh);
  onClone(*copy);
  return copy;
}


void PointCloudShape::onClone(PointCloudShape &copy) const
{
  Shape::onClone(copy);
  copy._indices.clear();
  std::copy(_indices.begin(), _indices.end(), std::back_inserter(copy._indices));
  copy._mesh = _mesh;
  copy._point_scale = _point_scale;
}
}  // namespace tes
