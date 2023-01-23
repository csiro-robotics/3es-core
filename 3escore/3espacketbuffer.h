//
// author: Kazys Stepanas
//
#ifndef _3ESPACKETBUFFER_H_
#define _3ESPACKETBUFFER_H_

#include "CoreConfig.h"

#include <cinttypes>
#include <vector>

namespace tes
{
struct PacketHeader;

/// This class accepts responsibility for collating incoming byte streams.
/// Data is buffered until full packets have arrived, which must be extracted
/// using @c extractPacket().
class _3es_coreAPI PacketBuffer
{
public:
  /// Constructors 2Kb buffer.
  PacketBuffer();
  /// Destructor.
  ~PacketBuffer();

  /// Adds @c bytes to the buffer.
  ///
  /// Data are rejected if the marker is not present or, if present,
  /// data before the marker are rejected.
  ///
  /// @return the index of the first accepted byte or -1 if all are rejected.
  int addBytes(const uint8_t *bytes, size_t byteCount);

  /// Extract the first valid packet in the buffer. Additional packets
  /// may be left available.
  ///
  /// The packet is extracted into the @p buffer, which is used to avoid
  /// memory allocation on each extract call. When a packet is available,
  /// the @p buffer is sized sufficiently to store the entire packet, then
  /// the packet is copied into the @p buffer. The return value is the same
  /// address as @p buffer.data(), but converted to the @c PacketHeader type.
  ///
  /// @param buffer A byte array to copy the packet into.
  /// @return A valid packet pointer if available, null if none available.
  PacketHeader *extractPacket(std::vector<uint8_t> &buffer);

private:
  /// Grow the packet buffer with @p bytes.
  /// @param bytes Data to append.
  /// @param byteCount Number of bytes from @p bytes to append.
  void appendData(const uint8_t *bytes, size_t byteCount);

  /// Remove the first @p byteCount bytes from the packet buffer.
  /// @param byteCount The number of bytes to remove from the buffer.
  void removeData(size_t byteCount);

  uint8_t *_packetBuffer;  ///< Buffers incoming packet data.
  size_t _byteCount;       ///< Number of data bytes currently stored in @c _packetBuffer;
  size_t _bufferSize;      ///< Size of @c _packetBuffer in bytes;
  bool _markerFound;       ///< Has the @c PacketHeader marker been found?
};
}  // namespace tes

#endif  // _3ESPACKETBUFFER_H_
