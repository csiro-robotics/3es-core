//
// author: Kazys Stepanas
//
#include "PacketReader.h"

#include "Crc.h"
#include "Endian.h"

#include <cstring>
#include <utility>

using namespace tes;

PacketReader::PacketReader(const PacketHeader *packet)
  : PacketStream<const PacketHeader>(packet)
{
  seek(0, Begin);
}


PacketReader::PacketReader(PacketReader &&other)
  : PacketStream<const PacketHeader>(nullptr)
{
  _packet = std::exchange(other._packet, nullptr);
  _status = std::exchange(other._status, Ok);
  _payloadPosition = std::exchange(other._payloadPosition, 0);
}


PacketReader &PacketReader::operator=(PacketReader &&other)
{
  other.swap(*this);
  return *this;
}


void PacketReader::swap(PacketReader &other)
{
  std::swap(_packet, other._packet);
  std::swap(_status, other._status);
  std::swap(_payloadPosition, other._payloadPosition);
}


bool PacketReader::checkCrc()
{
  if (isCrcValid())
  {
    return true;
  }

  if ((flags() & PFNoCrc))
  {
    _status |= CrcValid;
    return true;
  }

  const CrcType packetCrc = crc();
  const CrcType crcVal = calculateCrc();
  if (crcVal == packetCrc)
  {
    _status |= CrcValid;
    return true;
  }
  return false;
}


PacketReader::CrcType PacketReader::calculateCrc() const
{
  const CrcType crcVal = crc16(reinterpret_cast<const uint8_t *>(_packet), sizeof(PacketHeader) + payloadSize());
  return crcVal;
}


size_t PacketReader::readElement(uint8_t *bytes, size_t elementSize)
{
  if (bytesAvailable() >= elementSize)
  {
    memcpy(bytes, payload() + _payloadPosition, elementSize);
    networkEndianSwap(bytes, elementSize);
    _payloadPosition = uint16_t(_payloadPosition + elementSize);
    return elementSize;
  }

  return 0;
}


size_t PacketReader::readArray(uint8_t *bytes, size_t elementSize, size_t elementCount)
{
  size_t copyCount = bytesAvailable() / elementSize;
  if (copyCount > 0)
  {
    copyCount = (copyCount > elementCount) ? elementCount : copyCount;
    memcpy(bytes, payload() + _payloadPosition, copyCount * elementSize);
#if !TES_IS_NETWORK_ENDIAN
    uint8_t *fixBytes = bytes;
    for (unsigned i = 0; i < copyCount; ++i, fixBytes += elementSize)
    {
      networkEndianSwap(fixBytes, elementSize);
    }
#endif  // !TES_IS_NETWORK_ENDIAN
    _payloadPosition = uint16_t(_payloadPosition + elementSize * copyCount);
    return copyCount;
  }

  return 0;
}


size_t PacketReader::readRaw(uint8_t *bytes, size_t byteCount)
{
  size_t copyCount = (byteCount <= bytesAvailable()) ? byteCount : bytesAvailable();
  memcpy(bytes, payload() + _payloadPosition, copyCount);
  _payloadPosition = uint16_t(_payloadPosition + copyCount);
  return copyCount;
}


size_t PacketReader::peek(uint8_t *dst, size_t byteCount, bool allowByteSwap)
{
  size_t copyCount = (byteCount <= bytesAvailable()) ? byteCount : bytesAvailable();
  memcpy(dst, payload() + _payloadPosition, copyCount);
  // Do not adjust the payload position.

  if (allowByteSwap)
  {
    networkEndianSwap(dst, byteCount);
  }

  return copyCount;
}
