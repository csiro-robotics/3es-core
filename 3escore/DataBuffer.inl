//
// author: Kazys Stepanas
//

namespace tes
{
inline DataBuffer::DataBuffer() = default;


inline DataBuffer::DataBuffer(DataStreamType type, size_t component_count, size_t component_stride)
  : _component_count(int_cast<uint8_t>(component_count))
  , _element_stride(int_cast<uint8_t>(component_stride ? component_stride : component_count))
  , _type(type)
  , _flags((type != DctNone) ? Flag::OwnPointer : Flag::Zero)
{
  switch (type)
  {
  case DctNone:
  case DctPackedFloat16:
  case DctPackedFloat32:
    _component_count = _element_stride = 0;
    break;
  case DctInt8:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<int8_t>::size());
    _affordances = detail::DataBufferAffordancesT<int8_t>::instance();
    break;
  case DctUInt8:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<uint8_t>::size());
    _affordances = detail::DataBufferAffordancesT<uint8_t>::instance();
    break;
  case DctInt16:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<int16_t>::size());
    _affordances = detail::DataBufferAffordancesT<int16_t>::instance();
    break;
  case DctUInt16:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<uint16_t>::size());
    _affordances = detail::DataBufferAffordancesT<uint16_t>::instance();
    break;
  case DctInt32:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<int32_t>::size());
    _affordances = detail::DataBufferAffordancesT<int32_t>::instance();
    break;
  case DctUInt32:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<uint32_t>::size());
    _affordances = detail::DataBufferAffordancesT<uint32_t>::instance();
    break;
  case DctInt64:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<int64_t>::size());
    _affordances = detail::DataBufferAffordancesT<int64_t>::instance();
    break;
  case DctUInt64:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<uint64_t>::size());
    _affordances = detail::DataBufferAffordancesT<uint64_t>::instance();
    break;
  case DctFloat32:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<float>::size());
    _affordances = detail::DataBufferAffordancesT<float>::instance();
    break;
  case DctFloat64:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<double>::size());
    _affordances = detail::DataBufferAffordancesT<double>::instance();
    break;
  }
}

template <typename T>
inline DataBuffer::DataBuffer(const T *v, size_t count, size_t component_count,
                              size_t component_stride, bool own_pointer)
  : _stream(v)
  , _count(int_cast<unsigned>(count))
  , _component_count(int_cast<uint8_t>(component_count))
  , _element_stride(int_cast<uint8_t>(component_stride ? component_stride : component_count))
  , _primitive_type_size(int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<T>::size()))
  , _type(DataBufferPrimitiveTypeInfo<T>::type())
  , _flags(!!own_pointer * Flag::OwnPointer)
  , _affordances(detail::DataBufferAffordancesT<T>::instance())
{}


inline DataBuffer::DataBuffer(const Vector3f *v, size_t count)
  : _stream(v ? v->storage().data() : nullptr)
  , _count(int_cast<unsigned>(count))
  , _component_count(3)
  , _element_stride(int_cast<uint8_t>(sizeof(Vector3f) / sizeof(float)))
  , _primitive_type_size(int_cast<uint8_t>(sizeof(Vector3f)))
  , _type(DataBufferPrimitiveTypeInfo<float>::type())
  , _flags(0)
  , _affordances(detail::DataBufferAffordancesT<float>::instance())
{}


inline DataBuffer::DataBuffer(const Vector3d *v, size_t count)
  : _stream(v ? v->storage().data() : nullptr)
  , _count(int_cast<unsigned>(count))
  , _component_count(3)
  , _element_stride(int_cast<uint8_t>(sizeof(Vector3d) / sizeof(double)))
  , _primitive_type_size(int_cast<uint8_t>(sizeof(Vector3d)))
  , _type(DataBufferPrimitiveTypeInfo<double>::type())
  , _flags(0)
  , _affordances(detail::DataBufferAffordancesT<double>::instance())
{}


inline DataBuffer::DataBuffer(const Colour *c, size_t count)
  : _stream(c ? c->storage().data() : nullptr)
  , _count(int_cast<unsigned>(count))
  , _component_count(1)
  , _element_stride(1)
  , _primitive_type_size(int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<uint32_t>::size()))
  , _type(DctUInt32)
  , _flags(0)
  , _affordances(detail::DataBufferAffordancesT<uint32_t>::instance())
{
  static_assert(sizeof(Colour) == DataBufferPrimitiveTypeInfo<uint32_t>::size());
}


template <typename T>
inline DataBuffer::DataBuffer(const std::vector<T> &v, size_t component_count,
                              size_t component_stride)
  : _stream(v.data())
  , _count(int_cast<unsigned>(v.size() / (component_stride ? component_stride : component_count)))
  , _component_count(int_cast<uint8_t>(component_count))
  , _element_stride(int_cast<uint8_t>(component_stride ? component_stride : component_count))
  , _primitive_type_size(int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<T>::size()))
  , _type(DataBufferPrimitiveTypeInfo<T>::type())
  , _flags(0)
  , _affordances(detail::DataBufferAffordancesT<T>::instance())
{}

inline DataBuffer::DataBuffer(const std::vector<Vector3f> &v)
  : _stream(v.data()->storage().data())
  , _count(int_cast<unsigned>(v.size()))
  , _component_count(3)
  , _element_stride(int_cast<uint8_t>(sizeof(Vector3f) / sizeof(float)))
  , _primitive_type_size(int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<float>::size()))
  , _type(DctFloat32)
  , _flags(0)
  , _affordances(detail::DataBufferAffordancesT<float>::instance())
{}

inline DataBuffer::DataBuffer(const std::vector<Vector3d> &v)
  : _stream(v.data()->storage().data())
  , _count(int_cast<unsigned>(v.size()))
  , _component_count(3)
  , _element_stride(int_cast<uint8_t>(sizeof(Vector3d) / sizeof(double)))
  , _primitive_type_size(int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<double>::size()))
  , _type(DctFloat64)
  , _flags(0)
  , _affordances(detail::DataBufferAffordancesT<double>::instance())
{}

inline DataBuffer::DataBuffer(const std::vector<Colour> &colours)
  : _stream(colours.data()->storage().data())
  , _count(int_cast<unsigned>(colours.size()))
  , _component_count(1)
  , _element_stride(1)
  , _primitive_type_size(int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<uint32_t>::size()))
  , _type(DctUInt32)
  , _flags(0)
  , _affordances(detail::DataBufferAffordancesT<uint32_t>::instance())
{
  static_assert(sizeof(Colour) == DataBufferPrimitiveTypeInfo<uint32_t>::size());
}

inline DataBuffer::DataBuffer(DataBuffer &&other) noexcept
  : _stream(std::exchange(other._stream, nullptr))
  , _count(std::exchange(other._count, 0))
  , _component_count(std::exchange(other._component_count, 0))
  , _element_stride(std::exchange(other._element_stride, 0))
  , _primitive_type_size(std::exchange(other._primitive_type_size, 0))
  , _type(std::exchange(other._type, DctNone))
  , _flags(std::exchange(other._flags, 0))
  , _affordances(std::exchange(other._affordances, nullptr))
{}

inline DataBuffer::DataBuffer(const DataBuffer &other)
  : _stream(other._stream)
  , _count(other._count)
  , _component_count(other._component_count)
  , _element_stride(other._element_stride)
  , _primitive_type_size(other._primitive_type_size)
  , _type(other._type)
  , _flags(0)  // Copy assignment. We do not own the pointer.
  , _affordances(other._affordances)
{}

inline DataBuffer::~DataBuffer()
{
  reset();
}

template <typename T>
inline void DataBuffer::set(const T *v, size_t count, size_t component_count,
                            size_t component_stride, bool own_pointer)
{
  *this = std::move(DataBuffer(v, count, component_count, component_stride, own_pointer));
}

template <typename T>
inline void DataBuffer::set(const std::vector<T> &v, size_t component_count,
                            size_t component_stride)
{
  *this = std::move(DataBuffer(v, component_count, component_stride));
}

inline void DataBuffer::set(const std::vector<Vector3f> &v)
{
  *this = std::move(DataBuffer(v));
}

inline void DataBuffer::set(const std::vector<Vector3d> &v)
{
  *this = std::move(DataBuffer(v));
}

inline void DataBuffer::set(const std::vector<Colour> &colours)
{
  *this = std::move(DataBuffer(colours));
}

template <typename T>
inline T DataBuffer::get(size_t element_index, size_t component_index) const
{
  T datum{ 0 };
  _affordances->get(DataBufferPrimitiveTypeInfo<T>::type(), element_index, component_index, 1,
                    _stream, _count, _component_count, _element_stride, &datum, 1);
  return datum;
}

template <typename T>
inline size_t DataBuffer::get(size_t element_index, size_t element_count, T *dst,
                              size_t capacity) const
{
  const size_t components_read = _affordances->get(
    DataBufferPrimitiveTypeInfo<T>::type(), element_index, 0, element_count * _component_count,
    _stream, _count, _component_count, _element_stride, dst, capacity);
  return components_read / componentCount();
}

inline void DataBuffer::swap(DataBuffer &other) noexcept
{
  std::swap(_stream, other._stream);
  std::swap(_count, other._count);
  std::swap(_component_count, other._component_count);
  std::swap(_element_stride, other._element_stride);
  std::swap(_primitive_type_size, other._primitive_type_size);
  std::swap(_type, other._type);
  std::swap(_flags, other._flags);
  std::swap(_affordances, other._affordances);
}

inline DataBuffer &DataBuffer::operator=(DataBuffer &&other) noexcept
{
  swap(other);
  return *this;
}

inline DataBuffer &DataBuffer::operator=(const DataBuffer &other)
{
  if (this != &other)
  {
    _stream = other._stream;
    _count = other._count;
    _component_count = other._component_count;
    _element_stride = other._element_stride;
    _primitive_type_size = other._primitive_type_size;
    _type = other._type;
    _flags = 0;  // Copy assignment. We do not own the pointer.
    _affordances = other._affordances;
  }
  return *this;
}

template <typename T>
inline const T *DataBuffer::ptr(size_t element_index) const
{
  TES_ASSERT2(DataBufferPrimitiveTypeInfo<T>::type() == _type, "Element type mismatch");
  return &static_cast<const T *>(_stream)[element_index];
}

template <typename T>
inline const T *DataBuffer::ptrAt(size_t element_index) const
{
  if (DataBufferPrimitiveTypeInfo<T>::type() == _type)
  {
    return &static_cast<const T *>(_stream)[element_index];
  }
  return nullptr;
}

inline uint16_t DataBuffer::estimateTransferCount(size_t element_size, unsigned overhead,
                                                  unsigned byte_limit)
{
  // FIXME: Without additional overhead padding I was getting missing messages at the client with
  // no obvious error path. For this reason, we use 0xff00u, instead of 0xffffu
  //           packet header           message                 crc
  const size_t max_transfer =
    (0xff00u - (sizeof(PacketHeader) + overhead + sizeof(PacketWriter::CrcType))) / element_size;
  size_t count = byte_limit ? byte_limit / element_size : max_transfer;
  if (count > max_transfer)
  {
    count = max_transfer;
  }

  return static_cast<uint16_t>(count);
}

namespace detail
{
template <typename T>
DataBufferAffordances *DataBufferAffordancesT<T>::instance()
{
  static DataBufferAffordancesT<T> obj;
  return &obj;
}

template <typename T>
void DataBufferAffordancesT<T>::release(const void **stream_ptr, bool has_ownership) const
{
  if (has_ownership)
  {
    delete reinterpret_cast<const T *>(*stream_ptr);
    *stream_ptr = nullptr;
  }
}

template <typename T>
void DataBufferAffordancesT<T>::takeOwnership(const void **stream_ptr, bool has_ownership,
                                              const DataBuffer &stream) const
{
  if (has_ownership || *stream_ptr == nullptr)
  {
    // Already has ownership
    return;
  }

  // Allocate a new array.
  const T *src_array = static_cast<const T *>(*stream_ptr);
  T *new_array = new T[static_cast<size_t>(stream.count()) * stream.elementStride()];
  *stream_ptr = new_array;
  std::copy(src_array, src_array + stream.count() * stream.elementStride(), new_array);
}

template <typename T>
uint32_t DataBufferAffordancesT<T>::write(PacketWriter &packet, uint32_t offset,
                                          DataStreamType write_as_type, unsigned byte_limit,
                                          uint32_t receive_offset, const DataBuffer &stream,
                                          double quantisation_unit) const
{
  switch (write_as_type)
  {
  case DctInt8:
    return writeAs<int8_t>(packet, offset, write_as_type, byte_limit, receive_offset, stream);
  case DctUInt8:
    return writeAs<uint8_t>(packet, offset, write_as_type, byte_limit, receive_offset, stream);
  case DctInt16:
    return writeAs<int16_t>(packet, offset, write_as_type, byte_limit, receive_offset, stream);
  case DctUInt16:
    return writeAs<uint16_t>(packet, offset, write_as_type, byte_limit, receive_offset, stream);
  case DctInt32:
    return writeAs<int32_t>(packet, offset, write_as_type, byte_limit, receive_offset, stream);
  case DctUInt32:
    return writeAs<uint32_t>(packet, offset, write_as_type, byte_limit, receive_offset, stream);
  case DctInt64:
    return writeAs<int64_t>(packet, offset, write_as_type, byte_limit, receive_offset, stream);
  case DctUInt64:
    return writeAs<uint64_t>(packet, offset, write_as_type, byte_limit, receive_offset, stream);
  case DctFloat32:
    return writeAs<float>(packet, offset, write_as_type, byte_limit, receive_offset, stream);
  case DctFloat64:
    return writeAs<double>(packet, offset, write_as_type, byte_limit, receive_offset, stream);
  case DctPackedFloat16:
    return writeAsPacked<float, int16_t>(packet, offset, write_as_type, byte_limit, receive_offset,
                                         nullptr, static_cast<float>(quantisation_unit), stream);
  case DctPackedFloat32:
    return writeAsPacked<double, int32_t>(packet, offset, write_as_type, byte_limit, receive_offset,
                                          nullptr, quantisation_unit, stream);
  default:
    // Throw?
    return 0;
  }
}

template <typename T>
template <typename WriteType>
uint32_t DataBufferAffordancesT<T>::writeAs(PacketWriter &packet, uint32_t offset,
                                            DataStreamType write_as_type, unsigned byte_limit,
                                            uint32_t receive_offset, const DataBuffer &stream) const
{
  const unsigned item_size = static_cast<unsigned>(sizeof(WriteType)) * stream.componentCount();

  // Overhead: account for:
  // - uint32_t offset
  // - uint16_t count
  // - uint8_t component count
  // - uint8_t data type
  const unsigned overhead = sizeof(uint32_t) +  // offset
                            sizeof(uint16_t) +  // count
                            sizeof(uint8_t) +   // element stride
                            sizeof(uint8_t);    // data type;

  byte_limit =
    (byte_limit) ? (byte_limit > overhead ? byte_limit - overhead : 0) : packet.bytesRemaining();
  uint16_t transfer_count = DataBuffer::estimateTransferCount(item_size, overhead, byte_limit);
  if (transfer_count > stream.count() - offset)
  {
    transfer_count = static_cast<uint16_t>(stream.count() - offset);
  }

  // Write header
  bool ok = true;
  ok =
    packet.writeElement(static_cast<uint32_t>(offset + receive_offset)) == sizeof(uint32_t) && ok;
  ok = packet.writeElement(static_cast<uint16_t>(transfer_count)) == sizeof(uint16_t) && ok;
  ok = packet.writeElement(static_cast<uint8_t>(stream.componentCount())) == sizeof(uint8_t) && ok;
  ok = packet.writeElement(static_cast<uint8_t>(write_as_type)) == sizeof(uint8_t) && ok;

  if (!ok)
  {
    return 0;
  }

  const T *src = stream.ptr<T>(static_cast<size_t>(offset) * stream.elementStride());
  unsigned write_count = 0;
  if (DataBufferPrimitiveTypeInfo<T>::type() == DataBufferPrimitiveTypeInfo<WriteType>::type() &&
      stream.elementStride() == stream.componentCount())
  {
    // We can write the array directly if the T/WriteType types match and the source array is
    // densely packed (element stride matches component count).
    write_count += static_cast<unsigned>(packet.writeArray(
                     reinterpret_cast<const WriteType *>(src),
                     static_cast<size_t>(transfer_count) * stream.componentCount())) /
                   stream.componentCount();
  }
  else
  {
    // We have either a striding mismatch or a type mismatch. Componentwise write
    for (unsigned i = 0; i < transfer_count; ++i)
    {
      unsigned component_write_count = 0;
      for (unsigned j = 0; j < stream.componentCount(); ++j)
      {
        // NOLINTNEXTLINE(bugprone-signed-char-misuse)
        const auto dst_value = static_cast<WriteType>(src[j]);
        component_write_count +=
          static_cast<unsigned>(packet.writeElement(dst_value) / sizeof(dst_value));
      }
      write_count += component_write_count / stream.componentCount();
      src += stream.elementStride();
    }
  }

  if (write_count == transfer_count)
  {
    return write_count;
  }

  // Failed to write the expected number of items.
  return 0;
}

template <typename T>
template <typename FloatType, typename PackedType>
uint32_t DataBufferAffordancesT<T>::writeAsPacked(PacketWriter &packet, uint32_t offset,
                                                  DataStreamType write_as_type, unsigned byte_limit,
                                                  uint32_t receive_offset,
                                                  const FloatType *packet_origin,
                                                  const FloatType quantisation_unit,
                                                  const DataBuffer &stream) const
{
  // packet_origin is used to define the packing origin. That is, items are packed releative to
  // this. quantisation_unit is the divisor used to quantise data. packedType must be either
  // DctPackedFloat16 or DctPackedFloat32 Each component is packed as:
  //    PackedType((vertex[componentIndex] - packedOrigin[componentIndex]) / quantisation_unit)
  const unsigned item_size = static_cast<unsigned>(sizeof(PackedType)) * stream.componentCount();

  // Overhead: account for:
  // - uint32_t offset
  // - uint16_t count
  // - uint8_t element stride
  // - uint8_t data type
  // - FloatType quantisation_unit
  // - FloatType[stream.componentCount()] packet_origin
  const auto overhead =
    int_cast<unsigned>(sizeof(uint32_t) +                             // offset
                       sizeof(uint16_t) +                             // count
                       sizeof(uint8_t) +                              // element stride
                       sizeof(uint8_t) +                              // data type
                       sizeof(quantisation_unit) +                    // quantisation_unit
                       sizeof(FloatType) * stream.componentCount());  // packet_origin

  byte_limit = (byte_limit) ? byte_limit : packet.bytesRemaining();
  uint16_t transfer_count = DataBuffer::estimateTransferCount(item_size, overhead, byte_limit);
  if (transfer_count > stream.count() - offset)
  {
    transfer_count = static_cast<uint16_t>(stream.count() - offset);
  }

  if (transfer_count == 0)
  {
    return 0;
  }

  // Write header
  bool ok = true;
  ok =
    packet.writeElement(static_cast<uint32_t>(offset + receive_offset)) == sizeof(uint32_t) && ok;
  ok = packet.writeElement(static_cast<uint16_t>(transfer_count)) == sizeof(uint16_t) && ok;
  ok = packet.writeElement(static_cast<uint8_t>(stream.componentCount())) == sizeof(uint8_t) && ok;
  ok = packet.writeElement(static_cast<uint8_t>(write_as_type)) == sizeof(uint8_t) && ok;
  const FloatType q_unit{ quantisation_unit };
  ok = packet.writeElement(q_unit) == sizeof(q_unit) && ok;

  if (packet_origin)
  {
    ok = packet.writeArray(packet_origin, stream.componentCount()) == stream.componentCount() && ok;
  }
  else
  {
    const FloatType zero{ 0 };
    for (unsigned i = 0; i < stream.componentCount(); ++i)
    {
      ok = packet.writeElement(zero) == sizeof(FloatType) && ok;
    }
  }

  const T *src = stream.ptr<T>(static_cast<size_t>(offset) * stream.elementStride());
  unsigned write_count = 0;

  const FloatType quantisation_factor = FloatType{ 1 } / FloatType{ quantisation_unit };
  bool item_ok = true;
  for (unsigned i = 0; i < transfer_count; ++i)
  {
    for (unsigned j = 0; j < stream.componentCount(); ++j)
    {
      auto dst_value = static_cast<FloatType>(src[j]);
      if (packet_origin)
      {
        dst_value -= packet_origin[j];
      }
      dst_value *= quantisation_factor;
      const auto packed = static_cast<PackedType>(std::round(dst_value));
      if (std::abs(static_cast<FloatType>(packed) - dst_value) > 1)
      {
        // Failed: quantisation limit reached.
        return 0;
      }
      item_ok = item_ok && packet.writeElement(packed) == sizeof(packed);
    }
    write_count += !!item_ok;
    src += stream.elementStride();
  }

  if (write_count == transfer_count)
  {
    return write_count;
  }

  // Failed to write the expected number of items.
  return 0;
}

template <typename T>
uint32_t DataBufferAffordancesT<T>::read(PacketReader &packet, void **stream_ptr,
                                         unsigned *stream_size, bool *has_ownership,
                                         const DataBuffer &stream) const
{
  TES_UNUSED(stream);
  uint32_t offset;
  uint16_t count;

  bool ok = true;
  ok = packet.readElement(offset) == sizeof(offset) && ok;
  ok = packet.readElement(count) == sizeof(count) && ok;

  if (!ok)
  {
    return 0;
  }

  return read(packet, stream_ptr, stream_size, has_ownership, stream, offset, count);
}

template <typename T>
uint32_t DataBufferAffordancesT<T>::read(PacketReader &packet, void **stream_ptr,
                                         unsigned *stream_size, bool *has_ownership,
                                         const DataBuffer &stream, unsigned offset,
                                         unsigned count) const
{
  TES_UNUSED(stream);
  bool ok = true;
  uint8_t component_count = 0;  // stream.componentCount();;
  uint8_t packet_type = 0;      // DataBufferPrimitiveTypeInfo<T>::type();
  ok = packet.readElement(component_count) == sizeof(component_count) && ok;
  ok = packet.readElement(packet_type) == sizeof(packet_type) && ok;

  if (!ok)
  {
    return 0;
  }

  T *new_ptr = nullptr;
  if (*stream_ptr == nullptr || !*has_ownership || *stream_size < (offset + count))
  {
    // Current stream too small. Reallocate. Note we allocate with the stream's component count.
    new_ptr = new T[static_cast<size_t>(offset + count) * stream.componentCount()];
  }

  if (new_ptr)
  {
    if (*stream_ptr)
    {
      std::copy(static_cast<const T *>(*stream_ptr),
                static_cast<const T *>(*stream_ptr) + (*stream_size) * component_count, new_ptr);
      if (*has_ownership)
      {
        delete[] static_cast<const T *>(*stream_ptr);
      }
    }
    *stream_ptr = new_ptr;
    *stream_size = offset + count;
    *has_ownership = true;
  }

  // We can only read what's available and what we have capacity for. Minimise the component count
  component_count = std::min(component_count, static_cast<uint8_t>(stream.componentCount()));
  switch (packet_type)
  {
  case DctInt8:
    return readAs<int8_t>(packet, offset, count, component_count, stream_ptr);
  case DctUInt8:
    return readAs<uint8_t>(packet, offset, count, component_count, stream_ptr);
  case DctInt16:
    return readAs<int16_t>(packet, offset, count, component_count, stream_ptr);
  case DctUInt16:
    return readAs<uint16_t>(packet, offset, count, component_count, stream_ptr);
  case DctInt32:
    return readAs<int32_t>(packet, offset, count, component_count, stream_ptr);
  case DctUInt32:
    return readAs<uint32_t>(packet, offset, count, component_count, stream_ptr);
  case DctInt64:
    return readAs<int64_t>(packet, offset, count, component_count, stream_ptr);
  case DctUInt64:
    return readAs<uint64_t>(packet, offset, count, component_count, stream_ptr);
  case DctFloat32:
    return readAs<float>(packet, offset, count, component_count, stream_ptr);
  case DctFloat64:
    return readAs<double>(packet, offset, count, component_count, stream_ptr);
  case DctPackedFloat16:
    return readAsPacked<float, int16_t>(packet, offset, count, component_count, stream_ptr);
  case DctPackedFloat32:
    return readAsPacked<double, int32_t>(packet, offset, count, component_count, stream_ptr);
  default:
    // Throw?
    return 0;
  }
}

template <typename DST, typename SRC>
inline size_t affordanceCopy(DST *dst, size_t dst_capacity, const SRC *src,
                             size_t src_component_count, size_t src_element_stride,
                             size_t src_element_count, size_t component_read_count,
                             size_t src_elemment_index, size_t src_component_start)
{
  size_t wrote = 0;
  component_read_count = std::min(dst_capacity, component_read_count);
  for (size_t e = 0; e + src_elemment_index < src_element_count && wrote < component_read_count;
       ++e)
  {
    for (size_t c = src_component_start; c < src_component_count && wrote < component_read_count;
         ++c)
    {
      // NOLINTNEXTLINE(bugprone-signed-char-misuse)
      dst[wrote++] = static_cast<DST>(src[(src_elemment_index + e) * src_element_stride + c]);
    }
    src_component_start = 0;
  }

  return wrote;
}

template <typename T>
size_t DataBufferAffordancesT<T>::get(DataStreamType as_type, size_t element_index,
                                      size_t component_index, size_t component_read_count,
                                      const void *stream, size_t stream_element_count,
                                      size_t stream_component_count, size_t stream_element_stride,
                                      void *dst, size_t dst_capacity) const
{
  if (element_index >= stream_element_count ||
      element_index == stream_element_count && component_index >= stream_component_count ||
      component_read_count == 0)
  {
    return 0;
  }

  // Clamp the read count.
  const size_t element_read_count =
    std::max<size_t>(component_read_count / stream_component_count, 1u);
  component_read_count =
    std::min(component_read_count, element_read_count * stream_component_count);

  const T *src = static_cast<const T *>(stream);
  size_t read_component_count = 0;
  switch (as_type)
  {
  case DctInt8:
    read_component_count = affordanceCopy(
      static_cast<int8_t *>(dst), dst_capacity, src, stream_component_count, stream_element_stride,
      stream_element_count, component_read_count, element_index, component_index);
    break;
  case DctUInt8:
    read_component_count = affordanceCopy(
      static_cast<uint8_t *>(dst), dst_capacity, src, stream_component_count, stream_element_stride,
      stream_element_count, component_read_count, element_index, component_index);
    break;
  case DctInt16:
    read_component_count = affordanceCopy(
      static_cast<int16_t *>(dst), dst_capacity, src, stream_component_count, stream_element_stride,
      stream_element_count, component_read_count, element_index, component_index);
    break;
  case DctUInt16:
    read_component_count =
      affordanceCopy(static_cast<uint16_t *>(dst), dst_capacity, src, stream_component_count,
                     stream_element_stride, stream_element_count, component_read_count,
                     element_index, component_index);
    break;
  case DctInt32:
    read_component_count = affordanceCopy(
      static_cast<int32_t *>(dst), dst_capacity, src, stream_component_count, stream_element_stride,
      stream_element_count, component_read_count, element_index, component_index);
    break;
  case DctUInt32:
    read_component_count =
      affordanceCopy(static_cast<uint32_t *>(dst), dst_capacity, src, stream_component_count,
                     stream_element_stride, stream_element_count, component_read_count,
                     element_index, component_index);
    break;
  case DctInt64:
    read_component_count = affordanceCopy(
      static_cast<int64_t *>(dst), dst_capacity, src, stream_component_count, stream_element_stride,
      stream_element_count, component_read_count, element_index, component_index);
    break;
  case DctUInt64:
    read_component_count =
      affordanceCopy(static_cast<uint64_t *>(dst), dst_capacity, src, stream_component_count,
                     stream_element_stride, stream_element_count, component_read_count,
                     element_index, component_index);
    break;
  case DctFloat32:
    read_component_count = affordanceCopy(
      static_cast<float *>(dst), dst_capacity, src, stream_component_count, stream_element_stride,
      stream_element_count, component_read_count, element_index, component_index);
    break;
  case DctFloat64:
    read_component_count = affordanceCopy(
      static_cast<double *>(dst), dst_capacity, src, stream_component_count, stream_element_stride,
      stream_element_count, component_read_count, element_index, component_index);
    break;
  default:
    TES_THROW(Exception("Unsupported vertex stream read type"), false);
  }

  return read_component_count;
}

template <typename T>
template <typename ReadType>
uint32_t DataBufferAffordancesT<T>::readAs(PacketReader &packet, unsigned offset, unsigned count,
                                           unsigned component_count, void **stream_ptr) const
{
  T *dst = static_cast<T *>(*stream_ptr);
  dst += offset * component_count;
  ReadType read_value = {};

  for (unsigned i = 0; i < count; ++i)
  {
    for (unsigned j = 0; j < component_count; ++j)
    {
      if (packet.readElement(read_value) != sizeof(read_value))
      {
        return 0;
      }
      // NOLINTNEXTLINE(bugprone-signed-char-misuse)
      dst[j] = static_cast<T>(read_value);
    }
    dst += component_count;
  }

  return count;
}

template <typename T>
template <typename FloatType, typename ReadType>
uint32_t DataBufferAffordancesT<T>::readAsPacked(PacketReader &packet, unsigned offset,
                                                 unsigned count, unsigned component_count,
                                                 void **stream_ptr) const
{
  // First read the packing origin.
  std::vector<FloatType> origin(component_count);

  bool ok = true;
  FloatType quantisation_unit = 1;
  ok = packet.readElement(quantisation_unit) == sizeof(quantisation_unit) && ok;
  ok = packet.readArray(origin) == component_count && ok;

  if (!ok)
  {
    return 0;
  }

  T *dst = static_cast<T *>(*stream_ptr);
  dst += offset * component_count;

  for (unsigned i = 0; i < count; ++i)
  {
    for (unsigned j = 0; j < component_count; ++j)
    {
      ReadType read_value;
      if (packet.readElement(read_value) != sizeof(read_value))
      {
        return 0;
      }
      dst[j] = static_cast<T>(read_value * quantisation_unit + origin[j]);
    }
    dst += component_count;
  }

  return count;
}
}  // namespace detail
}  // namespace tes
