//
// author: Kazys Stepanas
//
#include "BaseConnection.h"

#include <3escore/CollatedPacket.h>
#include <3escore/CoreUtil.h>
#include <3escore/Debug.h>
#include <3escore/Endian.h>
#include <3escore/Log.h>
#include <3escore/Resource.h>
#include <3escore/ResourcePacker.h>
#include <3escore/Rotation.h>
#include <3escore/TcpSocket.h>

#include <3escore/shapes/Shape.h>

#include <algorithm>

namespace tes
{
namespace
{
constexpr float kSecondsToMicroseconds = 1e6;
}  // namespace

BaseConnection::BaseConnection(const ServerSettings &settings)
  : _current_resource(std::make_unique<ResourcePacker>())
  , _server_flags(settings.flags)
  , _collation(std::make_unique<CollatedPacket>((settings.flags & SFCompress) != 0))
{
  _packet_buffer.resize(settings.client_buffer_size);
  _packet = std::make_unique<PacketWriter>(_packet_buffer.data(),
                                           int_cast<uint16_t>(_packet_buffer.size()));
  initDefaultServerInfo(&_server_info);
  _seconds_to_time_unit =
    kSecondsToMicroseconds /
    (_server_info.time_unit ? static_cast<float>(_server_info.time_unit) : 1.0f);
  _collation->setCompressionLevel(settings.compression_level);
}


BaseConnection::~BaseConnection() = default;


void BaseConnection::setActive(bool enable)
{
  _active = enable;
}


bool BaseConnection::active() const
{
  return _active;
}


bool BaseConnection::sendServerInfo(const ServerInfoMessage &info)
{
  if (!_active)
  {
    return false;
  }

  _server_info = info;
  _seconds_to_time_unit =
    kSecondsToMicroseconds /
    (_server_info.time_unit ? static_cast<float>(_server_info.time_unit) : 1.0f);

  if (isConnected())
  {
    const std::lock_guard<Lock> guard(_packet_lock);
    _packet->reset(MtServerInfo, 0);
    if (info.write(*_packet))
    {
      if (_packet->finalise())
      {
        const std::lock_guard<Lock> send_guard(_send_lock);
        // Do not use collation buffer or compression for this message.
        writeBytes(_packet_buffer.data(), _packet->packetSize());
        return true;
      }
    }
  }

  return false;
}


int BaseConnection::send(const PacketWriter &packet, bool allow_collation)
{
  return send(packet.data(), packet.packetSize(), allow_collation);
}


int BaseConnection::send(const CollatedPacket &collated)
{
  if (!_active)
  {
    return 0;
  }

  // Must be finalised and uncompressed for this path..
  if (!collated.isFinalised() || collated.compressionEnabled())
  {
    return -1;
  }

  unsigned collated_bytes = 0;
  const uint8_t *bytes = collated.buffer(collated_bytes);

  if (collated_bytes < CollatedPacket::InitialCursorOffset + sizeof(PacketHeader))
  {
    // Nothing to send.
    return 0;
  }

  // Use the packet lock to prevent other sends until the collated packet is flushed.
  const std::lock_guard<Lock> guard(_packet_lock);
  // Extract each packet in turn.
  // Cycle each message and send.
  const auto *packet = reinterpret_cast<const PacketHeader *>(bytes);
  unsigned processed_bytes = CollatedPacket::InitialCursorOffset;
  unsigned packet_size = 0;
  uint16_t payload_size = 0;
  bool crc_present = false;
  if (!(packet->flags & PFNoCrc))
  {
    processed_bytes -= static_cast<unsigned>(sizeof(PacketWriter::CrcType));
  }
  packet = reinterpret_cast<const PacketHeader *>(bytes + processed_bytes);
  while (processed_bytes + sizeof(PacketHeader) < collated_bytes)
  {
    // Determine current packet size.
    // Get payload size.
    payload_size = packet->payload_size;
    networkEndianSwap(payload_size);
    // Add header size.
    packet_size = payload_size + static_cast<unsigned>(sizeof(PacketHeader));
    // Add Crc Size.
    crc_present = (packet->flags & PFNoCrc) == 0;
    packet_size += !!crc_present * static_cast<unsigned>(sizeof(PacketWriter::CrcType));

    // Send packet.
    if (packet_size + processed_bytes > collated_bytes)
    {
      return -1;
    }
    send(reinterpret_cast<const uint8_t *>(packet), int_cast<int>(packet_size));

    // Next packet.
    processed_bytes += packet_size;
    packet = reinterpret_cast<const PacketHeader *>(bytes + processed_bytes);
  }

  return int_cast<int>(processed_bytes);
}


int BaseConnection::send(const uint8_t *data, int byte_count, bool allow_collation)
{
  if (!_active)
  {
    return 0;
  }

  return writePacket(data, int_cast<uint16_t>(byte_count), allow_collation);
}


int BaseConnection::create(const Shape &shape)
{
  if (!_active)
  {
    return 0;
  }

  // const std::lock_guard<Lock> guard(_lock);
  const std::lock_guard<Lock> guard(_packet_lock);
  if (shape.writeCreate(*_packet))
  {
    // Send the create message.
    _packet->finalise();
    writePacket(_packet_buffer.data(), _packet->packetSize(), true);
    int64_t total_bytes_written = _packet->packetSize();

    // For complex shapes, we must also send data messages.
    if (shape.isComplex())
    {
      const int wrote = sendShapeData(shape);
      if (wrote < 0)
      {
        return -1;
      }
      total_bytes_written += wrote;
    }

    if (!shape.skipResources())
    {
      queueResources(shape);
    }

    if (total_bytes_written > std::numeric_limits<int>::max())
    {
      log::warn("Large byte data transfer for shape ", shape.routingId(), ":", shape.id(), " - ",
                total_bytes_written);
      total_bytes_written = std::numeric_limits<int>::max();
    }

    return static_cast<int>(total_bytes_written);
  }
  return -1;
}


int BaseConnection::destroy(const Shape &shape)
{
  if (!_active)
  {
    return 0;
  }

  const std::lock_guard<Lock> guard(_packet_lock);

  // Remove resources for persistent objects. Transient won't have destroy called and
  // won't correctly release the resources. Check the ID because I'm paranoid.
  if (shape.id() && !shape.skipResources())
  {
    _resource_buffer.clear();
    shape.enumerateResources(_resource_buffer);
    for (const auto &resource : _resource_buffer)
    {
      releaseResource(resource->uniqueKey());
    }
    // clear buffer to avoid holding references.
    _resource_buffer.clear();
  }

  if (shape.writeDestroy(*_packet))
  {
    _packet->finalise();
    writePacket(_packet_buffer.data(), _packet->packetSize(), true);
    return _packet->packetSize();
  }
  return -1;
}


int BaseConnection::update(const Shape &shape)
{
  if (!_active)
  {
    return 0;
  }

  const std::lock_guard<Lock> guard(_packet_lock);
  if (shape.writeUpdate(*_packet))
  {
    _packet->finalise();

    writePacket(_packet_buffer.data(), _packet->packetSize(), true);
    return _packet->packetSize();
  }
  return -1;
}


int BaseConnection::updateTransfers(unsigned byte_limit)
{
  if (!_active)
  {
    return 0;
  }

  const std::lock_guard<Lock> guard(_packet_lock);
  const std::lock_guard<Lock> resource_guard(_resource_lock);
  unsigned transferred = 0;

  while ((!byte_limit || transferred < byte_limit) &&
         (_current_resource->isValid() || !_resource_queue.empty()))
  {
    if (!_current_resource->isValid())
    {
      if (!_resource_queue.empty())  // Should never be empty. Loop check this.
      {
        // Start the next resource transfer.
        const uint64_t next_resource = _resource_queue.front();
        _resource_queue.pop_front();
        auto resource_info = _resources.find(next_resource);
        if (resource_info != _resources.end())
        {
          auto &resource = resource_info->second.resource;
          resource_info->second.started = true;
          _current_resource->transfer(resource);
        }
      }
      continue;
    }

    if (_current_resource->nextPacket(*_packet, byte_limit ? byte_limit - transferred : 0))
    {
      _packet->finalise();
      writePacket(_packet->data(), _packet->packetSize(), true);
      transferred += _packet->packetSize();
    }

    // Check completion.
    if (!_current_resource->isValid())
    {
      // Have completed
      const uint64_t completed_id = _current_resource->lastCompletedId();
      auto resource_info = _resources.find(completed_id);
      if (resource_info != _resources.end())
      {
        resource_info->second.sent = true;
      }
    }
  }

  return 0;
}


int BaseConnection::updateFrame(float dt, bool flush)
{
  if (!_active)
  {
    return 0;
  }

  // std::lock_guard<Lock> guard(_lock);
  int wrote = -1;
  ControlMessage msg;
  msg.control_flags = !flush * CFFramePersist;
  // Convert dt to desired time unit.
  msg.value32 = static_cast<uint32_t>(dt * _seconds_to_time_unit);
  msg.value64 = 0;
  const std::lock_guard<Lock> guard(_packet_lock);
  // Send frame number too?
  _packet->reset(MtControl, CIdFrame);
  if (msg.write(*_packet))
  {
    _packet->finalise();
    wrote = writePacket(_packet_buffer.data(), _packet->packetSize(),
                        !(_server_flags & SFNakedFrameMessage));
  }
  flushCollatedPacket();
  return wrote;
}


unsigned BaseConnection::referenceResource(const ResourcePtr &resource)
{
  if (!_active)
  {
    return 0;
  }

  unsigned ref_count = 0;
  const uint64_t resource_id = resource->uniqueKey();
  const std::lock_guard<Lock> resource_guard(_resource_lock);
  auto existing = _resources.find(resource_id);
  if (existing != _resources.end())
  {
    ref_count = ++existing->second.reference_count;
  }
  else
  {
    _resources.emplace(std::make_pair(resource_id, ResourceInfo(resource)));
    _resource_queue.push_back(resource_id);
    ref_count = 1;
  }

  return ref_count;
}


unsigned BaseConnection::releaseResource(const ResourcePtr &resource)
{
  if (!_active)
  {
    return 0;
  }

  const std::lock_guard<Lock> packet_guard(_packet_lock);
  return releaseResource(resource->uniqueKey());
}


int BaseConnection::sendShapeData(const Shape &shape)
{
  TES_ASSERT(shape.isComplex());
  unsigned progress = 0;
  int status = 0;
  int total_bytes_written = 0;
  while ((status = shape.writeData(*_packet, progress)) >= 0)
  {
    if (!_packet->finalise())
    {
      return -1;
    }

    const int wrote = writePacket(_packet_buffer.data(), _packet->packetSize(), true);

    if (wrote < 0)
    {
      return -1;
    }

    total_bytes_written += wrote;

    if (status == 0)
    {
      break;
    }
  }

  if (status == -1)
  {
    return -1;
  }

  return total_bytes_written;
}


unsigned BaseConnection::queueResources(const Shape &shape)
{
  if (shape.isTransient())
  {
    checkResources(shape);
    return 0;
  }

  _resource_buffer.clear();
  const unsigned resource_count = shape.enumerateResources(_resource_buffer);
  for (const auto &resource : _resource_buffer)
  {
    referenceResource(resource);
  }
  // clear buffer to avoid holding references.
  _resource_buffer.clear();

  return resource_count;
}


bool BaseConnection::checkResources(const Shape &shape)
{
  bool all_present = true;
  _resource_buffer.clear();
  shape.enumerateResources(_resource_buffer);
  for (const auto &resource : _resource_buffer)
  {
    const auto search = _resources.find(resource->uniqueKey());
    if (search == _resources.end())
    {
      all_present = false;
      log::warn("Shape ", shape.routingId(), ":", shape.id(), " missing resource ",
                resource->typeId(), ":", resource->id());
    }
  }
  // clear buffer to avoid holding references.
  _resource_buffer.clear();

  return all_present;
}


unsigned BaseConnection::releaseResource(uint64_t resource_id)
{
  unsigned reference_count = 0;
  const std::lock_guard<Lock> resource_guard(_resource_lock);
  auto existing = _resources.find(resource_id);
  if (existing != _resources.end())
  {
    if (existing->second.reference_count > 1)
    {
      reference_count = --existing->second.reference_count;
    }
    else
    {
      if (_current_resource->resource() &&
          _current_resource->resource()->uniqueKey() == resource_id)
      {
        _current_resource->cancel();
      }

      if (existing->second.started || existing->second.sent)
      {
        // Send destroy message.
        _packet->reset();
        existing->second.resource->destroy(*_packet);
        _packet->finalise();
        writePacket(_packet_buffer.data(), _packet->packetSize(), true);
      }

      _resources.erase(existing);
    }
  }

  return reference_count;
}


void BaseConnection::flushCollatedPacket()
{
  const std::lock_guard<Lock> guard(_send_lock);
  flushCollatedPacketUnguarded();
}


void BaseConnection::flushCollatedPacketUnguarded()
{
  if (_collation->collatedBytes())
  {
    _collation->finalise();
    unsigned byte_count = 0;
    const uint8_t *bytes = _collation->buffer(byte_count);
    if (bytes && byte_count)
    {
      writeBytes(bytes, int_cast<int>((byte_count)));
    }
    _collation->reset();
  }
}


int BaseConnection::writePacket(const uint8_t *buffer, uint16_t byte_count, bool allow_collation)
{
  const std::unique_lock<Lock> guard(_send_lock);

  if ((SFCollate & _server_flags) != 0 && !allow_collation)
  {
    flushCollatedPacketUnguarded();
  }

  if ((SFCollate & _server_flags) == 0 || !allow_collation)
  {
    return writeBytes(buffer, byte_count);
  }

  // Add to the collection buffer.
  if (byte_count >= _collation->availableBytes())
  {
    flushCollatedPacketUnguarded();
  }

  int send_count = _collation->add(buffer, byte_count);
  if (send_count == -1)
  {
    // Failed to collate. Packet may be too big to collated (due to collation overhead).
    // Flush the buffer, then send without collation.
    flushCollatedPacketUnguarded();
    send_count = writeBytes(buffer, byte_count);
  }

  return send_count;
}


void BaseConnection::ensurePacketBufferCapacity(size_t size)
{
  if (_packet_buffer.capacity() < size)
  {
    // Need to reallocated. Adjust packet pointer.
    _packet_buffer.resize(size);
    _packet = std::make_unique<PacketWriter>(_packet_buffer.data(),
                                             int_cast<uint16_t>(_packet_buffer.size()));
  }
  else if (_packet_buffer.size() < size)
  {
    // Resize without reallocation. No buffer adjustments required.
    _packet_buffer.resize(size);
  }
}
}  // namespace tes
