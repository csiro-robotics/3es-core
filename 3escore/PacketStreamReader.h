//
// author: Kazys Stepanas
//
#ifndef TES_CORE_PACKET_STREAM_READER_H
#define TES_CORE_PACKET_STREAM_READER_H

#include "CoreConfig.h"

#include "PacketHeader.h"

#include <array>
#include <cinttypes>
#include <istream>
#include <memory>
#include <vector>

namespace tes
{
struct PacketHeader;

/// A utility class which reads packets from a std::istream .
///
/// This collects bytes until a full packet is collected whenever
/// @c extractPacket() is called, provided there are sufficient bytes available.
/// A @c PacketReader is still required to decode the contents of the resulting
/// @c PacketHeader data.
class TES_CORE_API PacketStreamReader
{
public:
  /// Default constructor. The resulting reader is invalid. Use @c setStream() to initialise.
  PacketStreamReader();
  /// Construct a stream reader for the given stream.
  /// @param stream The stream to read.
  PacketStreamReader(std::shared_ptr<std::istream> stream);
  ~PacketStreamReader();

  /// Check if the stream is ok for more reading.
  /// @return True if ok to read on.
  [[nodiscard]] bool isOk() const { return _stream && _stream->good(); }

  /// Check if the stream is at the end of file.
  /// @return True if at end of file.
  [[nodiscard]] bool isEof() const { return !_stream || _stream->eof(); }

  /// (Re)set the stream to read from.
  /// @param stream The stream to read from.
  void setStream(std::shared_ptr<std::istream> stream);
  /// Get the stream in use.
  /// @return The current stream - may be null.
  [[nodiscard]] std::shared_ptr<std::istream> stream() const { return _stream; }

  /// Try extract the next packet from the stream. The packet pointer remains
  /// valid until the next call to @c extractPacket(). This object retains the
  /// ownership.
  ///
  /// @return The next packet or null on failure. Check status on failure.
  const PacketHeader *extractPacket();

  /// Seek to the given stream position.
  ///
  /// This clears the current data buffer, invalidating results from @c extractPacket().
  ///
  /// @param position The stream byte offset to seek to.
  void seek(std::istream::pos_type position);

private:
  size_t readMore(size_t more_count);
  bool checkMarker(std::vector<uint8_t> &buffer, size_t i);
  /// Consume the packet at the head of the buffer (if valid and able).
  void consume();

  /// Calculate the expected packet size for the packet at the head of the buffer.
  ///
  /// Only valid to call when we have a verified header at the _buffer start.
  /// @return The expected packet size including payload as indicated by the packet header.
  size_t calcExpectedSize();

  std::shared_ptr<std::istream> _stream;
  std::array<uint8_t, sizeof(tes::kPacketMarker)> _marker_bytes;
  std::vector<uint8_t> _buffer;
  size_t _chunk_size = 1024u;
};
}  // namespace tes

#endif  // TES_CORE_PACKET_STREAM_READER_H
