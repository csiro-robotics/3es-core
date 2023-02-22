//
// author: Kazys Stepanas
//
#ifndef TES_CORE_PACKET_BUFFER_H
#define TES_CORE_PACKET_BUFFER_H

#include "CoreConfig.h"

#include <array>
#include <cinttypes>
#include <vector>

namespace tes
{
struct PacketHeader;

/// This class accepts responsibility for collating incoming byte streams.
///
/// Data is buffered until full packets have arrived, which must be extracted using
/// @c extractPacket().
///
/// @note @c PacketStreamReader is recommended over using @c PacketBuffer.
///
/// @todo Deprecate this class in favour of @c PacketStreamReader.
class TES_CORE_API PacketBuffer
{
public:
  /// Constructors 2Kb buffer.
  PacketBuffer(size_t capacity = 2048u);
  /// Destructor.
  ~PacketBuffer();

  /// Adds @c bytes to the buffer.
  ///
  /// Data are rejected if the marker is not present or, if present, data before the marker are
  /// rejected.
  ///
  /// @return the index of the first accepted byte or -1 if all are rejected.
  int addBytes(const uint8_t *bytes, size_t byte_count);

  /// @overload
  template <int N>
  int addBytes(const std::array<uint8_t, N> &bytes)
  {
    return addBytes(bytes.data(), bytes.size());
  }

  /// @overload
  int addBytes(const std::vector<uint8_t> &bytes) { return addBytes(bytes.data(), bytes.size()); }

  /// Extract the first valid packet in the buffer. Additional packets may be left available.
  ///
  /// The packet is extracted into the @p buffer, which is used to avoid memory allocation on each
  /// extract call. When a packet is available, the @p buffer is sized sufficiently to store the
  /// entire packet, then the packet is copied into the @p buffer. The return value is the same
  /// address as @p buffer.data(), but converted to the @c PacketHeader type.
  ///
  /// @param buffer A byte array to copy the packet into.
  /// @return A valid packet pointer if available, null if none available.
  PacketHeader *extractPacket(std::vector<uint8_t> &buffer);

private:
  /// Grow the packet buffer with @p bytes.
  /// @param bytes Data to append.
  /// @param byte_count Number of bytes from @p bytes to append.
  template <typename Iter>
  void appendData(const Iter &begin, const Iter &end);

  /// Remove the first @p byte_count bytes from the packet buffer.
  /// @param byte_count The number of bytes to remove from the buffer.
  void removeData(size_t byte_count);

  std::vector<uint8_t> _packet_buffer;  ///< Buffers incoming packet data.
  bool _marker_found = false;           ///< Has the @c PacketHeader marker been found?
};
}  // namespace tes

#endif  // TES_CORE_PACKET_BUFFER_H
