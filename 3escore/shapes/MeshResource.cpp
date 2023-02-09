//
// author: Kazys Stepanas
//
#include "MeshResource.h"

#include <3escore/CoreUtil.h>
#include <3escore/MeshMessages.h>
#include <3escore/Rotation.h>
#include <3escore/TransferProgress.h>
#include <3escore/Transform.h>

#include <algorithm>
#include <vector>

namespace tes
{
uint16_t MeshResource::estimateTransferCount(size_t element_size, unsigned byte_limit,
                                             unsigned overhead)
{
  //                                    packet header           message                         crc
  const size_t max_transfer =
    (0xffffu - (sizeof(PacketHeader) + overhead + sizeof(uint16_t))) / element_size;
  size_t count = byte_limit ? byte_limit / element_size : max_transfer;
  if (count < 1)
  {
    count = 1;
  }
  else if (count > max_transfer)
  {
    count = max_transfer;
  }

  return int_cast<uint16_t>(count);
}

uint16_t MeshResource::typeId() const
{
  return MtMesh;
}


int MeshResource::create(PacketWriter &packet) const
{
  MeshCreateMessage msg;
  ObjectAttributesd attributes;
  const Transform transform = this->transform();

  msg.mesh_id = id();
  msg.vertex_count = vertexCount();
  msg.index_count = indexCount();
  msg.flags = 0;
  msg.draw_type = drawType();

  if (transform.preferDoublePrecision())
  {
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    msg.flags |= McfDoublePrecision;
  }

  packet.reset(typeId(), MeshCreateMessage::MessageId);

  const Vector3d &pos = transform.position();
  const Quaterniond &rot = transform.rotation();
  const Vector3d &scale = transform.scale();

  attributes.colour = tint();
  attributes.position[0] = pos[0];
  attributes.position[1] = pos[1];
  attributes.position[2] = pos[2];
  attributes.rotation[0] = rot[0];
  attributes.rotation[1] = rot[1];
  attributes.rotation[2] = rot[2];
  attributes.rotation[3] = rot[3];
  attributes.scale[0] = scale[0];
  attributes.scale[1] = scale[1];
  attributes.scale[2] = scale[2];

  if (msg.write(packet, attributes))
  {
    return 0;
  }

  return -1;
}


int MeshResource::destroy(PacketWriter &packet) const
{
  MeshDestroyMessage destroy;
  packet.reset(typeId(), MeshDestroyMessage::MessageId);
  destroy.mesh_id = id();
  destroy.write(packet);
  return 0;
}


// NOLINTNEXTLINE(readability-function-cognitive-complexity)
int MeshResource::transfer(PacketWriter &packet, unsigned byte_limit,
                           TransferProgress &progress) const
{
  // packet.reset(typeId(), 0);
  if (progress.phase == 0)
  {
    // Initialise phase.
    progress.phase = MmtVertex;
    progress.progress = 0;
    if (vertexCount() == 0)
    {
      // No vertices. Skip to next phase.
      nextPhase(progress);
    }
  }

  packet.reset(typeId(), int_cast<uint16_t>(progress.phase));
  MeshComponentMessage msg;
  msg.mesh_id = id();
  msg.write(packet);

  DataBuffer data_source;
  unsigned write_count = 0;
  switch (progress.phase)
  {
  case MmtVertex:
    data_source = vertices(0);
    switch (data_source.type())
    {
    case DctFloat32:
    case DctFloat64:
    case DctPackedFloat16:
    case DctPackedFloat32:
      break;
    default:
      TES_THROW(Exception("Bad vertex position type", __FILE__, __LINE__), -1);
    }
    if (data_source.componentCount() != 3)
    {
      TES_THROW(Exception("Bad vertex position component count", __FILE__, __LINE__), -1);
    }
    break;

  case MmtVertexColour: {
    unsigned expected_component_count = 1;
    data_source = colours(0);
    switch (data_source.type())
    {
    case DctUInt32:
      expected_component_count = 1;
      break;
    case DctUInt8:
      expected_component_count = 4;
      break;
    default:
      TES_THROW(Exception("Bad vertex colour type", __FILE__, __LINE__), -1);
    }
    if (data_source.componentCount() != expected_component_count)
    {
      TES_THROW(Exception("Bad vertex colour component count", __FILE__, __LINE__), -1);
    }
    break;
  }
  case MmtIndex:
    data_source = indices(0);
    switch (data_source.type())
    {
    case DctInt8:
    case DctUInt8:
    case DctInt16:
    case DctUInt16:
    case DctInt32:
    case DctUInt32:
    case DctInt64:
    case DctUInt64:
      break;
    default:
      TES_THROW(Exception("Bad index type", __FILE__, __LINE__), -1);
    }
    if (data_source.componentCount() != 1)
    {
      TES_THROW(Exception("Bad index component count", __FILE__, __LINE__), -1);
    }
    break;

  case MmtNormal:
    data_source = normals(0);
    switch (data_source.type())
    {
    case DctFloat32:
    case DctFloat64:
    case DctPackedFloat16:
    case DctPackedFloat32:
      break;
    default:
      TES_THROW(Exception("Bad vertex normal type", __FILE__, __LINE__), -1);
    }
    if (data_source.componentCount() != 3)
    {
      TES_THROW(Exception("Bad vertex normal component count", __FILE__, __LINE__), -1);
    }
    break;

  case MmtUv:
    data_source = uvs(0);
    switch (data_source.type())
    {
    case DctFloat32:
    case DctFloat64:
    case DctPackedFloat16:
    case DctPackedFloat32:
      break;
    default:
      TES_THROW(Exception("Bad vertex UV type", __FILE__, __LINE__), -1);
    }
    if (data_source.componentCount() != 2)
    {
      TES_THROW(Exception("Bad vertex UV component count", __FILE__, __LINE__), -1);
    }
    break;

  case MmtFinalise: {
    MeshFinaliseMessage msg;
    packet.reset(typeId(), MeshFinaliseMessage::MessageId);
    msg.mesh_id = id();
    msg.flags = (!normals(0).isValid()) ? MffCalculateNormals : 0;
    msg.write(packet);
    // Mark complete.
    progress.complete = true;
  }
  break;

  default:
    // Unknown state really.
    progress.failed = true;
    break;
  }

  if (data_source.isValid())
  {
    write_count = data_source.write(packet, int_cast<uint32_t>(progress.progress), byte_limit);

    if (write_count == 0 && data_source.count() > 0)
    {
      // Failed to write when we should have.
      return -1;
    }

    progress.progress += write_count;
  }

  if (!progress.complete && progress.progress >= data_source.count())
  {
    // Phase complete. Progress to the next phase.
    nextPhase(progress);
  }

  return 0;
}


bool MeshResource::readCreate(PacketReader &packet)
{
  MeshCreateMessage msg;
  ObjectAttributesd attributes;
  bool ok = true;
  ok = ok && msg.read(packet, attributes);
  return ok && processCreate(msg, attributes);
}


bool MeshResource::readTransfer(int message_type, PacketReader &packet)
{
  MeshComponentMessage msg;
  bool ok = true;

  if (!msg.read(packet))
  {
    return false;
  }

  // Read offset and count
  uint32_t offset = 0;
  uint16_t count = 0;

  // Read vertex stream offset and count.
  ok = packet.readElement(offset) == sizeof(offset) && ok;
  ok = packet.readElement(count) == sizeof(count) && ok;

  // If we need to peek the stream data type and component count we can do by invoking
  // peek_stream_info(). On success, component_count and packet_type will be valid.
  std::array<uint8_t, 2> component_count_and_packet_type;
  const uint8_t &component_count = component_count_and_packet_type[0];
  const uint8_t &packet_type = component_count_and_packet_type[1];
  const auto peek_stream_info = [&packet, &component_count_and_packet_type] {
    return packet.peek(component_count_and_packet_type.data(),
                       component_count_and_packet_type.size(),
                       false) == sizeof(component_count_and_packet_type);
  };

  DataBuffer read_stream;
  switch (message_type)
  {
  case MmtVertex: {
    read_stream = DataBuffer(static_cast<Vector3d *>(nullptr), 0);
    // Read the expected number of items.
    read_stream.read(packet, 0, count);
    ok = processVertices(msg, offset, read_stream) && ok;
    break;
  }
  case MmtIndex: {
    read_stream = DataBuffer(DctUInt32);
    // Read the expected number of items.
    read_stream.read(packet, 0, count);
    ok = processIndices(msg, offset, read_stream) && ok;
    break;
  }
  case MmtVertexColour: {
    // Peek stream info. We may have uint32_t or uint8_t streams.
    ok = peek_stream_info() && ok;
    read_stream = DataBuffer(static_cast<DataStreamType>(packet_type), component_count);
    // Read the expected number of items.
    read_stream.read(packet, 0, count);
    ok = processColours(msg, offset, read_stream) && ok;
    break;
  }
  case MmtNormal: {
    read_stream = DataBuffer(static_cast<Vector3d *>(nullptr), 0);
    // Read the expected number of items.
    read_stream.read(packet, 0, count);
    ok = processNormals(msg, offset, read_stream) && ok;
    break;
  }
  case MmtUv: {
    read_stream = DataBuffer(DctFloat64, 2);
    // Read the expected number of items.
    read_stream.read(packet, 0, count);
    ok = processUVs(msg, offset, read_stream) && ok;
    break;
  }
  }

  if (msg.mesh_id != id())
  {
    ok = false;
  }

  return ok;
}


void MeshResource::nextPhase(TransferProgress &progress) const
{
  int next = MmtFinalise;
  switch (progress.phase)
  {
    // First call.
  case 0:
    if (vertexCount() && vertices().isValid())
    {
      next = MmtVertex;
      break;
    }
    // Don't break.
    TES_FALLTHROUGH;
  case MmtVertex:
    if (indexCount() && indices().isValid())
    {
      next = MmtIndex;
      break;
    }
    // Don't break.
    TES_FALLTHROUGH;
  case MmtIndex:
    if (vertexCount() && colours().isValid())
    {
      next = MmtVertexColour;
      break;
    }
    // Don't break.
    TES_FALLTHROUGH;
  case MmtVertexColour:
    if (vertexCount() && normals().isValid())
    {
      next = MmtNormal;
      break;
    }
    // Don't break.
    TES_FALLTHROUGH;
  case MmtNormal:
    if (vertexCount() && uvs().isValid())
    {
      next = MmtUv;
      break;
    }
    // Don't break.
    TES_FALLTHROUGH;
  default:
    break;
  }

  progress.progress = 0;
  progress.phase = next;
}


bool MeshResource::processCreate(const MeshCreateMessage &msg, const ObjectAttributesd &attributes)
{
  TES_UNUSED(msg);
  TES_UNUSED(attributes);
  return false;
}


bool MeshResource::processVertices(const MeshComponentMessage &msg, unsigned offset,
                                   const DataBuffer &stream)
{
  TES_UNUSED(msg);
  TES_UNUSED(offset);
  TES_UNUSED(stream);
  return false;
}


bool MeshResource::processIndices(const MeshComponentMessage &msg, unsigned offset,
                                  const DataBuffer &stream)
{
  TES_UNUSED(msg);
  TES_UNUSED(offset);
  TES_UNUSED(stream);
  return false;
}


bool MeshResource::processColours(const MeshComponentMessage &msg, unsigned offset,
                                  const DataBuffer &stream)
{
  TES_UNUSED(msg);
  TES_UNUSED(offset);
  TES_UNUSED(stream);
  return false;
}


bool MeshResource::processNormals(const MeshComponentMessage &msg, unsigned offset,
                                  const DataBuffer &stream)
{
  TES_UNUSED(msg);
  TES_UNUSED(offset);
  TES_UNUSED(stream);
  return false;
}


bool MeshResource::processUVs(const MeshComponentMessage &msg, unsigned offset,
                              const DataBuffer &stream)
{
  TES_UNUSED(msg);
  TES_UNUSED(offset);
  TES_UNUSED(stream);
  return false;
}
}  // namespace tes
