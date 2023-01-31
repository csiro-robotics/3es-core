//
// author: Kazys Stepanas
//
#ifndef TES_CORE_PACKET_H
#define TES_CORE_PACKET_H

#include "CoreConfig.h"

#include <cinttypes>

namespace tes
{
/// Marker value identifying a packet header in local Endian.
extern const uint32_t TES_CORE_API PacketMarker;
/// Expected packet major version local Endian.
extern const uint16_t TES_CORE_API PacketVersionMajor;
/// Expected packet minor version local Endian.
extern const uint16_t TES_CORE_API PacketVersionMinor;

/// Flag values for @c PacketHeader objects.
enum PacketFlag
{
  /// Marks a @c PacketHeader as missing its 16-bit CRC.
  PF_NoCrc = (1 << 0)
};

/// The header for an incoming 3ES data packet. All packet data, including payload
/// bytes, must be in network endian which is big endian.
///
/// A two byte CRC value is to appear immediately after the @p PacketHeader header and
/// payload.
struct TES_CORE_API PacketHeader
{
  uint32_t marker;        ///< Marker bytes. Identifies the packet start.
  uint16_t versionMajor;  ///< PacketHeader major version number. May be used to control decoding.
  uint16_t versionMinor;  ///< PacketHeader minor version number. May be used to control decoding.
  /// Identifies the main packet receiver.
  uint16_t routingId;
  /// Identifies the message ID or message type.
  uint16_t messageId;
  uint16_t payloadSize;  ///< Size of the payload following this header.
  /// Offset from the end of this header to the payload.
  uint8_t payloadOffset;
  /// @c PacketFlag values.
  uint8_t flags;
};
}  // namespace tes

#endif  // TES_CORE_PACKET_H
