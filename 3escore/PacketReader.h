//
// author: Kazys Stepanas
//
#ifndef TES_CORE_PACKET_READER_H
#define TES_CORE_PACKET_READER_H

#include "CoreConfig.h"

#include "PacketStream.h"

#include <vector>

namespace tes
{
/// A utility class for dealing with reading packets.
///
/// @bug Use the payloadOffset in various calculations herein. It was added after this
/// class was written, but is currently only supported as being zero, so it's not an issue
/// yet.
class TES_CORE_API PacketReader : public PacketStream<const PacketHeader>
{
public:
  /// Creates a new packet reader for the given packet and its CRC.
  PacketReader(const PacketHeader *packet);

  /// Move constructor.
  /// @param other Packet to move
  PacketReader(PacketReader &&other) noexcept;

  PacketReader &operator=(PacketReader &&other) noexcept;

  void swap(PacketReader &other) noexcept;

  friend void swap(PacketReader &first, PacketReader &second) { first.swap(second); }

  /// Calculates the CRC value, returning true if it matches. This also sets
  /// @c isCrcValid() on success.
  ///
  /// Returns true immediately when @c isCrcValid() is already set.
  /// @return True if the CRC is valid.
  bool checkCrc();

  /// Caluclates the CRC for the packet.
  [[nodiscard]] CrcType calculateCrc() const;

  /// Returns the number of bytes available for writing in the payload.
  /// @return The number of bytes available for writing.
  [[nodiscard]] uint16_t bytesAvailable() const;

  /// Reads a single data element from the current position. This assumes that
  /// a single data element of size @p element_size is being read and may require
  /// an endian swap to the current platform endian.
  ///
  /// The reader position is advanced by @p element_size. Does not set the
  /// @c Fail bit on failure.
  ///
  /// @param bytes Location to read into.
  /// @param element_size Size of the data item being read at @p bytes.
  /// @return @p element_size on success, 0 otherwise.
  size_t readElement(uint8_t *bytes, size_t element_size);

  /// Reads an array of data items from the current position. This makes the
  /// same assumptions as @c readElement() and performs an endian swap per
  /// array element. Elements in the array are assumed to be contiguous in
  /// both source and destination locations.
  ///
  /// Up to @p element_count elements will be read depending on availability.
  /// Less may be read, but on success the number of bytes read will be
  /// a multiple of @p element_size.
  ///
  /// The reader position is advanced by the number of bytes read.
  /// Does not set the @c Fail bit on failure.
  ///
  /// @param bytes Location to read into.
  /// @param element_size Size of a single array element to read.
  /// @param element_count The number of elements to attempt to read.
  /// @return On success returns the number of whole elements read.
  size_t readArray(uint8_t *bytes, size_t element_size, size_t element_count);

  /// Reads raw bytes from the packet at the current position up to @p byte_count.
  /// No endian swap is performed on the data read.
  ///
  /// The reader position is advanced by @p byte_count.
  /// Does not set the @c Fail bit on failure.
  ///
  /// @param bytes Location to read into.
  /// @aparam byte_count Number of bytes to read.
  /// @return The number of bytes read. This may be less than @p byte_count if there
  ///   are insufficient data available.
  size_t readRaw(uint8_t *bytes, size_t byte_count);

  /// Peek @p byte_count bytes from the current position in the buffer. This does not affect the
  /// stream position.
  /// @param dst The memory to write to.
  /// @param byte_count Number of bytes to read.
  /// @param allow_byte_swap @c true to allow the byte ordering to be modified in @p dst. Only
  /// performed when
  ///   the network endian does not match the platform endian.
  /// @return The number of bytes read. Must match @p byte_count for success.
  size_t peek(uint8_t *dst, size_t byte_count, bool allow_byte_swap = true);

  /// Reads a single data item from the packet. This reads a number of bytes
  /// equal to @c sizeof(T) performing an endian swap if necessary.
  /// @param[out] element Set to the data read.
  /// @return @c sizeof(T) on success, zero on failure.
  template <typename T>
  size_t readElement(T &element);

  template <typename T>
  size_t readArray(T *elements, size_t element_count);

  template <typename T>
  size_t readArray(std::vector<T> &elements);

  template <typename T, int N>
  size_t readArray(std::array<T, N> &elements);

  template <typename T>
  PacketReader &operator>>(T &val);
};

inline uint16_t PacketReader::bytesAvailable() const
{
  return static_cast<uint16_t>(payloadSize() - _payload_position);
}

template <typename T>
inline size_t PacketReader::readElement(T &element)
{
  return readElement(reinterpret_cast<uint8_t *>(&element), sizeof(T));
}

template <typename T>
inline size_t PacketReader::readArray(T *elements, size_t element_count)
{
  return readArray(reinterpret_cast<uint8_t *>(elements), sizeof(T), element_count);
}


template <typename T>
inline size_t PacketReader::readArray(std::vector<T> &elements)
{
  return readArray(elements.data(), elements.size());
}

template <typename T, int N>
inline size_t PacketReader::readArray(std::array<T, N> &elements)
{
  return readArray(elements.data(), elements.size());
}

template <typename T>
inline PacketReader &PacketReader::operator>>(T &val)
{
  int read = readElement(val);
  _status |= !(read == sizeof(T)) * Fail;
  return *this;
}
}  // namespace tes

#endif  // TES_CORE_PACKET_READER_H
