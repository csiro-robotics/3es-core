//
// author: Kazys Stepanas
//
#ifndef TES_CORE_HEADER_STREAM_H
#define TES_CORE_HEADER_STREAM_H

#include "CoreConfig.h"

#include "Endian.h"
#include "PacketHeader.h"

namespace tes
{
/// A utility class used for managing read/write operations to a @c PacketHeader payload.
///
/// The template type is intended to be either a @c PacketReader or a @c const @c PacketHeader
/// for use with @c PacketWriter and @c PacketReader respectively.
template <class HEADER>
class PacketStream
{
public:
  /// Defies the packet CRC type.
  using CrcType = uint16_t;

  /// Control values for seeking.
  enum SeekPos
  {
    Begin,    ///< Seek from the beginning of the stream.
    Current,  ///< Seek from the current position.
    End       ///< Seek from the end of the stream.
  };

  /// Status bits.
  enum Status : unsigned
  {
    /// No issues.
    Ok = 0,
    /// End at of packet/stream.
    EOP = (1u << 0u),
    /// Set after an operation fails.
    Fail = (1u << 1u),
    /// Read only stream?
    ReadOnly = (1u << 2u),
    /// Is the CRC valid?
    CrcValid = (1u << 3u),
  };

  /// Create a stream to read from beginning at @p packet.
  /// @param packet The beginning of the data packet.
  PacketStream(HEADER *packet);

  // PacketHeader member access. Ensures network endian swap as required.
  /// Fetch the marker bytes in local endian.
  /// @return The @c PacketHeader::marker bytes.
  [[nodiscard]] uint32_t marker() const { return networkEndianSwapValue(_packet->marker); }
  /// Fetch the major version bytes in local endian.
  /// @return The @c PacketHeader::version_major bytes.
  [[nodiscard]] uint16_t versionMajor() const
  {
    return networkEndianSwapValue(_packet->version_major);
  }
  /// Fetch the minor version bytes in local endian.
  /// @return The @c PacketHeader::version_minor bytes.
  [[nodiscard]] uint16_t versionMinor() const
  {
    return networkEndianSwapValue(_packet->version_minor);
  }
  /// Fetch the payload size bytes in local endian.
  /// @return The @c PacketHeader::payloadSize bytes.
  [[nodiscard]] uint16_t payloadSize() const
  {
    return networkEndianSwapValue(_packet->payload_size);
  }
  /// Returns the size of the packet plus payload, giving the full data packet size including the
  /// CRC.
  /// @return PacketHeader data size (bytes).
  [[nodiscard]] uint16_t packetSize() const
  {
    return static_cast<uint16_t>(sizeof(HEADER) + payloadSize() +
                                 (((packet().flags & PFNoCrc) == 0) ? sizeof(CrcType) : 0));
  }
  /// Fetch the routing ID bytes in local endian.
  /// @return The @c PacketHeader::routing_id bytes.
  [[nodiscard]] uint16_t routingId() const { return networkEndianSwapValue(_packet->routing_id); }
  /// Fetch the message ID bytes in local endian.
  /// @return The @c PacketHeader::message_id bytes.
  [[nodiscard]] uint16_t messageId() const { return networkEndianSwapValue(_packet->message_id); }
  /// Fetch the flags bytes in local endian.
  /// @return the @c PacketHeader::flags bytes.
  [[nodiscard]] uint8_t flags() const { return networkEndianSwapValue(_packet->flags); }
  /// Fetch the CRC bytes in local endian.
  /// Invalid for packets with the @c PFNoCrc flag set.
  /// @return The packet's CRC value.
  [[nodiscard]] CrcType crc() const { return networkEndianSwapValue(*crcPtr()); }
  /// Fetch a pointer to the CRC bytes.
  /// Invalid for packets with the @c PFNoCrc flag set.
  /// @return A pointer to the CRC location.
  [[nodiscard]] CrcType *crcPtr();
  /// @overload
  [[nodiscard]] const CrcType *crcPtr() const;

  /// Report the @c Status bits.
  /// @return The @c Status flags.
  [[nodiscard]] uint16_t status() const;

  /// At end of packet/stream?
  /// @return True if at end of packet.
  [[nodiscard]] bool isEop() const { return _status & EOP; }
  /// Status OK?
  /// @return True if OK
  [[nodiscard]] bool isOk() const { return !isFail(); }
  /// Fail bit set?
  /// @return True if fail bit is set.
  [[nodiscard]] bool isFail() const { return (_status & Fail) != 0; }
  /// Read only stream?
  /// @return True if read only.
  [[nodiscard]] bool isReadOnly() const { return (_status & ReadOnly) != 0; }
  /// CRC validated?
  /// @return True if CRC has been validated.
  [[nodiscard]] bool isCrcValid() const { return (_status & CrcValid) != 0; }

  /// Access the head of the packet buffer, for direct @p PacketHeader access.
  /// Note: values are in network Endian.
  /// @return A reference to the @c PacketHeader.
  [[nodiscard]] HEADER &packet() const;

  /// Tell the current stream position.
  /// @return The current position.
  [[nodiscard]] uint16_t tell() const;
  /// Seek to the indicated position.
  /// @param offset Seek offset from @p pos.
  /// @param pos The seek reference position.
  bool seek(int offset, SeekPos pos = Begin);
  /// Direct payload pointer access.
  /// @return The start of the payload bytes.
  [[nodiscard]] const uint8_t *payload() const;

protected:
  HEADER *_packet = nullptr;        ///< Packet header and buffer start address.
  uint16_t _status = Ok;            ///< @c Status bits.
  uint16_t _payload_position = 0u;  ///< Payload cursor.

  /// Type traits: is @c T const?
  template <class T>
  struct IsConst
  {
    /// Check the traits.
    /// @return True if @p T is const.
    [[nodiscard]] bool check() const { return false; }
  };

  /// Type traits: is @c T const?
  template <class T>
  struct IsConst<const T>
  {
    /// Check the traits.
    /// @return True if @p T is const.
    [[nodiscard]] bool check() const { return true; }
  };
};

TES_EXTERN template class TES_CORE_API PacketStream<PacketHeader>;
TES_EXTERN template class TES_CORE_API PacketStream<const PacketHeader>;

template <class HEADER>
PacketStream<HEADER>::PacketStream(HEADER *packet)
  : _packet(packet)
{
  if (IsConst<HEADER>().check())
  {
    _status |= ReadOnly;
  }
}


template <class HEADER>
bool PacketStream<HEADER>::seek(int offset, SeekPos pos)
{
  switch (pos)
  {
  case Begin:
    if (offset <= payloadSize())
    {
      _payload_position = static_cast<uint16_t>(offset);
      return true;
    }
    break;

  case Current:
    if (offset >= 0 && offset + _payload_position <= payloadSize() ||
        offset < 0 && _payload_position >= -offset)
    {
      _payload_position = static_cast<uint16_t>(_payload_position + offset);
      return true;
    }
    break;

  case End:
    if (offset < payloadSize())
    {
      _payload_position = static_cast<uint16_t>(_packet->payload_size - 1 - offset);
      return true;
    }
    break;

  default:
    break;
  }

  return false;
}


template <class HEADER>
typename PacketStream<HEADER>::CrcType *PacketStream<HEADER>::crcPtr()
{
  // CRC appears after the payload.
  // TODO(KS): fix the const correctness of this.
  uint8_t *pos = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(_packet)) +
                 sizeof(HEADER) + payloadSize();
  return reinterpret_cast<CrcType *>(pos);
}


template <class HEADER>
const typename PacketStream<HEADER>::CrcType *PacketStream<HEADER>::crcPtr() const
{
  // CRC appears after the payload.
  const uint8_t *pos = reinterpret_cast<const uint8_t *>(_packet) + sizeof(HEADER) + payloadSize();
  return reinterpret_cast<const CrcType *>(pos);
}


template <class HEADER>
inline uint16_t PacketStream<HEADER>::status() const
{
  return _status;
}

template <class HEADER>
inline HEADER &PacketStream<HEADER>::packet() const
{
  return *_packet;
}

template <class HEADER>
inline uint16_t PacketStream<HEADER>::tell() const
{
  return _payload_position;
}

template <class HEADER>
inline const uint8_t *PacketStream<HEADER>::payload() const
{
  return reinterpret_cast<const uint8_t *>(_packet) + sizeof(HEADER);
}
}  // namespace tes

#endif  // TES_CORE_HEADER_STREAM_H
