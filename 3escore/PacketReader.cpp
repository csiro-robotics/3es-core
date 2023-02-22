//
// author: Kazys Stepanas
//
#include "PacketReader.h"

#include "Crc.h"
#include "Endian.h"

#include <cstring>
#include <utility>

namespace tes
{

PacketReader::PacketReader(const PacketHeader *packet)
  : PacketStream<const PacketHeader>(packet)
{
  seek(0, Begin);
}


PacketReader::PacketReader(PacketReader &&other) noexcept
  : PacketStream<const PacketHeader>(nullptr)
{
  _packet = std::exchange(other._packet, nullptr);
  _status = std::exchange(other._status, Ok);
  _payload_position = std::exchange(other._payload_position, 0);
}


PacketReader &PacketReader::operator=(PacketReader &&other) noexcept
{
  other.swap(*this);
  return *this;
}


void PacketReader::swap(PacketReader &other) noexcept
{
  std::swap(_packet, other._packet);
  std::swap(_status, other._status);
  std::swap(_payload_position, other._payload_position);
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

  const CrcType packet_crc = crc();
  const CrcType crc_val = calculateCrc();
  if (crc_val == packet_crc)
  {
    _status |= CrcValid;
    return true;
  }
  return false;
}


PacketReader::CrcType PacketReader::calculateCrc() const
{
  const CrcType crc_val =
    crc16(reinterpret_cast<const uint8_t *>(_packet), sizeof(PacketHeader) + payloadSize());
  return crc_val;
}


size_t PacketReader::readElement(uint8_t *bytes, size_t element_size)
{
  if (bytesAvailable() >= element_size)
  {
    std::memcpy(bytes, payload() + _payload_position, element_size);
    networkEndianSwap(bytes, element_size);
    _payload_position = static_cast<uint16_t>(_payload_position + element_size);
    return element_size;
  }

  return 0;
}


size_t PacketReader::readArray(uint8_t *bytes, size_t element_size, size_t element_count)
{
  size_t copy_count = bytesAvailable() / element_size;
  if (copy_count > 0)
  {
    copy_count = (copy_count > element_count) ? element_count : copy_count;
    std::memcpy(bytes, payload() + _payload_position, copy_count * element_size);
#if !TES_IS_NETWORK_ENDIAN
    uint8_t *fix_bytes = bytes;
    for (unsigned i = 0; i < copy_count; ++i, fix_bytes += element_size)
    {
      networkEndianSwap(fix_bytes, element_size);
    }
#endif  // !TES_IS_NETWORK_ENDIAN
    _payload_position = static_cast<uint16_t>(_payload_position + element_size * copy_count);
    return copy_count;
  }

  return 0;
}


size_t PacketReader::readRaw(uint8_t *bytes, size_t byte_count)
{
  const size_t copy_count = (byte_count <= bytesAvailable()) ? byte_count : bytesAvailable();
  std::memcpy(bytes, payload() + _payload_position, copy_count);
  _payload_position = static_cast<uint16_t>(_payload_position + copy_count);
  return copy_count;
}


size_t PacketReader::peek(uint8_t *dst, size_t byte_count, bool allow_byte_swap)
{
  const size_t copy_count = (byte_count <= bytesAvailable()) ? byte_count : bytesAvailable();
  std::memcpy(dst, payload() + _payload_position, copy_count);
  // Do not adjust the payload position.

  if (allow_byte_swap)
  {
    networkEndianSwap(dst, byte_count);
  }

  return copy_count;
}
}  // namespace tes
