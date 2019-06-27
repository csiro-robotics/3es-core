//
// author: Kazys Stepanas
//
#include "3esbaseconnection.h"

#include <3esresource.h>
#include <3esresourcepacker.h>

#include <shapes/3esshape.h>

#include <3escollatedpacket.h>
#include <3esendian.h>
#include <3esrotation.h>
#include <3estcpsocket.h>

#include <algorithm>
#include <mutex>

using namespace tes;

namespace
{
  const float kSecondsToMicroseconds = 1e6;
}

BaseConnection::BaseConnection(const ServerSettings &settings)
: _packet(nullptr)
, _currentResource(new ResourcePacker)
, _secondsToTimeUnit(0)
, _serverFlags(settings.flags)
, _collation(new CollatedPacket((settings.flags & SF_Compress) != 0))
, _active(true)
{
  _packetBuffer.resize(settings.clientBufferSize);
  _packet = new PacketWriter(_packetBuffer.data(), (uint16_t)_packetBuffer.size());
  initDefaultServerInfo(&_serverInfo);
  _secondsToTimeUnit = kSecondsToMicroseconds / (_serverInfo.timeUnit ? float(_serverInfo.timeUnit) : 1.0f);
  _collation->setCompressionLevel(settings.compressionLevel);
}


BaseConnection::~BaseConnection()
{
  delete _currentResource;
  delete _packet;
}


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

  _serverInfo = info;
  _secondsToTimeUnit = kSecondsToMicroseconds / (_serverInfo.timeUnit ? float(_serverInfo.timeUnit) : 1.0f);

  if (isConnected())
  {
    std::lock_guard<Lock> guard(_packetLock);
    _packet->reset(MtServerInfo, 0);
    if (info.write(*_packet))
    {
      _packet->finalise();
      std::lock_guard<Lock> sendGuard(_sendLock);
      // Do not use collation buffer or compression for this message.
      writeBytes(_packetBuffer.data(), _packet->packetSize());
      return true;
    }
  }

  return false;
}


int BaseConnection::send(const PacketWriter &packet, bool allowCollation)
{
  return send(packet.data(), packet.packetSize(), allowCollation);
}


int BaseConnection::send(const CollatedPacket &collated)
{
  if (!_active)
  {
    return 0;
  }

  // Can't send compressed packets in this way.
  if (collated.compressionEnabled())
  {
    return -1;
  }

  unsigned collatedBytes = 0;
  const uint8_t *bytes = collated.buffer(collatedBytes);

  if (collatedBytes < CollatedPacket::InitialCursorOffset + sizeof(PacketHeader))
  {
    // Nothing to send.
    return 0;
  }

  // Use the packet lock to prevent other sends until the collated packet is flushed.
  std::lock_guard<Lock> guard(_packetLock);
  // Extract each packet in turn.
  // Cycle each message and send.
  const PacketHeader *packet = (const PacketHeader *)(bytes);
  unsigned processedBytes = CollatedPacket::InitialCursorOffset;
  unsigned packetSize = 0;
  uint16_t payloadSize = 0;
  bool crcPreset = false;
  if (!(packet->flags & PF_NoCrc))
  {
    processedBytes -= unsigned(sizeof(PacketWriter::CrcType));
  }
  packet = (const PacketHeader *)(bytes + processedBytes);
  while (processedBytes + sizeof(PacketHeader) < collatedBytes)
  {
    // Determine current packet size.
    // Get payload size.
    payloadSize = packet->payloadSize;
    networkEndianSwap(payloadSize);
    // Add header size.
    packetSize = payloadSize + unsigned(sizeof(PacketHeader));
    // Add Crc Size.
    crcPreset = (packet->flags & PF_NoCrc) == 0;
    packetSize += !!crcPreset * unsigned(sizeof(PacketWriter::CrcType));

    // Send packet.
    if (packetSize + processedBytes > collatedBytes)
    {
      return -1;
    }
    send((const uint8_t *)packet, int(packetSize));

    // Next packet.
    processedBytes += packetSize;
    packet = (const PacketHeader *)(bytes + processedBytes);
  }

  return int(processedBytes);
}


int BaseConnection::send(const uint8_t *data, int byteCount, bool allowCollation)
{
  if (!_active)
  {
    return 0;
  }

  return writePacket(data, uint16_t(byteCount), allowCollation);
}


int BaseConnection::create(const Shape &shape)
{
  if (!_active)
  {
    return 0;
  }

  //std::lock_guard<Lock> guard(_lock);
  std::lock_guard<Lock> guard(_packetLock);
  if (shape.writeCreate(*_packet))
  {
    _packet->finalise();
    writePacket(_packetBuffer.data(), _packet->packetSize(), true);
    int writeSize = _packet->packetSize();
    int wrote;

    // Write complex shape data.
    if (shape.isComplex())
    {
      unsigned progress = 0;
      int res = 0;
      while ((res = shape.writeData(*_packet, progress)) >= 0)
      {
        if (!_packet->finalise())
        {
          return -1;
        }

        wrote = writePacket(_packetBuffer.data(), _packet->packetSize(), true);

        if (wrote < 0)
        {
          return -1;
        }

        writeSize += wrote;

        if (res == 0)
        {
          break;
        }
      }

      if (res == -1)
      {
        return -1;
      }
    }

    // Collate and queue resources for persistent objects. Transient not allowed because
    // destroy won't be called and references won't be released.
    if (shape.id() != 0)
    {
      const unsigned resCapacity = 8;
      const Resource *resources[resCapacity];
      unsigned totalResources = 0;
      unsigned resCount = 0;
      do
      {
        resCount = shape.enumerateResources(resources, resCapacity, totalResources);
        for (unsigned i = 0; i < resCount; ++i)
        {
          referenceResource(resources[i]);
        }
        totalResources += resCount;
      } while (resCount);
    }

    if (writeSize < std::numeric_limits<int>::max())
    {
      return writeSize;
    }

    return std::numeric_limits<int>::max();
  }
  return -1;
}


int BaseConnection::destroy(const Shape &shape)
{
  if (!_active)
  {
    return 0;
  }

  std::lock_guard<Lock> guard(_packetLock);

  // Remove resources for persistent objects. Transient won't have destroy called and
  // won't correctly release the resources. Check the ID because I'm paranoid.
  if (shape.id())
  {
    const unsigned resCapacity = 8;
    const Resource *resources[resCapacity];
    unsigned totalResources = 0;
    unsigned resCount = 0;
    do
    {
      resCount = shape.enumerateResources(resources, resCapacity, totalResources);
      for (unsigned i = 0; i < resCount; ++i)
      {
        releaseResource(resources[i]->uniqueKey());
      }
      totalResources += resCount;
    } while (resCount);
  }

  if (shape.writeDestroy(*_packet))
  {
    _packet->finalise();
    writePacket(_packetBuffer.data(), _packet->packetSize(), true);
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

  std::lock_guard<Lock> guard(_packetLock);
  if (shape.writeUpdate(*_packet))
  {
    _packet->finalise();

    writePacket(_packetBuffer.data(), _packet->packetSize(), true);
    return _packet->packetSize();
  }
  return -1;
}


int BaseConnection::updateTransfers(unsigned byteLimit)
{
  if (!_active)
  {
    return 0;
  }

  std::lock_guard<Lock> guard(_packetLock);
  unsigned transferred = 0;

  while ((!byteLimit || transferred < byteLimit) && (!_currentResource->isNull() || !_resourceQueue.empty()))
  {
    bool startNext = false;
    if (!_currentResource->isNull())
    {
      if (_currentResource->nextPacket(*_packet, byteLimit ? byteLimit - transferred : 0))
      {
        _packet->finalise();
        writePacket(_packet->data(), _packet->packetSize(), true);
        transferred += _packet->packetSize();
      }

      // Completed
      if (_currentResource->isNull())
      {
        uint64_t completedId = _currentResource->lastCompletedId();
        startNext = true;
        auto resourceInfo = _resources.find(completedId);
        if (resourceInfo != _resources.end())
        {
          resourceInfo->second.sent = true;
        }
      }
    }
    else
    {
      startNext = !_resourceQueue.empty();
    }

    if (startNext)
    {
      if (!_resourceQueue.empty())
      {
        uint64_t nextResource = _resourceQueue.front();
        _resourceQueue.pop_front();
        auto resourceInfo = _resources.find(nextResource);
        if (resourceInfo != _resources.end())
        {
          const Resource *resource = resourceInfo->second.resource;
          resourceInfo->second.started = true;
          _currentResource->transfer(resource);
        }
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

  //std::lock_guard<Lock> guard(_lock);
  int wrote = -1;
  ControlMessage msg;
  msg.controlFlags = !flush * CFFramePersist;
  // Convert dt to desired time unit.
  msg.value32 = uint32_t(dt * _secondsToTimeUnit);
  msg.value64 = 0;
  std::lock_guard<Lock> guard(_packetLock);
  // Send frame number too?
  _packet->reset(MtControl, CIdFrame);
  if (msg.write(*_packet))
  {
    _packet->finalise();
    wrote = writePacket(_packetBuffer.data(), _packet->packetSize(), !(_serverFlags & SF_NakedFrameMessage));
  }
  flushCollatedPacket();
  return wrote;
}


unsigned BaseConnection::referenceResource(const Resource *resource)
{
  if (!_active)
  {
    return 0;
  }

  unsigned refCount = 0;
  uint64_t resId = resource->uniqueKey();
  auto existing = _resources.find(resId);
  if (existing != _resources.end())
  {
    refCount = ++existing->second.referenceCount;
  }
  else
  {
    ResourceInfo info(resource);
    _resources.insert(std::make_pair(resId, info));
    _resourceQueue.push_back(resId);
    refCount = 1;
  }

  return refCount;
}


unsigned BaseConnection::releaseResource(const Resource *resource)
{
  if (!_active)
  {
    return 0;
  }

  return releaseResource(resource->uniqueKey());
}


unsigned BaseConnection::releaseResource(uint64_t resourceId)
{
  unsigned refCount = 0;
  auto existing = _resources.find(resourceId);
  if (existing != _resources.end())
  {
    if (existing->second.referenceCount > 1)
    {
      refCount = --existing->second.referenceCount;
    }
    else
    {
      if (_currentResource->resource() && _currentResource->resource()->uniqueKey() == resourceId)
      {
        _currentResource->cancel();
      }

      if (existing->second.started || existing->second.sent)
      {
        // Send destroy message.
        _packet->reset();
        existing->second.resource->destroy(*_packet);
        _packet->finalise();
        writePacket(_packetBuffer.data(), _packet->packetSize(), true);
      }

      _resources.erase(existing);
    }
  }

  return refCount;
}


void BaseConnection::flushCollatedPacket()
{
  std::lock_guard<Lock> guard(_sendLock);
  flushCollatedPacketUnguarded();
}


void BaseConnection::flushCollatedPacketUnguarded()
{
  if (_collation->collatedBytes())
  {
    _collation->finalise();
    unsigned byteCount = 0;
    const uint8_t *bytes = _collation->buffer(byteCount);
    if (bytes && byteCount)
    {
      writeBytes(bytes, (int)byteCount);
    }
    _collation->reset();
  }
}


int BaseConnection::writePacket(const uint8_t *buffer, uint16_t byteCount, bool allowCollation)
{
  std::unique_lock<Lock> guard(_sendLock);

  if ((SF_Collate & _serverFlags) != 0 && !allowCollation)
  {
    flushCollatedPacketUnguarded();
  }

  if ((SF_Collate & _serverFlags) == 0 || !allowCollation)
  {
    return writeBytes(buffer, byteCount);
  }

  // Add to the collection buffer.
  if (byteCount >= _collation->availableBytes())
  {
    flushCollatedPacketUnguarded();
  }

  int sendCount = _collation->add(buffer, byteCount);
  if (sendCount == -1)
  {
    // Failed to collate. Packet may be too big to collated (due to collation overhead).
    // Flush the buffer, then send without collation.
    flushCollatedPacketUnguarded();
    sendCount = writeBytes(buffer, byteCount);
  }

  return sendCount;
}


void BaseConnection::ensurePacketBufferCapacity(size_t size)
{
  if (_packetBuffer.capacity() < size)
  {
    // Need to reallocated. Adjust packet pointer.
    _packetBuffer.resize(size);
    delete _packet;
    _packet = new PacketWriter(_packetBuffer.data(), (uint16_t)_packetBuffer.size());
  }
  else if (_packetBuffer.size() < size)
  {
    // Resize without reallocation. No buffer adjustments required.
    _packetBuffer.resize(size);
  }
}
