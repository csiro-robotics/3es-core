//
// author: Kazys Stepanas
//
#include "PacketBuffer.h"

#include "PacketHeader.h"
#include "PacketReader.h"

#include <algorithm>
#include <cstring>

using namespace tes;

namespace
{
int packetMarkerPosition(const uint8_t *bytes, size_t byteCount)
{
  uint32_t packetMarker = networkEndianSwapValue(tes::PacketMarker);
  const uint8_t *markerBytes = (const uint8_t *)&packetMarker;
  for (size_t i = 0; i < byteCount; i += 4)
  {
    if (bytes[i] == *markerBytes)
    {
      // First marker byte found. Check for the rest.
      bool found = true;
      for (unsigned j = 1; j < sizeof(packetMarker); ++j)
      {
        found = found && bytes[i + j] == markerBytes[j];
      }

      if (found)
      {
        return int(i);
      }
    }
  }

  return -1;
}
}  // namespace

PacketBuffer::PacketBuffer()
  : _packetBuffer(nullptr)
  , _byteCount(0u)
  , _bufferSize(2048)
  , _markerFound(false)
{
  _packetBuffer = new uint8_t[_bufferSize];
}


PacketBuffer::~PacketBuffer()
{
  delete[] _packetBuffer;
}


int PacketBuffer::addBytes(const uint8_t *bytes, size_t byteCount)
{
  if (_markerFound)
  {
    appendData(bytes, byteCount);
    // All bytes accepted.
    return 0;
  }

  // Clear for the marker in the incoming bytes.
  // Reject bytes not found.
  int markerPos = packetMarkerPosition(bytes, byteCount);
  if (markerPos >= 0)
  {
    _markerFound = true;
    appendData(bytes + markerPos, byteCount - size_t(markerPos));
    return markerPos;
  }

  return -1;
}


PacketHeader *PacketBuffer::extractPacket(std::vector<uint8_t> &buffer)
{
  if (_markerFound && _byteCount >= sizeof(PacketHeader))
  {
    // Remember, the CRC appears after the packet payload. We have to include
    // that in our mem copy.
    PacketHeader *pending = reinterpret_cast<PacketHeader *>(_packetBuffer);
    PacketReader reader(pending);
    if (sizeof(PacketHeader) + reader.payloadSize() + sizeof(PacketReader::CrcType) <= _byteCount)
    {
      // We have a full packet. Allocate a copy and extract the full packet data.
      const unsigned packetSize = reader.packetSize();
      buffer.resize(packetSize);
      // FIXME(KS): why allocate? Can't we hold a pointer in the buffer, then shift bytes on release?
      std::copy(_packetBuffer, _packetBuffer + packetSize, buffer.begin());

      _markerFound = false;
      if (_byteCount > packetSize)
      {
        // Find next marker beyond the packet just returned.
        int nextMarkerPos = packetMarkerPosition(_packetBuffer + packetSize, _byteCount - packetSize);
        if (nextMarkerPos >= 0)
        {
          removeData(packetSize + unsigned(nextMarkerPos));
          _markerFound = true;
        }
        else
        {
          // No new marker. Remove all data.
          removeData(_byteCount);
        }
      }
      else
      {
        removeData(packetSize);
      }

      return reinterpret_cast<PacketHeader *>(buffer.data());
    }
  }

  return nullptr;
}


void PacketBuffer::appendData(const uint8_t *bytes, size_t byteCount)
{
  if (_byteCount + byteCount > _bufferSize)
  {
    // Resize the buffer.
    size_t growBy = std::max<size_t>(1024u, byteCount);
    uint8_t *newBuffer = new uint8_t[_bufferSize + growBy];
    memcpy(newBuffer, _packetBuffer, _byteCount);
    delete[] _packetBuffer;
    _packetBuffer = newBuffer;
    _bufferSize += growBy;
  }

  memcpy(_packetBuffer + _byteCount, bytes, byteCount);
  _byteCount += byteCount;
}


void PacketBuffer::removeData(size_t byteCount)
{
  if (_byteCount > byteCount)
  {
    memmove(_packetBuffer, _packetBuffer + byteCount, _byteCount - byteCount);
    _byteCount -= byteCount;
  }
  else
  {
    _byteCount = 0u;
  }
}
