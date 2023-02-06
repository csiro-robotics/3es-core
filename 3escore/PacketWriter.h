//
// author: Kazys Stepanas
//
#ifndef TES_CORE_PACKET_WRITER_H
#define TES_CORE_PACKET_WRITER_H

#include "CoreConfig.h"

#include "PacketStream.h"

namespace tes
{
/// A utility class for writing payload data to a @c PacketHeader.
///
/// This keeps the @c PacketHeader @c payloadSize member up to date and
/// ensures the CRC is calculated, either via @c calculateCrc() explicitly
/// on on destruction.
///
/// The payload buffer size must be specified in the constructor, and data writes
/// are limited by this value. The packet is assumed to be structured such that
/// the packet header is located at the start of the buffer, followed immediately by
/// space for the payload.
///
/// Two construction options are available, one where the @c PacketHeader details are already
/// initialised, except for the @c payloadSize and @c crc. The given packet is
/// assumed to be the start of the data buffer. The second constructor accepts a raw
/// byte pointer, which marks the start of the buffer, and the size of the buffer.
/// The buffer size must be large enough for the @ PacketHeader. Remaining space is available
/// for the payload.
///
/// @bug Use the payloadOffset in various calculations herein. It was added after this
/// class was written, but is currently only supported as being zero, so it's not an issue
/// yet.
class TES_CORE_API PacketWriter : public PacketStream<PacketHeader>
{
public:
  // TODO(KS): support constructor from std::array.

  /// Creates a @c PacketWriter to write to the given @p packet. This
  /// marks the start of the packet buffer.
  ///
  /// The packet members are initialised, but @c payloadSize and @c crc are
  /// left at zero to be calculated later. The @c routing_id maybe given now
  /// or set with @c setRoutingId().
  ///
  /// @param packet The packet to write to.
  /// @param max_payload_size Specifies the space available for the payload (bytes).
  ///   This is in excess of the packet size, not the total buffer size.
  PacketWriter(PacketHeader *packet, uint16_t max_payload_size, uint16_t routing_id = 0,
               uint16_t message_id = 0);

  /// Creates a @c PacketWriter to write to the given byte buffer.
  ///
  /// The buffer size must be at least @c sizeof(PacketHeader), larger if any
  /// payload is required. If not, then the @c isFail() will be true and
  /// all write operations will fail.
  ///
  /// The @c routing_id maybe given now or set with @c setRoutingId().
  ///
  /// @param buffer The packet data buffer.
  /// @param buffer_size The total number of bytes available for the @c PacketHeader
  ///   and its paylaod. Must be at least @c sizeof(PacketHeader), or all writing
  ///   will fail.
  /// @param routing_id Optionlly sets the @c routing_id member of the packet.
  PacketWriter(uint8_t *buffer, uint16_t buffer_size, uint16_t routing_id = 0,
               uint16_t message_id = 0);

  /// Copy constructor. Simple as neither writer owns the underlying memory.
  /// Both point to the same underlying memory, but only one should be used.
  /// @param other The packet to copy.
  PacketWriter(const PacketWriter &other);

  /// Move constructor.
  /// @param other The packet to move.
  PacketWriter(PacketWriter &&other) noexcept;

  /// Destructor, ensuring the CRC is calculated.
  ~PacketWriter();

  /// Assignment operator. Simple as neither writer owns the underlying memory.
  /// Both point to the same underlying memory, but only one should be used.
  /// @param other The packet to copy.
  PacketWriter &operator=(PacketWriter other);

  void swap(PacketWriter &other);

  friend inline void swap(PacketWriter &first, PacketWriter &second) { first.swap(second); }

  /// Resets the packet, clearing out all variable data including the payload, crc and routing id.
  /// Allows preparation for writing new data to the same payload buffer.
  ///
  /// @param routing_id Optional specification for the @c routing_id after reset.
  void reset(uint16_t routing_id, uint16_t message_id);

  /// @overload
  inline void reset() { reset(0, 0); }

  void setRoutingId(uint16_t routing_id);
  [[nodiscard]] PacketHeader &packet() const;

  [[nodiscard]] const uint8_t *data() const;

  [[nodiscard]] uint8_t *payload();

  inline void invalidateCrc() { _status = static_cast<uint16_t>(_status & ~CrcValid); }

  /// Returns the number of bytes remaining available in the payload.
  /// This is calculated as the @c maxPayloadSize() - @c payloadSize().
  /// @return Number of bytes remaining available for write.
  [[nodiscard]] uint16_t bytesRemaining() const;

  /// Returns the size of the payload buffer. This is the maximum number of bytes
  /// which can be written to the payload.
  /// @return The payload buffer size (bytes).
  [[nodiscard]] uint16_t maxPayloadSize() const;

  /// Finalises the packet for sending, calculating the CRC.
  /// @return True if the packet is valid and ready for sending.
  bool finalise();

  /// Calculates the CRC and writes it to the @c PacketHeader crc member.
  ///
  /// The current CRC value is returned when @c isCrcValid() is true.
  /// The CRC will not be calculate when @c isFail() is true and the
  /// result is undefined.
  /// @return The Calculated CRC, or undifined when @c isFail().
  CrcType calculateCrc();

  /// Writes a single data element from the current position. This assumes that
  /// a single data element of size @p element_size is being write and may require
  /// an endian swap to the current platform endian.
  ///
  /// The writer position is advanced by @p element_size. Does not set the
  /// @c Fail bit on failure.
  ///
  /// @param bytes Location to write from.
  /// @param element_size Size of the data item being write at @p bytes.
  /// @return @p element_size on success, 0 otherwise.
  size_t writeElement(const uint8_t *bytes, size_t element_size);

  /// Writes an array of data items from the current position. This makes the
  /// same assumptions as @c writeElement() and performs an endian swap per
  /// array element. Elements in the array are assumed to be contiguous in
  /// both source and destination locations.
  ///
  /// The writer position is advanced by the number of bytes write.
  /// Does not set the @c Fail bit on failure.
  ///
  /// @param bytes Location to write from.
  /// @param element_size Size of a single array element to write.
  /// @param element_count The number of elements to attempt to write.
  /// @return On success returns the number of elements written, not bytes.
  size_t writeArray(const uint8_t *bytes, size_t element_size, size_t element_count);

  /// Writes raw bytes from the packet at the current position up to @p byte_count.
  /// No endian swap is performed on the data write.
  ///
  /// The writer position is advanced by @p byte_count.
  /// Does not set the @c Fail bit on failure.
  ///
  /// @param bytes Location to write into.
  /// @aparam byte_count Number of bytes to write.
  /// @return The number of bytes write. This may be less than @p byte_count if there
  ///   are insufficient data available.
  size_t writeRaw(const uint8_t *bytes, size_t byte_count);

  /// Writes a single data item from the packet. This writes a number of bytes
  /// equal to @c sizeof(T) performing an endian swap if necessary.
  /// @param[out] element Set to the data write.
  /// @return @c sizeof(T) on success, zero on failure.
  template <typename T>
  size_t writeElement(const T &element);

  template <typename T>
  size_t writeArray(const T *elements, size_t element_count);

  template <typename T>
  PacketWriter &operator>>(T &val);

protected:
  uint8_t *payloadWritePtr();
  void incrementPayloadSize(size_t inc);

  uint16_t _buffer_size = 0;
};

inline void PacketWriter::setRoutingId(uint16_t routing_id)
{
  _packet->routing_id = routing_id;
}

inline PacketHeader &PacketWriter::packet() const
{
  return *_packet;
}

inline const uint8_t *PacketWriter::data() const
{
  return reinterpret_cast<uint8_t *>(_packet);
}

inline uint8_t *PacketWriter::payload()
{
  return reinterpret_cast<uint8_t *>(_packet) + sizeof(PacketHeader);
}

template <typename T>
inline size_t PacketWriter::writeElement(const T &element)
{
  return writeElement(reinterpret_cast<const uint8_t *>(&element), sizeof(T));
}

template <typename T>
inline size_t PacketWriter::writeArray(const T *elements, size_t element_count)
{
  return writeArray(reinterpret_cast<const uint8_t *>(elements), sizeof(T), element_count);
}


template <typename T>
inline PacketWriter &PacketWriter::operator>>(T &val)
{
  int written = writeElement(val);
  _status |= !(written == sizeof(T)) * Fail;
  return *this;
}


inline uint8_t *PacketWriter::payloadWritePtr()
{
  return payload() + _payload_position;
}
}  // namespace tes

#endif  // TES_CORE_PACKET_WRITER_H
