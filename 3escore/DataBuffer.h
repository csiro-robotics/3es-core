//
// author: Kazys Stepanas
//
#ifndef TES_CORE_DATA_BUFFER_H
#define TES_CORE_DATA_BUFFER_H

#include "CoreConfig.h"

#include "Colour.h"
#include "CoreUtil.h"
#include "Debug.h"
#include "Messages.h"
#include "PacketReader.h"
#include "PacketWriter.h"
#include "Vector3.h"

#include <algorithm>
#include <array>
#include <cinttypes>
#include <memory>
#include <utility>
#include <vector>

#define STREAM_TYPE_INFO(_type, _type_name) \
  template <>                               \
  class DataBufferTypeInfo<_type>           \
  {                                         \
  public:                                   \
    static DataStreamType type()            \
    {                                       \
      return _type_name;                    \
    }                                       \
    static size_t size()                    \
    {                                       \
      return sizeof(_type);                 \
    }                                       \
  }


namespace tes
{
class PacketWriter;

/// Type traits providing information for type @c T within a @c DataBuffer context.
template <typename T>
class DataBufferTypeInfo
{
public:
  /// Query the @c DataStreamType corresponding to @c T .
  /// @return The corresponding data type.
  static DataStreamType type() { return DctNone; }
  /// Query the byte size of @c T .
  /// @return The byte size of @c T .
  static size_t size() { return 0; }
};

STREAM_TYPE_INFO(int8_t, DctInt8);
STREAM_TYPE_INFO(uint8_t, DctUInt8);
STREAM_TYPE_INFO(int16_t, DctInt16);
STREAM_TYPE_INFO(uint16_t, DctUInt16);
STREAM_TYPE_INFO(int32_t, DctInt32);
STREAM_TYPE_INFO(uint32_t, DctUInt32);
STREAM_TYPE_INFO(int64_t, DctInt64);
STREAM_TYPE_INFO(uint64_t, DctUInt64);
STREAM_TYPE_INFO(float, DctFloat32);
STREAM_TYPE_INFO(double, DctFloat64);

class DataBuffer;

// Afordances:
// - Take ownership of a copy of the steam
// - Write to PacketStream as either the same format, or the special cases below:
//    - DctFloat32 can be written as:
//      - DctPackedFloat16
//      - DctPackedFloat32
//      - DctFloat64
//    - DctFloat64 can be written as:
//      - DctPackedFloat16
//      - DctPackedFloat32
//      - DctFloat16
// - Read from PacketStream
// - Delete

namespace detail
{
class TES_CORE_API DataBufferAffordances
{
public:
  virtual ~DataBufferAffordances();

  virtual void release(const void **stream_ptr, bool has_ownership) const = 0;
  virtual void takeOwnership(const void **stream_ptr, bool has_ownership, const DataBuffer &stream) const = 0;
  virtual uint32_t write(PacketWriter &packet, uint32_t offset, DataStreamType write_as_type, unsigned byteLimit,
                         uint32_t receiveOffset, const DataBuffer &stream, double quantisation_unit = 0.0) const = 0;
  virtual uint32_t read(PacketReader &packet, void **stream_ptr, unsigned *stream_size, bool *has_ownership,
                        const DataBuffer &stream) const = 0;
  virtual uint32_t read(PacketReader &packet, void **stream_ptr, unsigned *stream_size, bool *has_ownership,
                        const DataBuffer &stream, unsigned offset, unsigned count) const = 0;
  virtual size_t get(DataStreamType as_type, size_t element_index, size_t component_index, size_t component_read_count,
                     const void *stream, size_t stream_element_count, size_t stream_component_count,
                     size_t stream_element_stride, void *dst, size_t dst_capacity) const = 0;
};

template <typename T>
class TES_CORE_API DataBufferAffordancesT : public DataBufferAffordances
{
public:
  static DataBufferAffordances *instance();

  void release(const void **stream_ptr, bool has_ownership) const override;
  void takeOwnership(const void **stream_ptr, bool has_ownership, const DataBuffer &stream) const override;
  uint32_t write(PacketWriter &packet, uint32_t offset, DataStreamType write_as_type, unsigned byteLimit,
                 uint32_t receiveOffset, const DataBuffer &stream, double quantisation_unit = 0.0) const override;
  uint32_t read(PacketReader &packet, void **stream_ptr, unsigned *stream_size, bool *has_ownership,
                const DataBuffer &stream) const override;
  uint32_t read(PacketReader &packet, void **stream_ptr, unsigned *stream_size, bool *has_ownership,
                const DataBuffer &stream, unsigned offset, unsigned count) const;

  size_t get(DataStreamType as_type, size_t element_index, size_t component_index, size_t component_read_count,
             const void *stream, size_t stream_element_count, size_t stream_component_count,
             size_t stream_element_stride, void *dst, size_t dst_capacity) const override;

  template <typename WriteType>
  uint32_t writeAs(PacketWriter &packet, uint32_t offset, DataStreamType write_as_type, unsigned byteLimit,
                   uint32_t receiveOffset, const DataBuffer &stream) const;

  template <typename FloatType, typename PackedType>
  uint32_t writeAsPacked(PacketWriter &packet, uint32_t offset, DataStreamType write_as_type, unsigned byteLimit,
                         uint32_t receiveOffset, const FloatType *packingOrigin, const FloatType quantisationUnit,
                         const DataBuffer &stream) const;


  template <typename ReadType>
  uint32_t readAs(PacketReader &packet, uint32_t offset, uint32_t count, unsigned componentCount,
                  void **stream_ptr) const;

  template <typename FloatType, typename ReadType>
  uint32_t readAsPacked(PacketReader &packet, unsigned offset, unsigned count, unsigned componentCount,
                        void **stream_ptr) const;
};

extern template class TES_CORE_API DataBufferAffordancesT<int8_t>;
extern template class TES_CORE_API DataBufferAffordancesT<uint8_t>;
extern template class TES_CORE_API DataBufferAffordancesT<int16_t>;
extern template class TES_CORE_API DataBufferAffordancesT<uint16_t>;
extern template class TES_CORE_API DataBufferAffordancesT<int32_t>;
extern template class TES_CORE_API DataBufferAffordancesT<uint32_t>;
extern template class TES_CORE_API DataBufferAffordancesT<int64_t>;
extern template class TES_CORE_API DataBufferAffordancesT<uint64_t>;
extern template class TES_CORE_API DataBufferAffordancesT<float>;
extern template class TES_CORE_API DataBufferAffordancesT<double>;
}  // namespace detail

/// A helper class for wrapping various input array types into data streams for data transfer.
///
/// A @c DataBuffer is intended to hold a borrowed pointer for use with mesh data. The stream may represent vertex
/// or index data of various data types and sizes, but is expected to be of a particular type on transfer. For example
/// a @c DataBuffer may wrap a @c double array representing a @c Vector3 vertex stream. On transfer, the data may be
/// transfered using single precision, or quantised precision.
///
/// There are several key concepts to understanding how the @c DataBuffer interprets and stores information. Firstly
/// the assumptions are that the source array stores @em vertices which can be represented by a simple @em dataType:
/// @c intX_t , @c uintX_t , @c float or @c double . The array is broken up into @em vertices where each @em vertex is
/// composed of @em componentCount consecutive @em dataElements of the simple data type. A vertex be followed by some
/// padding - possibly for data alignment - of M @em dataElements. Finally, there number of @em vertices is known.
///
/// The terminology is broken down below:
/// - @em dataType - the simple data type containedin the stream.
/// - @em dataElements - a number of consecutive @em dataType elements
/// - @em componentCount - the number of @em dataType elements in each @em vertex
/// - @em vertexStride - the number of @em dataType elements between each vertex. This will be at least as large as
///   @em componentCount
///
/// Some examples are provided below to help illustrate the terminology:
///
/// Logical Type      | @em dataType  | @em componentCount  | @em vertexStride  |
/// ----------------- | ------------- | ------------------- | ----------------- |
/// 32-bit indices    | uint32_t      | 1                   | 1                 |
/// float3 (packed)   | float         | 3                   | 3                 |
/// float3 (aligned)  | float         | 3                   | 4                 |
///
/// Note <tt>float3 (aligned)</tt> assumes 16-byte data alignment, which is often optimal for single precision vertex
/// storage. In constrast <tt>float3 (packed)</tt> assumes densely packed float tripplets such as the @c tes::Vector3f
/// definition.
///
/// The byte size of each element is calculated as <tt>sizeof(dataType) * componentCount</tt>.
///
/// The byte size of the entire array is calculated as <tt>count() * sizeof(dataType) * componentCount</tt>.
///
/// @c componentCount() values above 16 are not supported.
class TES_CORE_API DataBuffer
{
public:
  DataBuffer();

  template <typename T>
  DataBuffer(const T *v, size_t count, size_t componentCount = 1, size_t componentStride = 0, bool ownPointer = false);

  DataBuffer(const Vector3f *v, size_t count);

  template <size_t Count>
  inline DataBuffer(const std::array<Vector3f, Count> &v)
    : DataBuffer(v.data(), Count)
  {}

  DataBuffer(const Vector3d *v, size_t count);

  template <size_t Count>
  inline DataBuffer(const std::array<Vector3d, Count> &v)
    : DataBuffer(v.data(), Count)
  {}

  DataBuffer(const Colour *c, size_t count);

  template <size_t Count>
  inline DataBuffer(const std::array<Colour, Count> &c)
    : DataBuffer(c.data(), Count)
  {}

  template <typename T>
  DataBuffer(const std::vector<T> &v, size_t componentCount = 1, size_t componentStride = 0);

  template <typename T, size_t Count>
  inline DataBuffer(const std::array<T, Count> &v)
    : DataBuffer(v.data(), Count)
  {}

  DataBuffer(const std::vector<Vector3f> &v);

  DataBuffer(const std::vector<Vector3d> &v);

  DataBuffer(const std::vector<Colour> &v);

  DataBuffer(DataBuffer &&other);

  DataBuffer(const DataBuffer &other);

  ~DataBuffer();

  void reset();

  template <typename T>
  void set(const T *v, size_t count, size_t componentCount = 1, size_t componentStride = 0, bool ownPointer = false);

  template <typename T>
  void set(const std::vector<T> &v, size_t componentCount = 1, size_t componentStride = 0);

  void set(const std::vector<Vector3f> &v);
  void set(const std::vector<Vector3d> &v);
  void set(const std::vector<Colour> &v);

  /// Read a single item at the given element index, and component index.
  ///
  /// The element index accounts for element striding, while the component index allows reading intermediate values.
  /// For example, consider a @c DataBuffer creates from 10 @c Vector3f elements. This creates a @c float
  /// @c DataBuffer with an element count of 10, an element stride of 3 and a component count of 3 (for the XYZ
  /// channels of the @c Vector3f . The @p element_index is used to address each @c Vector3f while the
  /// @c component_count is used in the range [0, 2] to extract the XYZ values.
  ///
  /// @note This only supports reading basic types, meaning that template types of @c Vector3&lt;T&gt; and @c Colour
  /// are not supported. Use @c float or @c double for @c Vector3&lt;T&gt; buffers and @c uint32_t for @c Colour .
  template <typename T>
  T get(size_t element_index, size_t component_index = 0) const;

  /// Read a block of data from the buffer. This reads from the @p element_index reading @c element_count data items
  /// into @p dst .
  ///
  /// There are some caveates to consider here because a buffer may have a @c componentCount() greater than 1 (such as
  /// for @c Vector3&lt;T&gt; buffers). The effect is that for a successful read operation the @p capacity at @p dst
  /// must be at least @p element_count * @c componentCount() . This essentially makes the 'units' of @c capacity
  /// will different from @p element_index , @p element_count and the return value when the @c DataBuffer has a
  /// @c componentCount() greater than 1. The items are read in a tightly packed fasion into @p dst .
  ///
  /// @note This only supports reading basic types, meaning that template types of @c Vector3&lt;T&gt; and @c Colour
  /// are not supported. Use @c float or @c double for @c Vector3&lt;T&gt; buffers and @c uint32_t for @c Colour .
  ///
  /// @param element_index The index of the element in the buffer to start reading from.
  /// @param element_count The number of elements to read.
  /// @param dst The address to read into.
  /// @param capacity The data capacity of @c dst , as the number of @c T elements it can hold.
  /// @tparam T Data type to read as. Must be a basic type as supported by @c DataStreamType (see note above).
  /// @return The number of @c DataBuffer @em elements read. The number of @c T elements written to @p dst will be
  ///   this value times the @c componentCount() .
  template <typename T>
  size_t get(size_t element_index, size_t element_count, T *dst, size_t capacity) const;

  // template <typename T>
  // bool get(T *dst, size_t element_index, size_t component_index = 0) const;

  DataBuffer &operator=(DataBuffer other);

  inline bool isValid() const { return _stream != nullptr; }

  inline unsigned count() const { return _count; }
  inline unsigned addressableCount() const { return _count * _componentCount; }
  inline unsigned basicTypeSize() const { return _basicTypeSize; }
  inline unsigned byteStride() const { return _elementStride * _basicTypeSize; }
  inline unsigned componentCount() const { return _componentCount; }
  inline unsigned elementStride() const { return _elementStride; }

  inline bool ownPointer() const { return int(_flags & Flag::OwnPointer) != 0; }
  inline bool writable() const { return int(_flags & Flag::Writable) != 0; }
  inline DataStreamType type() const { return _type; }

  void swap(DataBuffer &other);

  template <typename T>
  const T *ptr(size_t element_index = 0) const;

  template <typename T>
  const T *ptrAt(size_t element_index) const;

  /// Copy the internal array and take ownership. Does nothing if this object already owns its own array memory.
  void duplicate();

  static uint16_t estimateTransferCount(size_t elementSize, unsigned overhead, unsigned byteLimit);

  // receiveOffset: offset packed into the message for the receiver to handle. Allows a small vertex buffer to represent
  // a slice of a buffer at the other end.
  unsigned write(PacketWriter &packet, uint32_t offset, unsigned byteLimit = 0, uint32_t receiveOffset = 0) const;
  unsigned writePacked(PacketWriter &packet, uint32_t offset, double quantisation_unit, unsigned byteLimit = 0,
                       uint32_t receiveOffset = 0) const;

  /// Read : reading offset and count from the @p packet
  unsigned read(PacketReader &packet);

  /// Read : skipping offset and count, with @p count given.
  unsigned read(PacketReader &packet, unsigned offset, unsigned count);

  friend inline void swap(DataBuffer &a, DataBuffer &b) { a.swap(b); }

private:
  void *writePtr() { return (ownPointer()) ? const_cast<void *>(_stream) : nullptr; }

  /// Flag values for @c _flags
  enum Flag : uint8_t
  {
    Zero = 0,               ///< Zero value
    OwnPointer = (1 << 0),  ///< Indicates this object owns the heap allocation for @c _stream
    Writable = (1 << 1),    ///< Is writing to @c _stream allowed?
  };

  const void *_stream{ nullptr };
  unsigned _count{ 0 };          ///< Number of vertices in the @p _stream .
  uint8_t _componentCount{ 1 };  ///< Number of data type component elements in each vertex. E.g., Vector3 has 3.
  /// Number of data type elements between each vertex. For any densely packed array this value will match
  /// @c _componentCount . For alined, or interleaved arrays, this valid will be larger than @c _componentCount .
  ///
  /// For example, an array of 16 byte aligned @c float3 vertices will have a @c _componentCount of 3 and a
  /// @c _elementStride of 4.
  uint8_t _elementStride{ 1 };
  /// Size of the basic @c _type stored in @c stream .
  uint8_t _basicTypeSize{ 0 };
  DataStreamType _type{ DctNone };  ///< The simple data type for @c _stream
  uint8_t _flags{ Flag::Zero };     ///< Does this class own the @c _stream pointer?
  /// Pointer to the implementation for various operations supported on a @c DataBuffer . This is using a type
  /// erasure setup.
  detail::DataBufferAffordances *_affordances{ nullptr };
};

/// Write a @c DataBuffer to @p packet as a stream of @c DstType . Note this function requires knowing the concrete
/// type of the @c DataBuffer data for proper casting. It is assumed that simple casting from @c SrcType t
/// @c DstType in assignment is valid.
///
/// This function is not recommended for converting from floating point to integer streams or vise versa.
///
/// This function should be used after writing @c PackerHeader and message header information to @c packet .
///
/// The data written are:
/// - `uint32_t offset` : the @p offset argument
/// - `uint16_t count` : number of items written to the packet
/// - `uint8_t componentCount` : @p stream.componentCount()
/// - `uint8_t dataType` : @p stream.type()
/// - `DstType[] array` : `count * componentCount` elements of type `DstType`
///
/// @param packet The data packet to write to.
/// @param stream The stream to write data from.
/// @param offset The element offset into @p stream to start writing from. This applies @c stream.componentCount()
/// @return The number of items added to the @p packet or zero on any failure (@p packet becomes invalid).
template <typename DstType, typename SrcType>
unsigned writeStream(PacketWriter &packet, const DataBuffer &stream, uint32_t offset);

/// Write a @c DataBuffer to @p packet as qunatised, and packed data stream. This is intended only for input streams
/// containing @c float or @c double data such as vertex positions or normals.
///
/// For each vertex, we subtract the @p packingOrigin (if given), divide by the @p quantisationUnit then cast to the
/// @c PacketType . For example, given a @c Vector3f input stream to be written as @c DctPackedFloat16 :
/// - @p packingOrigin is a @c Vector3f used as a reference coordinate frame for each vertex.
/// - @p quantisationUnit is the smallest representable unit: for example if the stream is in metres, packing at
///   millimetre prosicion requires a @p quantisationUnit of `0.001`.
///
/// This function fails if the combination of target precision, @p packingOrigin and @p quantisationUnit is insufficient
/// to represent any item in the stream.
///
/// The data written are:
/// - `uint32_t offset` : the @p offset argument
/// - `uint16_t count` : number of items written to the packet
/// - `uint8_t componentCount` : @p stream.componentCount()
/// - `uint8_t dataType` : @p stream.type()
/// - `FloatType[stream.componentCount()] packingOrigin` : from @p packingOrigin or `(0, 0, 0)` if null.
/// - `float32 quantisationUnit` : from @p quantisationUnit .
/// - `DstType[] array` : `count * componentCount` elements of type `DstType`
///
/// @param packet The data packet to write to.
/// @param stream The stream to write data from.
/// @param offset The element offset into @p stream to start writing from. This applies @c stream.componentCount()
/// @param packingOrigin Reference origin for each item in the stream or @c nullptr to use `(0, 0, 0)`
/// @param quantisationUnit Quantisation scaling unit.
/// @param packedType The target, packed type either @c DctPackedFloat16 or @c DctPackedFloat32 .
/// @return The number of items added to the @p packet or zero on any failure (@p packet becomes invalid).
template <typename FloatType, typename PackedType, typename SrcType>
unsigned writeStreamPackedFloat(PacketWriter &packet, const DataBuffer &stream, uint32_t offset,
                                const FloatType *packingOrigin, float quantisationUnit, DataStreamType packedType);

unsigned readStream(PacketReader &packet, DataBuffer &stream);

}  // namespace tes

#include "DataBuffer.inl"

#endif  // TES_CORE_DATA_BUFFER_H
