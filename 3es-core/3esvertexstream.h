//
// author: Kazys Stepanas
//
#ifndef _3ESVERTEXSTREAM_H
#define _3ESVERTEXSTREAM_H

#include "3es-core.h"

#include "3escoreutil.h"
#include "3esdebug.h"
#include "3esmessages.h"
#include "3espacketreader.h"
#include "3espacketwriter.h"
#include "3esvector3.h"

#include <cinttypes>
#include <memory>
#include <utility>
#include <vector>

#define STREAM_TYPE_INFO(_type, _type_name)             \
  template <>                                           \
  class VertexStreamTypeInfo<_type>                     \
  {                                                     \
  public:                                               \
    static DataStreamType type() { return _type_name; } \
    static size_t size() { return sizeof(_type); }      \
  }


namespace tes
{
class PacketWriter;

template <typename T>
class VertexStreamTypeInfo
{
public:
  static DataStreamType type() { return DctNone; }
  static size_t size() { return 0; }
};

STREAM_TYPE_INFO(int8_t, DctInt8);
STREAM_TYPE_INFO(uint8_t, DctUInt8);
STREAM_TYPE_INFO(int16_t, DctInt16);
STREAM_TYPE_INFO(uint16_t, DctUInt16);
STREAM_TYPE_INFO(int32_t, DctInt32);
STREAM_TYPE_INFO(uint32_t, DctUInt32);
STREAM_TYPE_INFO(float, DctFloat32);
STREAM_TYPE_INFO(double, DctFloat64);

class VertexStream;

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
class _3es_coreAPI VertexStreamAffordances
{
public:
  virtual ~VertexStreamAffordances();

  virtual void release(const void **stream_ptr, bool has_ownership) const = 0;
  virtual void takeOwnership(const void **stream_ptr, bool has_ownership, const VertexStream &stream) const = 0;
  virtual uint32_t write(PacketWriter &packet, uint32_t offset, DataStreamType write_as_type,
                         const VertexStream &stream, float quantisation_unit = 0.0) const = 0;
  virtual uint32_t read(PacketReader &packet, void **stream_ptr, bool *has_ownership,
                        const VertexStream &stream) const = 0;
  virtual uint32_t read(PacketReader &packet, void **stream_ptr, bool *has_ownership, const VertexStream &stream,
                        unsigned offset, unsigned count) const = 0;
};

template <typename T>
class _3es_coreAPI VertexStreamAffordancesT : public VertexStreamAffordances
{
public:
  static VertexStreamAffordances *instance();

  void release(const void **stream_ptr, bool has_ownership) const override;
  void takeOwnership(const void **stream_ptr, bool has_ownership, const VertexStream &stream) const override;
  uint32_t write(PacketWriter &packet, uint32_t offset, DataStreamType write_as_type, const VertexStream &stream,
                 float quantisation_unit = 0.0) const override;
  uint32_t read(PacketReader &packet, void **stream_ptr, bool *has_ownership,
                const VertexStream &stream) const override;
  uint32_t read(PacketReader &packet, void **stream_ptr, bool *has_ownership, const VertexStream &stream,
                unsigned offset, unsigned count) const;

  template <typename WriteType>
  uint32_t writeAs(PacketWriter &packet, uint32_t offset, DataStreamType write_as_type,
                   const VertexStream &stream) const;

  template <typename FloatType, typename PackedType>
  uint32_t writeAsPacked(PacketWriter &packet, uint32_t offset, DataStreamType write_as_type,
                         const FloatType *packingOrigin, const float quantisationUnit,
                         const VertexStream &stream) const;


  template <typename ReadType>
  uint32_t readAs(PacketReader &packet, uint32_t offset, uint32_t count, unsigned componentCount,
                  void **stream_ptr) const;

  template <typename FloatType, typename ReadType>
  uint32_t readAsPacked(PacketReader &packet, unsigned offset, unsigned count, unsigned componentCount,
                        void **stream_ptr) const;
};

extern template class _3es_coreAPI VertexStreamAffordancesT<int8_t>;
extern template class _3es_coreAPI VertexStreamAffordancesT<uint8_t>;
extern template class _3es_coreAPI VertexStreamAffordancesT<int16_t>;
extern template class _3es_coreAPI VertexStreamAffordancesT<uint16_t>;
extern template class _3es_coreAPI VertexStreamAffordancesT<int32_t>;
extern template class _3es_coreAPI VertexStreamAffordancesT<uint32_t>;
extern template class _3es_coreAPI VertexStreamAffordancesT<float>;
extern template class _3es_coreAPI VertexStreamAffordancesT<double>;
}  // namespace detail

/// A helper class for wrapping various input array types into data streams for data transfer.
///
/// A @c VertexStream is intended to hold a borrowed pointer for use with mesh data. The stream may represent vertex
/// or index data of various data types and sizes, but is expected to be of a particular type on transfer. For example
/// a @c VertexStream may wrap a @c double array representing a @c Vector3 vertex stream. On transfer, the data may be
/// transfered using single precision, or quantised precision.
///
/// There are several key concepts to understanding how the @c VertexStream interprets and stores information. Firstly
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
class _3es_coreAPI VertexStream
{
public:
  VertexStream();

  template <typename T>
  VertexStream(const T *v, size_t count, size_t componentCount = 1, size_t componentStride = 0,
               bool ownPointer = false);

  VertexStream(const Vector3f *v, size_t count);

  VertexStream(const Vector3d *v, size_t count);

  template <typename T>
  VertexStream(const std::vector<T> &v, size_t componentCount = 1, size_t componentStride = 0);

  VertexStream(const std::vector<Vector3f> &v);

  VertexStream(const std::vector<Vector3d> &v);

  VertexStream(VertexStream &&other);

  VertexStream(const VertexStream &other);

  ~VertexStream();

  void reset();

  template <typename T>
  void set(const T *v, size_t count, size_t componentCount = 1, size_t componentStride = 0, bool ownPointer = false);

  template <typename T>
  void set(const std::vector<T> &v, size_t componentCount = 1, size_t componentStride = 0);

  void set(const std::vector<Vector3f> &v);
  void set(const std::vector<Vector3d> &v);

  template <typename T>
  T get(size_t element_index, size_t component_index = 0) const;

  VertexStream &operator=(VertexStream other);

  inline bool isValid() const { return _stream != nullptr; }

  inline unsigned count() const { return _count; }
  inline unsigned basicTypeSize() const { return _basicTypeSize; }
  inline unsigned byteStride() const { return _elementStride * _basicTypeSize; }
  inline unsigned componentCount() const { return _componentCount; }
  inline unsigned elementStride() const { return _elementStride; }

  inline bool ownPointer() const { return int(_flags & Flag::OwnPointer) != 0; }
  inline bool writable() const { return int(_flags & Flag::Writable) != 0; }
  inline DataStreamType type() const { return _type; }

  void swap(VertexStream &other);

  template <typename T>
  const T *ptr(size_t element_index = 0) const;

  template <typename T>
  const T *ptrAt(size_t element_index) const;

  /// Copy the internal array and take ownership. Does nothing if this object already owns its own array memory.
  void duplicate();

  static uint16_t estimateTransferCount(size_t elementSize, unsigned overhead, unsigned byteLimit);

  unsigned write(PacketWriter &packet, uint32_t offset) const;
  unsigned writePacked(PacketWriter &packet, uint32_t offset, float quantisation_unit) const;

  /// Read : reading offset and count from the @p packet
  unsigned read(PacketReader &packet);

  /// Read : skipping offset and count, with @p count given.
  unsigned read(PacketReader &packet, unsigned offset, unsigned count);

  friend inline void swap(VertexStream &a, VertexStream &b) { a.swap(b); }

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
  uint8_t _componentCount{ 0 };  ///< Number of data type component elements in each vertex. E.g., Vector3 has 3.
  /// Number of data type elements between each vertex. For any densely packed array this value will match
  /// @c _componentCount . For alined, or interleaved arrays, this valid will be larger than @c _componentCount .
  ///
  /// For example, an array of 16 byte aligned @c float3 vertices will have a @c _componentCount of 3 and a
  /// @c _elementStride of 4.
  uint8_t _elementStride{ 0 };
  /// Size of the basic @c _type stored in @c stream .
  uint8_t _basicTypeSize{ 0 };
  DataStreamType _type{ DctNone };  ///< The simple data type for @c _stream
  uint8_t _flags{ Flag::Zero };     ///< Does this class own the @c _stream pointer?
  /// Pointer to the implementation for various operations supported on a @c VertexStream . This is using a type
  /// erasure setup.
  detail::VertexStreamAffordances *_affordances{ nullptr };
};

/// Write a @c VertexStream to @p packet as a stream of @c DstType . Note this function requires knowing the concrete
/// type of the @c VertexStream data for proper casting. It is assumed that simple casting from @c SrcType t
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
unsigned writeStream(PacketWriter &packet, const VertexStream &stream, uint32_t offset);

/// Write a @c VertexStream to @p packet as qunatised, and packed data stream. This is intended only for input streams
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
unsigned writeStreamPackedFloat(PacketWriter &packet, const VertexStream &stream, uint32_t offset,
                                const FloatType *packingOrigin, float quantisationUnit, DataStreamType packedType);

unsigned readStream(PacketReader &packet, VertexStream &stream);

}  // namespace tes

#include "3esvertexstream.inl"

#endif  // _3ESVERTEXSTREAM_H
