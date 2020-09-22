//
// author: Kazys Stepanas
//
#include "3esmeshresource.h"

#include "3esmeshmessages.h"
#include "3esrotation.h"
#include "3estransferprogress.h"
#include "3estransform.h"

#include <algorithm>
#include <vector>

using namespace tes;

uint16_t MeshResource::estimateTransferCount(size_t elementSize, unsigned byteLimit, unsigned overhead)
{
  //                                    packet header           message                         crc
  const size_t maxTransfer = (0xffffu - (sizeof(PacketHeader) + overhead + sizeof(uint16_t))) / elementSize;
  size_t count = byteLimit ? byteLimit / elementSize : maxTransfer;
  if (count < 1)
  {
    count = 1;
  }
  else if (count > maxTransfer)
  {
    count = maxTransfer;
  }

  return uint16_t(count);
}


uint16_t MeshResource::typeId() const
{
  return MtMesh;
}


int MeshResource::create(PacketWriter &packet) const
{
  MeshCreateMessage msg;
  ObjectAttributesd attributes;
  Transform transform = this->transform();

  msg.meshId = id();
  msg.vertexCount = vertexCount();
  msg.indexCount = indexCount();
  msg.flags = 0;
  msg.drawType = drawType();

  if (transform.preferDoublePrecision())
  {
    msg.flags |= McfDoublePrecision;
  }

  packet.reset(typeId(), MeshCreateMessage::MessageId);

  const Vector3d pos(transform.position());
  const Quaterniond rot(transform.rotation());
  const Vector3d scale(transform.scale());

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
  destroy.meshId = id();
  destroy.write(packet);
  return 0;
}


int MeshResource::transfer(PacketWriter &packet, unsigned byteLimit, TransferProgress &progress) const
{
  // packet.reset(typeId(), 0);
  if (progress.phase == 0)
  {
    // Initialise phase.
    progress.phase = MmtVertex;
    progress.progress = 0;
  }

  packet.reset(typeId(), progress.phase);
  MeshComponentMessage msg;
  msg.meshId = id();
  msg.write(packet);

  VertexBuffer dataSource;
  unsigned writeCount = 0;
  switch (progress.phase)
  {
  case MmtVertex:
    dataSource = vertices(0);
    switch (dataSource.type())
    {
    case DctFloat32:
    case DctFloat64:
    case DctPackedFloat16:
    case DctPackedFloat32:
      break;
    default:
      TES_THROW(Exception("Bad vertex position type", __FILE__, __LINE__), -1);
    }
    if (dataSource.componentCount() != 3)
    {
      TES_THROW(Exception("Bad vertex position component count", __FILE__, __LINE__), -1);
    }
    break;

  case MmtVertexColour:
    packet.reset(typeId(), MmtVertexColour);
    dataSource = colours(0);
    switch (dataSource.type())
    {
    case DctUInt32:
      break;
    default:
      TES_THROW(Exception("Bad vertex colour type", __FILE__, __LINE__), -1);
    }
    if (dataSource.componentCount() != 1)
    {
      TES_THROW(Exception("Bad vertex colour component count", __FILE__, __LINE__), -1);
    }
    break;

  case MmtIndex:
    packet.reset(typeId(), MmtIndex);
    dataSource = indices(0);
    switch (dataSource.type())
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
    if (dataSource.componentCount() != 1)
    {
      TES_THROW(Exception("Bad index component count", __FILE__, __LINE__), -1);
    }
    break;

  case MmtNormal:
    packet.reset(typeId(), MmtNormal);
    dataSource = normals(0);
    switch (dataSource.type())
    {
    case DctFloat32:
    case DctFloat64:
    case DctPackedFloat16:
    case DctPackedFloat32:
      break;
    default:
      TES_THROW(Exception("Bad vertex normal type", __FILE__, __LINE__), -1);
    }
    if (dataSource.componentCount() != 3)
    {
      TES_THROW(Exception("Bad vertex normal component count", __FILE__, __LINE__), -1);
    }
    break;

  case MmtUv:
    packet.reset(typeId(), MmtUv);
    dataSource = uvs(0);
    switch (dataSource.type())
    {
    case DctFloat32:
    case DctFloat64:
    case DctPackedFloat16:
    case DctPackedFloat32:
      break;
    default:
      TES_THROW(Exception("Bad vertex UV type", __FILE__, __LINE__), -1);
    }
    if (dataSource.componentCount() != 2)
    {
      TES_THROW(Exception("Bad vertex UV component count", __FILE__, __LINE__), -1);
    }
    break;

  case MmtFinalise: {
    MeshFinaliseMessage msg;
    packet.reset(typeId(), MeshFinaliseMessage::MessageId);
    msg.meshId = id();
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

  if (dataSource.isValid())
  {
    writeCount = dataSource.write(packet, uint32_t(progress.progress), byteLimit);

    if (writeCount == 0 && dataSource.count() > 0)
    {
      // Failed to write when we should have.
      return -1;
    }

    progress.progress += writeCount;
  }

  if (!progress.complete && progress.progress >= dataSource.count())
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


bool MeshResource::readTransfer(int messageType, PacketReader &packet)
{
  MeshComponentMessage msg;
  bool ok = true;

  if (!msg.read(packet))
  {
    return false;
  }

  VertexBuffer readStream;

  // Read offset and count
  uint32_t offset = 0;
  uint16_t count = 0;

  // Read vertex stream offset and count.
  ok = packet.readElement(offset) == sizeof(offset) && ok;
  ok = packet.readElement(count) == sizeof(count) && ok;

  switch (messageType)
  {
  case MmtVertex: {
    readStream = VertexBuffer(static_cast<Vector3d *>(nullptr), 0);
    // Read the expected number of items.
    readStream.read(packet, 0, count);
    ok = processVertices(msg, offset, readStream) && ok;
    break;
  }
  case MmtIndex: {
    readStream = VertexBuffer(static_cast<uint32_t *>(nullptr), 0);
    // Read the expected number of items.
    readStream.read(packet, 0, count);
    ok = processIndices(msg, offset, readStream) && ok;
    break;
  }
  case MmtVertexColour: {
    readStream = VertexBuffer(static_cast<uint32_t *>(nullptr), 0);
    // Read the expected number of items.
    readStream.read(packet, 0, count);
    ok = processColours(msg, offset, readStream) && ok;
    break;
  }
  case MmtNormal: {
    readStream = VertexBuffer(static_cast<Vector3d *>(nullptr), 0);
    // Read the expected number of items.
    readStream.read(packet, 0, count);
    ok = processNormals(msg, offset, readStream) && ok;
    break;
  }
  case MmtUv: {
    readStream = VertexBuffer(static_cast<double *>(nullptr), 0, 2);
    // Read the expected number of items.
    readStream.read(packet, 0, count);
    ok = processUVs(msg, offset, readStream) && ok;
    break;
  }
  }

  if (msg.meshId != id())
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


bool MeshResource::processVertices(const MeshComponentMessage &msg, unsigned offset, const VertexBuffer &stream)
{
  TES_UNUSED(msg);
  TES_UNUSED(offset);
  TES_UNUSED(stream);
  return false;
}


bool MeshResource::processIndices(const MeshComponentMessage &msg, unsigned offset, const VertexBuffer &stream)
{
  TES_UNUSED(msg);
  TES_UNUSED(offset);
  TES_UNUSED(stream);
  return false;
}


bool MeshResource::processColours(const MeshComponentMessage &msg, unsigned offset, const VertexBuffer &stream)
{
  TES_UNUSED(msg);
  TES_UNUSED(offset);
  TES_UNUSED(stream);
  return false;
}


bool MeshResource::processNormals(const MeshComponentMessage &msg, unsigned offset, const VertexBuffer &stream)
{
  TES_UNUSED(msg);
  TES_UNUSED(offset);
  TES_UNUSED(stream);
  return false;
}


bool MeshResource::processUVs(const MeshComponentMessage &msg, unsigned offset, const VertexBuffer &stream)
{
  TES_UNUSED(msg);
  TES_UNUSED(offset);
  TES_UNUSED(stream);
  return false;
}
