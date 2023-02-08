//
// author: Kazys Stepanas
//
#include "PacketWriter.h"

#include "Crc.h"
#include "Endian.h"

#include <cstring>
#include <utility>

namespace tes
{
PacketWriter::PacketWriter(PacketHeader *packet, uint16_t max_payload_size, uint16_t routing_id,
                           uint16_t message_id)
  : PacketStream<PacketHeader>(packet)
  , _buffer_size(static_cast<uint16_t>(max_payload_size + sizeof(PacketHeader)))
{
  _packet->marker = kPacketMarker;
  _packet->version_major = kPacketVersionMajor;
  _packet->version_minor = kPacketVersionMinor;
  _packet->routing_id = networkEndianSwapValue(routing_id);
  _packet->message_id = networkEndianSwapValue(message_id);
  _packet->payload_size = 0u;
  _packet->payload_offset = 0u;
  _packet->flags = 0u;
}


PacketWriter::PacketWriter(uint8_t *buffer, uint16_t buffer_size, uint16_t routing_id,
                           uint16_t message_id)
  : PacketStream<PacketHeader>(reinterpret_cast<PacketHeader *>(buffer))
{
  _buffer_size = buffer_size;
  if (buffer_size >= sizeof(PacketHeader) + sizeof(CrcType))
  {
    _packet->marker = networkEndianSwapValue(kPacketMarker);
    _packet->version_major = networkEndianSwapValue(kPacketVersionMajor);
    _packet->version_minor = networkEndianSwapValue(kPacketVersionMinor);
    _packet->routing_id = networkEndianSwapValue(routing_id);
    _packet->message_id = networkEndianSwapValue(message_id);
    ;
    _packet->payload_size = 0u;
    _packet->payload_offset = 0u;
    _packet->flags = 0u;
  }
  else
  {
    _status |= Fail;
  }
}


PacketWriter::PacketWriter(const PacketWriter &other)
  : PacketStream<PacketHeader>(reinterpret_cast<PacketHeader *>(other._packet))
  , _buffer_size(other._buffer_size)
{
  _status = other._status;
  _payload_position = other._payload_position;
}


PacketWriter::PacketWriter(PacketWriter &&other) noexcept
  : PacketStream<PacketHeader>(nullptr)
{
  _packet = std::exchange(other._packet, nullptr);
  _status = std::exchange(other._status, Ok);
  _payload_position = std::exchange(other._payload_position, 0);
  _buffer_size = std::exchange(other._buffer_size, 0);
}


PacketWriter::~PacketWriter() = default;


PacketWriter &PacketWriter::operator=(PacketWriter other)
{
  other.swap(*this);
  return *this;
}


void PacketWriter::swap(PacketWriter &other)
{
  std::swap(_packet, other._packet);
  std::swap(_status, other._status);
  std::swap(_payload_position, other._payload_position);
  std::swap(_buffer_size, other._buffer_size);
}


void PacketWriter::reset(uint16_t routing_id, uint16_t message_id)
{
  _status = Ok;
  if (_buffer_size >= sizeof(PacketHeader))
  {
    _packet->routing_id = networkEndianSwapValue(routing_id);
    _packet->message_id = networkEndianSwapValue(message_id);
    _packet->payload_size = 0u;
    _packet->payload_offset = 0u;
    _packet->flags = 0u;
    _payload_position = 0;
  }
  else
  {
    _status |= Fail;
  }
}


uint16_t PacketWriter::bytesRemaining() const
{
  return static_cast<uint16_t>(maxPayloadSize() - payloadSize());
}


uint16_t PacketWriter::maxPayloadSize() const
{
  return (!isFail()) ? static_cast<uint16_t>(_buffer_size - sizeof(PacketHeader)) : 0u;
}


bool PacketWriter::finalise()
{
  if (!isFail())
  {
    calculateCrc();
  }
  return !isFail();
}


PacketWriter::CrcType PacketWriter::calculateCrc()
{
  if (isCrcValid())
  {
    return crc();
  }

  if (isFail())
  {
    return 0u;
  }

  if (packet().flags & PFNoCrc)
  {
    // No CRC requested.
    _status |= CrcValid;
    return 0;
  }

  CrcType *crc_pos = crcPtr();
  // Validate the CRC position for buffer overflow.
  const auto crc_offset = static_cast<unsigned>(reinterpret_cast<uint8_t *>(crc_pos) -
                                                reinterpret_cast<uint8_t *>(_packet));
  if (crc_offset > _buffer_size - sizeof(CrcType))
  {
    // CRC overruns the buffer. Cannot calculate.
    _status |= Fail;
    return 0;
  }

  const CrcType crc_val =
    crc16(reinterpret_cast<const uint8_t *>(_packet), sizeof(PacketHeader) + payloadSize());
  *crc_pos = networkEndianSwapValue(crc_val);
  _status |= CrcValid;
  return *crc_pos;
}


size_t PacketWriter::writeElement(const uint8_t *bytes, size_t element_size)
{
  if (bytesRemaining() >= element_size)
  {
    memcpy(payloadWritePtr(), bytes, element_size);
    networkEndianSwap(payloadWritePtr(), element_size);
    _payload_position = static_cast<uint16_t>(_payload_position + element_size);
    incrementPayloadSize(element_size);
    return element_size;
  }

  return 0;
}


size_t PacketWriter::writeArray(const uint8_t *bytes, size_t element_size, size_t element_count)
{
  size_t copy_count = bytesRemaining() / element_size;
  if (copy_count > 0)
  {
    copy_count = (copy_count > element_count) ? element_count : copy_count;
    memcpy(payloadWritePtr(), bytes, copy_count * element_size);
#if !TES_IS_NETWORK_ENDIAN
    uint8_t *fix_bytes = payloadWritePtr();
    for (unsigned i = 0; i < copy_count; ++i, fix_bytes += element_size)
    {
      networkEndianSwap(fix_bytes, element_size);
    }
#endif  // !TES_IS_NETWORK_ENDIAN
    incrementPayloadSize(element_size * copy_count);
    _payload_position = static_cast<uint16_t>(_payload_position + element_size * copy_count);
    return copy_count;
  }

  return 0;
}


size_t PacketWriter::writeRaw(const uint8_t *bytes, size_t byte_count)
{
  const size_t copy_count = (byte_count <= bytesRemaining()) ? byte_count : bytesRemaining();
  memcpy(payloadWritePtr(), bytes, copy_count);
  incrementPayloadSize(copy_count);
  _payload_position = static_cast<uint16_t>(_payload_position + copy_count);
  return copy_count;
}


void PacketWriter::incrementPayloadSize(size_t inc)
{
  _packet->payload_size = static_cast<uint16_t>(payloadSize() + inc);
  networkEndianSwap(_packet->payload_size);
  invalidateCrc();
}
}  // namespace tes
