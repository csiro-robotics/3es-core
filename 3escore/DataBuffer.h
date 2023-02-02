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
  class DataBufferPrimitiveTypeInfo<_type>  \
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
class DataBufferPrimitiveTypeInfo
{
public:
  /// Query the @c DataStreamType corresponding to @c T .
  /// @return The corresponding primitive type.
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
/// Base class for the affordances of a @c DataBuffer.
///
/// These functions handled various data conversion and read/write operations.
class TES_CORE_API DataBufferAffordances
{
public:
  /// Virtual destructor.
  virtual ~DataBufferAffordances();

  /// Release the memory pointer at @p stream_ptr.
  ///
  /// This assumes that the pointer at @p stream_ptr was allocated by this affordance via
  /// @c takeOwnership(). The underlying type is fixed by the affordances implementation.
  /// This function is called even if the @p stream_ptr points to borrowed memory, in which case
  /// the implementation must preserve the memory.
  ///
  /// @param stream_ptr A pointer to the memory pointer that should be released (note the
  /// additional indirection). May be null.
  /// @param has_ownership True if the pointer at @p stream_ptr is owned by the @c DataBuffer and
  /// can be released.
  virtual void release(const void **stream_ptr, bool has_ownership) const = 0;

  /// Called to take ownership of the data pointed to at the address of @p stream_ptr.
  ///
  /// After this call the @p stream_ptr must addres to a pointer to a memory section which is
  /// owned by the @c DataBuffer. More plainly, if @c has_ownership is false and @c *stream_ptr
  /// (note the dereference operator) is not null, then a copy of what is currently at
  /// @c *stream_ptr is made, then @c *stream_ptr updated to point to the copy.
  ///
  /// @param[in,out] stream_ptr A pointer to the memory pointer that should be released (note the
  /// additional indirection). May be null.
  /// @param has_ownership True if we already have ownership of @c *stream_ptr. This function should
  /// only perform it's copy operations when @p has_ownership is false.
  /// @param stream The @p DataBuffer which will take ownership of @p stream_ptr.
  virtual void takeOwnership(const void **stream_ptr, bool has_ownership,
                             const DataBuffer &stream) const = 0;

  /// Write data from @p stream to @p packet ensuring we write data of the type specified by
  /// @p write_as_type.
  ///
  /// This write function writes content from @p stream to @p packet ensuring the correct type is
  /// written regardless of the data type stored in @p stream. We can assume that the @c DataBuffer
  /// primitive type already matches that expected by this @c DataBufferAffordances implementation,
  /// but must writing all @c DataStreamType options. Note that some combinations may not make much
  /// sense in practice, but must be supported regardless.
  ///
  /// @param packet The data packet to write to.
  /// @param offset The element offset into @p stream to start writing from.
  /// @param write_as_type The data type to write to @p packet.
  /// @param byteLimit The maximum number of bytes we can write to @p packet.
  /// @param receiveOffset Receiver offset. This is added to @p offset when writing to th
  /// @p packet. See @c DataBuffer::write() for details.
  /// @param stream The @c DataBuffer to read from. This affordances object is expected to belong
  /// to the @c stream.
  /// @param quantisation_unit Quantisation unit used for @c DctPackedFloat16 and
  /// @c DctPackedFloat32 operations.
  /// @return The number of elements written to @p packet. Zero on failure.
  virtual uint32_t write(PacketWriter &packet, uint32_t offset, DataStreamType write_as_type,
                         unsigned byteLimit, uint32_t receiveOffset, const DataBuffer &stream,
                         double quantisation_unit = 0.0) const = 0;
  /// Write data from @p packet into the address at @p stream_ptr ensuring we read data of the type
  /// expected by these affordances.
  ///
  /// This operation can result in the pointer at @p stream_ptr being reallocated to ensure it
  /// can support `offset + count` elements also considering the @c stream.componentCount().
  /// Note that the @c componentCount() must be correct for the @c DataBuffer before attempting to
  /// read.
  ///
  /// We must be able to read any @c DataStreamType from the @p packet into the expected affordances
  /// data type.
  ///
  /// The packet format is defined by @c DataBuffer::write().
  ///
  /// @param packet The data packet to read from.
  /// @param[in,out] stream_ptr The address of the pointer to read into.
  /// @param[in,out] stream_size The number of elements which can be stored at @p stream_ptr.
  /// Will be updated when @p *stream_ptr is reallocated.
  /// @param[in,out] has_ownership Pointer to the ownership flag of @p stream. If this is false then
  /// the @p *stream_ptr will always be reallocated after which @p *has_ownership is set to true.
  /// @param stream The @c DataBuffer to write to. This is the object which owns @p stream_ptr,
  /// @p stream_size, @p has_ownership and this affordances object.
  /// @return The number of elements read from the @p packet. Zero on failure.
  virtual uint32_t read(PacketReader &packet, void **stream_ptr, unsigned *stream_size,
                        bool *has_ownership, const DataBuffer &stream) const = 0;
  /// This overload skips reading @p offset and @p count from @p packet which are parameters
  /// instead.
  /// @param packet The data packet to read from.
  /// @param[in,out] stream_ptr The address of the pointer to read into.
  /// @param[in,out] stream_size The number of elements which can be stored at @p stream_ptr.
  /// Will be updated when @p *stream_ptr is reallocated.
  /// @param[in,out] has_ownership Pointer to the ownership flag of @p stream. If this is false then
  /// the @p *stream_ptr will always be reallocated after which @p *has_ownership is set to true.
  /// @param stream The @c DataBuffer to write to. This is the object which owns @p stream_ptr,
  /// @p stream_size, @p has_ownership and this affordances object.
  /// @param offset The element offset at which to store data in @p stream.
  /// @param count The number of elements in the @p packet.
  /// @return The number of elements read from the @p packet. Zero on failure.
  virtual uint32_t read(PacketReader &packet, void **stream_ptr, unsigned *stream_size,
                        bool *has_ownership, const DataBuffer &stream, unsigned offset,
                        unsigned count) const = 0;
  /// This implements single element reads from a @c DataBuffer with type conversion.
  ///
  /// This may read one or more components of the specified element as indicated by the
  /// @p component_index and @p component_read_count. For example, we can read the XYZ components
  /// of @c Vector3f data with @c component_index 0 and @c component_read_count 3, we can read YZ
  /// components with @c component_index 1 and @c component_read_count 2 or just the Y component
  /// with @c component_index 1 and @c component_read_count 1.
  ///
  /// The pointer parameters all belong to the @c DataBuffer which owns these affordances.
  ///
  /// Note that reading as @c DctPackedFloat16 or @c DctPackedFloat32 is nonsensical and
  /// implementations should throw @c tes::Exception when @p as_type is one of these nonsense
  /// values.
  ///
  /// @param as_type The data type to convert to.
  /// @param element_index The index of the element to read from the @c DataBuffer.
  /// @param component_index The index of first component at @p element_index to read from the @c
  /// DataBuffer.
  /// @param component_read_count The number of components to read.
  /// @param stream A pointer to the memory referenced by the @p DataBuffer.
  /// @param stream_element_count The number of elements stored in @p stream.
  /// @param stream_component_count The number of components per element in @p stream.
  /// @param stream_element_stride The number of components per element in @p stream given based on
  /// storage, rather than addressibility. That is, this includes padding components.
  /// @param dst The address to write to. Data at this address must match @p as_type.
  /// @param dst_capacity Specifies the number of @p as_type elements which can be stored at @p dst.
  /// That is the maximum destination element count.
  /// @return The number of components read on success, zero on failure.
  virtual size_t get(DataStreamType as_type, size_t element_index, size_t component_index,
                     size_t component_read_count, const void *stream, size_t stream_element_count,
                     size_t stream_component_count, size_t stream_element_stride, void *dst,
                     size_t dst_capacity) const = 0;
};

/// Templated implementations for @c DataBufferAffordances
/// @tparam T The primitive data type matching one of the non-packed types corresponding to
/// @c DataStreamType.
template <typename T>
class TES_CORE_API DataBufferAffordancesT : public DataBufferAffordances
{
public:
  /// Get the singleton implementation for this affordances type.
  /// @return Affordances singleton.
  static DataBufferAffordances *instance();

  void release(const void **stream_ptr, bool has_ownership) const override;
  void takeOwnership(const void **stream_ptr, bool has_ownership,
                     const DataBuffer &stream) const override;
  uint32_t write(PacketWriter &packet, uint32_t offset, DataStreamType write_as_type,
                 unsigned byteLimit, uint32_t receiveOffset, const DataBuffer &stream,
                 double quantisation_unit = 0.0) const override;
  uint32_t read(PacketReader &packet, void **stream_ptr, unsigned *stream_size, bool *has_ownership,
                const DataBuffer &stream) const override;
  uint32_t read(PacketReader &packet, void **stream_ptr, unsigned *stream_size, bool *has_ownership,
                const DataBuffer &stream, unsigned offset, unsigned count) const;

  size_t get(DataStreamType as_type, size_t element_index, size_t component_index,
             size_t component_read_count, const void *stream, size_t stream_element_count,
             size_t stream_component_count, size_t stream_element_stride, void *dst,
             size_t dst_capacity) const override;

  /// Helper function to write data from @c T to the @p WriteType.
  /// @tparam WriteType The data type to write as.
  /// @param packet The data packet to write to.
  /// @param offset An element index offset to start writing from this buffer.
  /// @param write_as_type The @c DataStreamType to write as. This generally matches the type @c T
  /// however it is also valid to have @c DctPackedFloat16 with for @c WriteType @c int16_t and
  /// @c DctDctPackedFloat32 with @c WriteType @c int32_t.
  /// @param byteLimit maximum number of bytes to write to @p packet.
  /// @param receiveOffset Added to the @p offset for the receiver to handle. See remarks.
  /// @param stream The owning @c DataBuffer.
  /// @return The number of elements written or zero on failure.
  template <typename WriteType>
  uint32_t writeAs(PacketWriter &packet, uint32_t offset, DataStreamType write_as_type,
                   unsigned byteLimit, uint32_t receiveOffset, const DataBuffer &stream) const;

  /// Function function for writing packed ata.
  /// @tparam FloatType Either @c float or @c double matching the affordances type.
  /// @tparam PackedType The packet primitive type either @c int16_t for @c DctPackedFloat16 or
  /// @c int32_t for @c DctPackedFloat32.
  /// @param packet The data packet to write to.
  /// @param offset An element index offset to start writing from this buffer.
  /// @param write_as_type The @c DataStreamType to write as; either @c DctPackedFloat16 or
  /// @c DctDctPackedFloat32.
  /// @param byteLimit maximum number of bytes to write to @p packet.
  /// @param receiveOffset Added to the @p offset for the receiver to handle. See remarks.
  /// @param packingOrigin A origin offset applied to help with quantisation. Essentially this
  /// value is subtracted from each element. The number of elements at @c packingOrigin must match
  /// the @c stream.componentCount(). For example, if writing @c Vector3d elements, then
  /// @c packingOrigin must be an array of 3 @c double values (i.e., a @c Vector3d ) and this value
  /// is subtracted from each @c Vector3d before packing.
  /// @param quantisationUnit The quantisation unit divides into each element to quantise it.
  /// @param stream The owning @c DataBuffer.
  /// @return The number of elements written or zero on failure.
  template <typename FloatType, typename PackedType>
  uint32_t writeAsPacked(PacketWriter &packet, uint32_t offset, DataStreamType write_as_type,
                         unsigned byteLimit, uint32_t receiveOffset, const FloatType *packingOrigin,
                         const FloatType quantisationUnit, const DataBuffer &stream) const;

  /// Helper function to read data of type @p ReadType from @p packet into @p *stream_ptr.
  ///
  /// The data type at @p *stream_ptr matches the class template type @c T. The @p *stream_ptr may
  /// be reallocated to mean the required read size requirements.
  ///
  /// @tparam ReadType The data type to read. Generally we
  /// @param packet Data packet to read from.
  /// @param offset Element offset into @p *stream_ptr to write to.
  /// @param count Number of elements from @p packet to read.
  /// @param componentCount Number of components per element.
  /// @param[in,out] stream_ptr A pointer to the @c DataBuffer data pointer.
  /// @return The number of element read, zero on failure.
  template <typename ReadType>
  uint32_t readAs(PacketReader &packet, uint32_t offset, uint32_t count, unsigned componentCount,
                  void **stream_ptr) const;

  /// Helper function to read packed data of type @p ReadType from @p packet into @p *stream_ptr.
  ///
  /// This implements reading @c float from @c DctPackedFloat16 or @c double from
  /// @c DctPackedFloat32 packets.
  ///
  /// @tparam ReadType The data type to read. Generally we
  /// @param packet Data packet to read from.
  /// @param offset Element offset into @p *stream_ptr to write to.
  /// @param count Number of elements from @p packet to read.
  /// @param componentCount Number of components per element.
  /// @param[in,out] stream_ptr A pointer to the @c DataBuffer data pointer.
  /// @return The number of element read, zero on failure.
  template <typename FloatType, typename ReadType>
  uint32_t readAsPacked(PacketReader &packet, unsigned offset, unsigned count,
                        unsigned componentCount, void **stream_ptr) const;
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
/// A @c DataBuffer is intended to abstract various kinds of mesh data streams for read and write
/// operations. A data stream typically represents vertex or index data of various primitive types
/// and sizes, but is expected to be of a particular type on transfer or on read. The @c DataBuffer
/// handles data element conversion from the underlying stream type to the expected stream type.
/// For example a @c DataBuffer may wrap a @c double array representing a @c Vector3 vertex stream.
/// On transfer, the data may be quantised to reduce the data size. A client then reads this vertex
/// data into a new @c DataBuffer of the quantised type, then can read this data buffer as either
/// a @c double precision or single precision @c float data buffer. These conversions are handled
/// by the @c DataBufferAffordances implementations.
///
/// A @c DataBuffer may have either a borrowed pointer to a data stream or it may take ownership of
/// its own memory, potentially copying the original data stream. Borrowed memory results in a read
/// only buffer, while owned memory may be writable.
///
/// There are several key concepts to understanding how the @c DataBuffer interprets and stores
/// information. Firstly the assumptions are that the source array stores @em vertices which can be
/// represented by a simple @em primitiveType: @c intX_t , @c uintX_t , @c float or @c double . The
/// array is broken up into @em vertices where each @em vertex is composed of a number of
/// consecutive
/// @em dataElements determined by the @em componentCount all of the same primitive type. A vertex
/// be followed by some padding - possibly for data alignment - of M @em dataElements. Finally,
/// there number of @em vertices must be known and fixed.
///
/// The terminology is broken down below:
/// - @em primitiveType - the primitive type contained in the stream; e.g., @c int32_t, @c double.
/// - @em dataElements - a number of consecutive @em primitiveType elements
/// - @em componentCount - the number of @em primitiveType elements in each @em vertex
/// - @em vertexStride - the number of @em primitiveType elements between each vertex. This will be
/// at
///   least as large as @em componentCount
///
/// Some examples are provided below to help illustrate the terminology:
///
/// Logical Type       | @em primitiveType  | @em componentCount | @em vertexStride |
/// ------------------ | ------------- | ------------------ | ---------------- |
/// `32-bit indices`   | uint32_t      | 1                  | 1                |
/// `float3 (packed)`  | float         | 3                  | 3                |
/// `float3 (aligned)` | float         | 3                  | 4                |
///
/// Note `float3 (aligned)` assumes 16-byte data alignment, which is often optimal for single
/// precision vertex storage. In contrast `float3 (packed)` assumes densely packed float triplets
/// such as the @c tes::Vector3f definition.
///
/// The byte size of each element is calculated as `sizeof(primitiveType) * componentCount`.
///
/// The byte size of the entire array is calculated as
/// `count() * sizeof(primitiveType) * componentCount`. @c componentCount() values above 16 are not
/// supported.
///
/// The @c DataBuffer class is designed to work with @c PacketWriter and @c PacketReader classes
/// allowing the contents to be transferred from a third eye scene server to a client. The code
/// below shows how to send and receive @c DataBuffer contents.
///
/// @code
/// // TODO
/// @endcode
class TES_CORE_API DataBuffer
{
public:
  /// Default constructor. The resulting @c DataBuffer is of @c type() @c DctNone and is not usable
  /// unless @c set() is called.
  DataBuffer();

  /// Configure an empty buffer of the given type, component count and stride.
  ///
  /// Note that the @c DataStreamType values @c DctNone or packed data types are invalid for this
  /// instantiation and result in an invalid @c DataBuffer, equivalent to calling @c DataBuffer().
  ///
  /// @param type The stream type. Using @c DctNone or a packed data type is invalid.
  /// @param componentCount Number of components of @c type in each element.
  /// @param componentStride Stride between elements. The stride is sized by @p type and must
  /// be @c >= @p componentCount excepting that a zero value implies
  /// `componentStride = componentCount`.
  explicit DataBuffer(DataStreamType type, size_t componentCount = 1, size_t componentStride = 0);

  /// Construct from a raw pointer array.
  /// @tparam T The array data type. See @em primitiveType restrictions in class comments.
  /// @param v The vertex data array.
  /// @param count Number of vertex elements in the array.
  /// @param componentCount Number of components of type @p T in each element.
  /// @param componentStride Stride between elements in @p v. The stride is sized by @p T and must
  /// be @c >= @p componentCount excepting that a zero value implies
  /// `componentStride = componentCount`.
  /// @param ownPointer True to take ownership of the memory at @p v.
  template <typename T>
  DataBuffer(const T *v, size_t count, size_t componentCount = 1, size_t componentStride = 0,
             bool ownPointer = false);

  /// Construct a vertex data buffer from a @c Vector3f data type using borrowed memory.
  /// @param v The vertex array.
  /// @param count The number of vertex elements in @p v.
  DataBuffer(const Vector3f *v, size_t count);

  /// Construct a vertex data buffer from a @c Vector3f @c std::array using borrowed memory.
  /// @tparam Count The number of elements in @p v.
  /// @param v The vertex array.
  template <size_t Count>
  DataBuffer(const std::array<Vector3f, Count> &v)
    : DataBuffer(v.data(), Count)
  {}

  /// Construct a vertex data buffer from a @c Vector3d data type using borrowed memory.
  /// @param v The vertex array.
  /// @param count The number of vertex elements in @p v.
  DataBuffer(const Vector3d *v, size_t count);

  /// Construct a vertex data buffer from a @c Vector3d @c std::array using borrowed memory.
  /// @tparam Count The number of elements in @p v.
  /// @param v The vertex array.
  template <size_t Count>
  DataBuffer(const std::array<Vector3d, Count> &v)
    : DataBuffer(v.data(), Count)
  {}

  /// Construct from a @c Colour array using borrowed memory.
  ///
  /// Each colour is represented by a set of 4 component @c uint8_t entries {red, green, blue,
  /// alpha}.
  ///
  /// @param c The colour array.
  /// @param count The number of elements in @p c.
  DataBuffer(const Colour *c, size_t count);

  /// Construct from a @c Colour @c std::array using borrowed memory.
  ///
  /// Each colour is represented by a set of 4 component @c uint8_t entries {red, green, blue,
  /// alpha}.
  ///
  /// @tparam Count The number of elements in @p c.
  /// @param c The colour array.
  template <size_t Count>
  DataBuffer(const std::array<Colour, Count> &c)
    : DataBuffer(c.data(), Count)
  {}

  /// Construct a data buffer from a @c std::vector using borrowed memory.
  /// @tparam T The array data type. See @em primitiveType restrictions in class comments.
  /// @param v The vertex data array.
  /// @param componentCount Number of components of type @p T in each element.
  /// @param componentStride Stride between elements in @p v. The stride is sized by @p T and must
  /// be @c >=
  ///   @p componentCount excepting that a zero value implies `componentStride = componentCount`.
  template <typename T>
  DataBuffer(const std::vector<T> &v, size_t componentCount = 1, size_t componentStride = 0);

  /// Construct a data buffer from a @c std::array using borrowed memory. Component stride and count
  /// are 1.
  /// @tparam T The array data type. See @em primitiveType restrictions in class comments.
  /// @tparam Count Number of vertex elements in the array.
  /// @param v The vertex data array.
  template <typename T, size_t Count>
  DataBuffer(const std::array<T, Count> &v)
    : DataBuffer(v.data(), Count)
  {}

  /// Construct from a @c Vector3f @c std::vector using borrowed memory.
  /// @param v The vertex array.
  DataBuffer(const std::vector<Vector3f> &v);

  /// Construct from a @c Vector3d @c std::vector using borrowed memory.
  /// @param v The vertex array.
  DataBuffer(const std::vector<Vector3d> &v);

  /// Construct from a @c Colour @c std::vector using borrowed memory.
  ///
  /// Each colour is represented by a set of 4 component @c uint8_t entries {red, green, blue,
  /// alpha}.
  ///
  /// @param v The colour array.
  DataBuffer(const std::vector<Colour> &v);

  /// Move constructor.
  /// @param other Object to move.
  DataBuffer(DataBuffer &&other);

  /// Copy constructor. This borrows memory from @p other and results in a @c DataBuffer which
  /// @em borrows the memory. Use @c duplicate() to copy the data into owned memory.
  /// @param other Object to copy.
  DataBuffer(const DataBuffer &other);

  /// Destructor.
  ~DataBuffer();

  /// Clears this @c DataBuffer releasing memory as required. This results in an invalid
  /// @c DataBuffer object.
  void reset();

  /// Set data from a raw pointer array.
  ///
  /// Existing memory is released as required.
  ///
  /// @tparam T The array data type. See @em primitiveType restrictions in class comments.
  /// @param v The vertex data array.
  /// @param count Number of vertex elements in the array.
  /// @param componentCount Number of components of type @p T in each element.
  /// @param componentStride Stride between elements in @p v. The stride is sized by @p T and must
  /// be @c >=
  ///   @p componentCount excepting that a zero value implies `componentStride = componentCount`.
  /// @param ownPointer True to take ownership of the memory at @p v.
  template <typename T>
  void set(const T *v, size_t count, size_t componentCount = 1, size_t componentStride = 0,
           bool ownPointer = false);

  /// Set data from a @c std::vector using borrowed memory.
  ///
  /// Existing memory is released as required.
  ///
  /// @tparam T The array data type. See @em primitiveType restrictions in class comments.
  /// @param v The vertex data array.
  /// @param componentCount Number of components of type @p T in each element.
  /// @param componentStride Stride between elements in @p v. The stride is sized by @p T and must
  /// be @c >=
  ///   @p componentCount excepting that a zero value implies `componentStride = componentCount`.
  template <typename T>
  void set(const std::vector<T> &v, size_t componentCount = 1, size_t componentStride = 0);

  /// Set from a @c Vector3f @c std::vector using borrowed memory.
  ///
  /// Existing memory is released as required.
  ///
  /// @param v The vertex array.
  void set(const std::vector<Vector3f> &v);
  /// Set from a @c Vector3d @c std::vector using borrowed memory.
  ///
  /// Existing memory is released as required.
  ///
  /// @param v The vertex array.
  void set(const std::vector<Vector3d> &v);
  /// Set from a @c Colour @c std::vector using borrowed memory.
  ///
  /// Existing memory is released as required.
  ///
  /// @param v The colour array.
  void set(const std::vector<Colour> &v);

  /// Read a single item at the given element index, and component index.
  ///
  /// The element index accounts for element striding, while the component index allows reading
  /// intermediate values. For example, consider a @c DataBuffer creates from 10 @c Vector3f
  /// elements. This creates a @c float
  /// @c DataBuffer with an element count of 10, an element stride of 3 and a component count of 3
  /// (for the XYZ channels of the @c Vector3f . The @p element_index is used to address each @c
  /// Vector3f while the
  /// @c component_count is used in the range [0, 2] to extract the XYZ values.
  ///
  /// @note This only supports reading primitive types, meaning that template types of @c
  /// Vector3&lt;T&gt; and @c Colour are not supported. Use @c float or @c double for @c
  /// Vector3&lt;T&gt; buffers and @c uint32_t for @c Colour .
  template <typename T>
  T get(size_t element_index, size_t component_index = 0) const;

  /// Read a block of data from the buffer. This reads from the @p element_index reading @c
  /// element_count data items into @p dst .
  ///
  /// There are some caveats to consider here because a buffer may have a @c componentCount()
  /// greater than 1 (such as for @c Vector3&lt;T&gt; buffers). The effect is that for a successful
  /// read operation the @p capacity at @p dst must be at least @p element_count * @c
  /// componentCount() . This essentially makes the 'units' of @c capacity will different from @p
  /// element_index , @p element_count and the return value when the @c DataBuffer has a
  /// @c componentCount() greater than 1. The items are read in a tightly packed fasion into @p dst
  /// .
  ///
  /// @note This only supports reading primitive types, meaning that template types of @c
  /// Vector3&lt;T&gt; and @c Colour are not supported. Use @c float or @c double for @c
  /// Vector3&lt;T&gt; buffers and @c uint32_t for @c Colour .
  ///
  /// @param element_index The index of the element in the buffer to start reading from.
  /// @param element_count The number of elements to read.
  /// @param dst The address to read into.
  /// @param capacity The data capacity of @c dst , as the number of @c T elements it can hold.
  /// @tparam T Data type to read as. Must be a primitive type as supported by @c DataStreamType
  /// (see note above).
  /// @return The number of @c DataBuffer @em elements read. The number of @c T elements written to
  /// @p dst will be
  ///   this value times the @c componentCount() .
  template <typename T>
  size_t get(size_t element_index, size_t element_count, T *dst, size_t capacity) const;

  /// Move assignment.
  /// @param other Object to move.
  /// @return @c *this
  DataBuffer &operator=(DataBuffer &&other);

  /// Copy assignment operator. This borrows memory from @p other and results in a @c DataBuffer
  /// which @em borrows the memory. Use @c duplicate() to copy the data into owned memory.
  ///
  /// @param other Object to copy.
  /// @return @c *this
  DataBuffer &operator=(const DataBuffer &other);

  /// Checks if the data buffer is valid.
  ///
  /// Validity is determined by having a valid pointer. Note that if this is a borrowed pointer then
  /// behaviour is undefined if the original memory has been released.
  ///
  /// @return True if the buffer has a valid pointer.
  bool isValid() const { return _stream != nullptr; }

  /// Return the number of elements in the data buffer.
  /// @return The number of elements in the buffer.
  unsigned count() const { return _count; }

  /// Returns the number of addressable primitives in the data buffer.
  ///
  /// This is essentially `count() * componentCount()`. For example, in a data buffer containing
  /// 12 @c Vector3f elements, the @c addressableCount() yields the number of @c float values which
  /// can be read from the buffer; 12 @c Vector3f elements with 3 @c float elements per @c Vector3f.
  ///
  /// @return The number of addressable primitives in the data buffer.
  unsigned addressableCount() const { return _count * _componentCount; }

  /// Return the size of the primitive type stored in the @c DataBuffer - e.g., @c int32_t, @c
  /// double, etc.
  /// @return The buffer primitive type.
  unsigned primitiveTypeSize() const { return _primitiveTypeSize; }
  /// Return the byte stride between elements in the buffer. Must be at lease the size of the
  /// primitive type times the @c componentCount().
  /// @return The element byte stride.
  unsigned byteStride() const { return _elementStride * _primitiveTypeSize; }
  /// Return the number of primitive components (or channels) in each element.
  /// @return The component count.
  unsigned componentCount() const { return _componentCount; }
  /// Return the stride between elements in the buffer where where the unit is a single primitive.
  /// @return The element stride.
  unsigned elementStride() const { return _elementStride; }

  /// Check if this buffer owns its memory.
  /// @return True if this data buffer owns it's memory.
  bool ownPointer() const { return int(_flags & Flag::OwnPointer) != 0; }

  /// Return and identifier for the data primitive stored in the buffer.
  /// @return The buffer primitive type.
  DataStreamType type() const { return _type; }

  /// Swap the contents of this buffer with @p other.
  /// @param other The buffer to swap with.
  void swap(DataBuffer &other);

  /// Retrieve a read only primitive pointer into the data buffer.
  ///
  /// The template type @p T must be compatible with the @c DataStreamType indicated by @c type() or
  /// behaviour is undefined.
  ///
  /// An @p element_index may optionally be used to retrieve a pointer to the Nth element in the
  /// buffer. @c element_index must be in the range `[0, count()]` or behaviour is undefined.
  ///
  /// @tparam T The type to cast to.
  /// @param element_index The index of the element to retrieve a pointer to.
  /// @return A pointer of type @p T into the buffer at @p element_index.
  template <typename T>
  const T *ptr(size_t element_index = 0) const;

  /// Retrieve a read only primitive pointer into the data buffer with bounds checking.
  ///
  /// The template type @p T must be compatible with the @c DataStreamType indicated by @c type() or
  /// behaviour is undefined.
  ///
  /// An @p element_index may optionally be used to retrieve a pointer to the Nth element in the
  /// buffer. If @c element_index is not in range `[0, count()]`, the a @c nullptr is returned.
  ///
  /// @tparam T The type to cast to.
  /// @param element_index The index of the element to retrieve a pointer to.
  /// @return A pointer of type @p T into the buffer at @p element_index or a @c nullptr if
  /// @p element_index is out of range.
  template <typename T>
  const T *ptrAt(size_t element_index) const;

  /// Copy the internal array and take ownership. Does nothing if this object already owns its own
  /// array memory.
  /// @return @c *this
  DataBuffer &duplicate();

  /// Estimates now many elements may be packed for a network transfer operation given the specified
  /// limits.
  ///
  /// @param elementSize The size of each data element to transfer.
  /// @param overhead Byte overhead for a single transfer packet (headers etc), which effectively
  /// reduces the @c byteLimit.
  /// @param byteLimit Maximum number of bytes which can be transferred.
  /// @return The maximum number of elements which can be packed into a single network packet.
  static uint16_t estimateTransferCount(size_t elementSize, unsigned overhead, unsigned byteLimit);

  // receiveOffset: offset packed into the message for the receiver to handle. Allows a small vertex
  // buffer to represent a slice of a buffer at the other end.
  /// Write data from the @c DataBuffer to @p packet.
  ///
  /// This writes elements from the data buffer to the @p packet up to a maximum determined by
  /// @c estimateTransferCount(). The @p offset is an element offset representing the index of the
  /// first element to pack from the buffer. The @p byteLimit sets the maximum number of bytes which
  /// can be packed.
  ///
  /// Note that packing removes any alignment offsets, meaning the packed data has an effective
  /// @c componentStride() equal to the @p componentCount().
  ///
  /// Finally the @p receiveOffset maybe used to adjust the @p offset at the receiving end.
  /// Essentially it is added to the element offset value written to the packet. The use case is
  /// to allow a small @c DataBuffer to be created by the sender, which is used to write into a
  /// section of a larger buffer the receiver has; i.e., update a small section of an existing
  /// buffer using a smaller buffer as the data source.
  ///
  /// The data written to the @p packet is as follows:
  ///
  /// - @c uint32_t @c offset - the index of the first element in the @p packet.
  ///   Calculated as `offset + receiveOffset`.
  /// - @c uin16_t @c count - the number of elements in the @p packet.
  /// - @c uint8_t @c componentCount - number of primitives in each element (channels).
  /// - @c uint8_t @c dataType - the @c DataStreamType of data in the buffer.
  /// - buffer data
  ///
  /// @param packet The data packet to write to.
  /// @param offset An element index offset to start writing from this buffer.
  /// @param byteLimit maximum number of bytes to write to @p packet.
  /// @param receiveOffset Added to the @p offset for the receiver to handle. See remarks.
  /// @return The number of elements written.
  unsigned write(PacketWriter &packet, uint32_t offset, unsigned byteLimit = 0,
                 uint32_t receiveOffset = 0) const;

  /// Write data from this buffer in a packet/quantised form if possible.
  ///
  /// This selects the most appropriate quantisation type based on the buffer @c type() as indicated
  /// below:
  ///
  /// - @c DctFloat32 write as @c DctPackedFloat16
  /// - @c DctFloat64 write as @c DctPackedFloat32
  ///
  /// All other types are written as is.
  ///
  /// From this we see that @c float and @c double data elements are quantised to use half the data
  /// width. The @p quantisation_unit specifies the quantisation precision - i.e., the value of a
  /// single quantised unit. Each element is divided by this value on write and multiplied by
  /// this value on read.
  ///
  /// Otherwise this method functions as the @c write() method.
  ///
  /// @param packet The data packet to write to.
  /// @param offset An element index offset to start writing from this buffer.
  /// @param byteLimit maximum number of bytes to write to @p packet.
  /// @param receiveOffset Added to the @p offset for the receiver to handle. See remarks.
  /// @return The number of elements written.
  unsigned writePacked(PacketWriter &packet, uint32_t offset, double quantisation_unit,
                       unsigned byteLimit = 0, uint32_t receiveOffset = 0) const;

  /// Read content from the given @p packet first reading the packet data count and offset.
  ///
  /// The packet is assumed to have the following format:
  ///
  /// - @c uint32_t @c offset - the element offset to write into this buffer.
  /// - @c uint16_t @c count - the number of elements to read from the @p packet.
  /// - @c uint8_t @c componentCount - the @c componentCount() of data in the @p packet (channels).
  /// - @c uint8_t @c packetType - the @c DataStreamType of data in the packet.
  ///
  /// This function uses @c DataBufferAffordances to decode from the @p packet into the
  /// @c DataBuffer making it possible to read from one data stream type into another. Additionally
  /// this @c DataBuffer is resized when required to ensure that it can support `offset + count`
  /// elements.
  ///
  /// Note this process can unpack from a quantised data buffer to an unpacked buffer; e.g., from
  /// @c DctPackedFloat16 to @c DctFloat32.
  ///
  /// Note there are some limitations to the data conversion process. Most notably, the
  /// @c componentCount in the @p packet must match this @c componentCount(). This implies that the
  /// @c DataBuffer must be constructed before reading with the expected destination primitive type
  /// and @c componentCount(). The @c read() function fails when the @p componentCount() does not
  /// match.
  ///
  /// @param packet The data packet to read from.
  /// @return The number of elements read from @p packet. Zero on failure.
  unsigned read(PacketReader &packet);

  /// Read content from the given @p packet using the given @p offset and @p count.
  ///
  /// This method is identical to the @p read(PacketReader&) overload except that it skips reading
  /// the @p offset and @p count, using the provided values instead.
  ///
  /// @param packet The data packet to read from.
  /// @param offset The element offset to write into this buffer.
  /// @param count The number of elements to read from the @p packet.
  /// @return The number of elements read from @p packet. Zero on failure.
  unsigned read(PacketReader &packet, unsigned offset, unsigned count);

  /// Implementation for @c std::swap().
  /// @param a First buffer.
  /// @param b Second buffer.
  friend void swap(DataBuffer &a, DataBuffer &b) { a.swap(b); }

private:
  /// Flag values for @c _flags
  enum Flag : uint8_t
  {
    Zero = 0,               ///< Zero value
    OwnPointer = (1 << 0),  ///< Indicates this object owns the heap allocation for @c _stream
  };

  /// Get a writable address for the data buffer.
  ///
  /// This is the @c DataBuffer stream point provided the buffer owns its own memory. Otherwise
  /// a @c nullptr is given.
  ///
  /// @return The writable address, or @c nullptr when the buffer is not writable.
  void *writePtr() { return (ownPointer()) ? const_cast<void *>(_stream) : nullptr; }

  const void *_stream = nullptr;
  unsigned _count = 0;  ///< Number of vertices in the @p _stream .
  uint8_t _componentCount =
    1;  ///< Number of primitive type component elements in each vertex. E.g., Vector3 has 3.
  /// Number of primitive type elements between each vertex. For any densely packed array this value
  /// will match
  /// @c _componentCount . For aligned, or interleaved arrays, this valid will be larger than @c
  /// _componentCount .
  ///
  /// For example, an array of 16 byte aligned @c float3 vertices will have a @c _componentCount of
  /// 3 and a
  /// @c _elementStride of 4.
  uint8_t _elementStride = 1;
  /// Size of the primitive @c _type stored in @c stream .
  uint8_t _primitiveTypeSize = 0;
  DataStreamType _type = DctNone;  ///< The primitive type for @c _stream
  uint8_t _flags = Flag::Zero;     ///< Does this class own the @c _stream pointer?
  /// Pointer to the implementation for various operations supported on a @c DataBuffer . This is
  /// using a type erasure setup.
  detail::DataBufferAffordances *_affordances{ nullptr };
};
}  // namespace tes

#include "DataBuffer.inl"

#endif  // TES_CORE_DATA_BUFFER_H
