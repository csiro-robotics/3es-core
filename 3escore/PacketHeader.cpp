//
// author: Kazys Stepanas
//
#include "PacketHeader.h"

#include "Endian.h"

namespace tes
{
const uint32_t kPacketMarker = 0x03e55e30u;
const uint16_t kPacketVersionMajor = 0u;
const uint16_t kPacketVersionMinor = 4u;
const uint16_t kPacketCompatibilityVersionMajor = 0u;
const uint16_t kPacketCompatibilityVersionMinor = 3u;
}  // namespace tes
