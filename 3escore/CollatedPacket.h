//
// author: Kazys Stepanas
//
#ifndef TES_CORE_COLLATED_PACKET_H
#define TES_CORE_COLLATED_PACKET_H

#include "CoreConfig.h"

//
#include "CompressionLevel.h"
#include "Connection.h"
#include "PacketHeader.h"

#include <vector>

namespace tes
{
struct CollatedPacketMessage;
struct CollatedPacketZip;
class PacketWriter;

/// A utility class which generates a @c MtCollatedPacket message by appending multiple
/// other messages. Compression may optionally be applied.
///
/// Typical usage:
/// - Instantiate the packet.
/// - Reset the packet.
/// - For each constituent message
///   - Generate the packet using @p PacketWriter.
///   - Finalise the message.
///   - Call @c add(const PacketWriter &)
/// - Call @c finalise() on the collated packet.
/// - Send the collated packet.
/// - Call @c reset()
///
/// The @c CollatedPacket also extends the @c Connection class in order to support
/// multi-threaded packet generation and synchronisation. While the @c Connection
/// and @c Connection implementations are required to be thread-safe, they
/// cannot guarantee packets area correctly collated by thread. Thus a
/// @c CollatedPacket can be used per thread to collate messages for each thread.
/// The packet content can then be sent as a single transaction.
///
/// By supporting the @c Connection methods, a @p CollatedPacket can be used in place
/// of a 'server' argument with various server utility macros and functions.
///
/// By default, a @c CollatedPacket has is limited to supporting @c MaxPacketSize bytes.
/// This allows a single packet with a single @c PacketHeader and collated packet
/// message with optional compression. However, when a collated packet is used for
/// transaction collation (as described in the multi-threaded case), it may require
/// collation of larger data sizes. In this case, the @c CollatedPacket(unsigned, unsigned)
/// constructor can be used to specify a larger collation buffer limit (the buffer resizes as
/// required). Such large, collated packets are sent using
/// @c Server::send(const CollatedPacket &). Internally, the method may either send
/// the packet as is (if small enough), or extract and reprocess each collated packet.
class TES_CORE_API CollatedPacket : public Connection
{
public:
  /// Byte count overhead added by using a @p CollatedPacket.
  /// This is the sum of @p PacketHeader, @c CollatedPacketMessage and the @c PacketWriter::CrcType.
  static const size_t Overhead;
  /// Initial cursor position in the write buffer.
  /// This is the sum of @p PacketHeader, @c CollatedPacketMessage.
  static const unsigned InitialCursorOffset;
  /// The default packet size limit for a @c CollatedPacketMessage.
  static constexpr uint16_t kMaxPacketSize = static_cast<uint16_t>(~0u);
  /// The default buffer size.
  static constexpr uint16_t kDefaultBufferSize = 0xff00u;

  /// Initialise a collated packet. This sets the initial packet size limited
  /// by @c kMaxPacketSize, and compression options.
  ///
  /// @param compress True to compress data as written.
  /// @param buffer_size The initial buffer_size
  /// @bug Specifying a buffer size too close to 0xffff (even correctly accounting for
  ///   the expected overhead) results in dropped packets despite the network layer
  ///   not reporting errors. Likely I'm missing some overhead detail. For now, use
  ///   a lower packet size.
  CollatedPacket(bool compress, uint16_t buffer_size = kDefaultBufferSize);

  /// Initialise a collated packet allowing packet sizes large than @p kMaxPacketSize.
  /// This is intended for collating messages to be send as a group in a thread-safe
  /// fashion. The maximum packet size may exceed the normal send limit. As such
  /// compression is not allowed to better support splitting.
  ///
  /// @param buffer_size The initial buffer_size
  /// @param max_packet_size The maximum packet size.
  CollatedPacket(unsigned buffer_size, unsigned max_packet_size);

  /// Destructor.
  ~CollatedPacket() override;

  /// Is compression enabled. Required ZLIB.
  /// @return True if compression is enabled.
  [[nodiscard]] bool compressionEnabled() const;

  /// Set the target compression level. Rejected if @p level is out of range of @c CompressionLevel.
  /// May be set even if compression is not enabled, but will have no effect.
  /// @param level The target level to set. See @c CompressionLevel.
  void setCompressionLevel(int level);

  /// Get the target compression level.
  /// @return The current compression level @c CompressionLevel.
  [[nodiscard]] int compressionLevel() const;

  /// Return the capacity of the collated packet.
  ///
  /// This defaults to 64 * 1024 - 1 (the maximum for a 16-bit unsigned integer),
  /// when using the constructor: @c CollatedPacket(bool, unsigned). It may be
  /// larger when using the @c CollatedPacket(unsigned, unsigned) constructor.
  /// See that constructor and class notes for details.
  ///
  /// @return The maximum packet capacity or 0xffffffffu if the packet size is variable.
  [[nodiscard]] unsigned maxPacketSize() const;

  /// Reset the collated packet, dropping any existing data.
  void reset();

  /// Add the packet data in @p packet to the collation buffer.
  ///
  /// The method will fail (return -1) when the @c maxPacketSize() has been reached.
  /// In this case, the packet should be sent and reset before trying again.
  /// The method will also fail if the packet has already been finalised using
  /// @c finalise().
  ///
  /// @param packet The packet data to add.
  /// @return The <tt>packet.packetSize()</tt> on success, or -1 on failure.
  int add(const PacketWriter &packet);

  /// Add bytes to the packet. Use with care as the @p buffer should always
  /// start with a valid @c PacketHeader in network byte order.
  /// @param buffer The data to add.
  /// @param byte_count The number of bytes in @p buffer.
  /// @return The <tt>packet.packetSize()</tt> on success, or -1 on failure.
  int add(const uint8_t *buffer, uint16_t byte_count);

  /// Finalises the collated packet for sending. This includes completing
  /// compression and calculating the CRC.
  /// @return True on successful finalisation, false when already finalised.
  bool finalise();

  /// Check if @c finalise() has been called.
  /// @return True if @c finalise() has been called.
  [[nodiscard]] bool isFinalised() const { return _finalised; }

  /// Access the internal buffer pointer.
  /// @param[out] byte_count Set to the number of used bytes in the collated buffer, including
  ///     the CRC when the packet has been finalised.
  /// @return The internal buffer pointer.
  [[nodiscard]] const uint8_t *buffer(unsigned &byte_count) const;

  /// Return the number of bytes that have been collated. This excludes the @c PacketHeader
  /// and @c CollatedPacketMessage, but will include the CRC once finalised.
  [[nodiscard]] unsigned collatedBytes() const;

  /// Return the number of bytes available in the collated packet. This considers @c collatedBytes()
  /// so far and the packet @c Overhead with respect to @c maxPacketSize().
  /// @return The number of byte which can be written to the packet before it is full.
  [[nodiscard]] unsigned availableBytes() const;

  //-------------------------------------------
  // Connection methods.
  //-------------------------------------------

  /// Ignored for @c CollatedPacket.
  void close() override;

  /// Enable/disable the connection. While disabled, messages are ignored.
  /// @param active The active state to set.
  void setActive(bool active) override;

  /// Check if currently active.
  /// @return True while active.
  [[nodiscard]] bool active() const override;

  /// Identifies the collated packet.
  /// @return Always "CollatedPacket".
  [[nodiscard]] const char *address() const override;

  /// Not supported.
  /// @return Zero.
  [[nodiscard]] uint16_t port() const override;

  /// Always connected.
  /// @return True.
  [[nodiscard]] bool isConnected() const override;

  /// Collated the create message for @p shape.
  /// @param shape The shape of interest.
  /// @return The number of bytes added, or -1 on failure (as per @c add()).
  int create(const Shape &shape) override;

  /// Collated the update message for @p shape.
  /// @param shape The shape of interest.
  /// @return The number of bytes added, or -1 on failure (as per @c add()).
  int destroy(const Shape &shape) override;

  /// Collated the destroy message for @p shape.
  /// @param shape The shape of interest.
  /// @return The number of bytes added, or -1 on failure (as per @c add()).
  int update(const Shape &shape) override;

  /// Not supported.
  /// @param byte_limit Ignored.
  /// @return -1.
  int updateTransfers(unsigned byte_limit) override;

  /// Not supported.
  /// @param dt Ignored.
  /// @param flush Ignored.
  int updateFrame(float dt, bool flush) override;

  /// Not supported.
  /// @return 0;
  unsigned referenceResource(const ResourcePtr &resource) override;

  /// Not supported.
  /// @return 0;
  unsigned releaseResource(const ResourcePtr &resource) override;

  /// Pack the given @p info into the packet.
  /// @param info Details on the server.
  /// @return True on success
  bool sendServerInfo(const ServerInfoMessage &info) override;

  /// Add data from @c packet.
  /// @param packet Data to add. Must be finalised.
  /// @param allow_collation Ignored.
  int send(const PacketWriter &packet, bool allow_collation) override;

  /// Aliased to @p add().
  /// @param buffer The data to add.
  /// @param byte_count The number of bytes in @p buffer.
  /// @param allow_collation Ignored in this context.
  /// @return The <tt>packet.packetSize()</tt> on success, or -1 on failure.
  int send(const uint8_t *data, int byte_count, bool allow_collation) override;

  /// Not supported
  /// @param collated N/A
  /// @return Throws @c Exception or returns -1 depending on the @c TES_EXCEPTIONS define.
  int send(const CollatedPacket &collated) override;

private:
  /// Initialise the buffer.
  /// @param compress Enable compression?
  /// @param buffer_size Initial buffer size.
  /// @param max_packet_size Maximum buffer size.
  void init(bool compress, unsigned buffer_size, unsigned max_packet_size);

  /// Expand the internal buffer size by @p expand_by bytes up to @c maxPacketSize().
  /// @param expand_by Minimum number of bytes to expand by.
  static void expand(unsigned expand_by, std::vector<uint8_t> &buffer, unsigned current_data_count,
                     unsigned max_packet_size);

  std::unique_ptr<CollatedPacketZip> _zip;  ///< Present and used when compression is enabled.
  std::vector<uint8_t> _buffer;             ///< Internal buffer.
  /// Buffer used to finalise collation. Deflating may not be successful, so we can try and fail
  /// with this buffer.
  std::vector<uint8_t> _final_buffer;
  // unsigned _buffer_size = 0;                ///< current size of @c _buffer.
  // unsigned _final_buffer_size = 0;          ///< current size of @c _final_buffer.
  unsigned _final_packet_cursor = 0;        ///< End of data in @c _final_buffer
  unsigned _cursor = 0;                     ///< Current write position in @c _buffer.
  unsigned _max_packet_size = 0;            ///< Maximum @p _buffer_size.
  uint16_t _compression_level = ClDefault;  ///< @c CompressionLevel
  bool _finalised = false;                  ///< Finalisation flag.
  bool _active = true;                      ///< For @c Connection::active().
};


inline bool CollatedPacket::compressionEnabled() const
{
  return _zip != nullptr;
}


inline unsigned CollatedPacket::maxPacketSize() const
{
  return _max_packet_size;
}


inline unsigned CollatedPacket::collatedBytes() const
{
  return _cursor;
}

inline unsigned CollatedPacket::availableBytes() const
{
  const unsigned used = collatedBytes() + static_cast<unsigned>(Overhead);
  return (_max_packet_size >= used) ? _max_packet_size - used : 0;
}
}  // namespace tes

#endif  // TES_CORE_COLLATED_PACKET_H
