//
// author: Kazys Stepanas
//

namespace tes
{
unsigned writeStreamAsUInt32(PacketWriter &packet, const VertexStream &stream, uint32_t offset, unsigned byteLimit);


inline VertexStream::VertexStream()
  : _stream(nullptr)
  , _count(0)
  , _componentCount(1)
  , _elementStride(1)
  , _basicTypeSize(0)
  , _type(DctNone)
  , _ownPointer(false)
  , _affordances(nullptr)
{}

template <typename T>
inline VertexStream::VertexStream(const T *v, size_t count, size_t componentCount, size_t componentStride,
                                  bool ownPointer)
  : _stream(v)
  , _count(count)
  , _componentCount(componentCount)
  , _elementStride(componentStride ? componentStride : componentCount)
  , _basicTypeSize(uint8_t(VertexStreamTypeInfo<T>::size()))
  , _type(VertexStreamTypeInfo<T>::type())
  , _ownPointer(ownPointer)
  , _affordances(detail::VertexStreamAffordancesT<T>::instance())
{}

template <typename T>
inline VertexStream::VertexStream(const std::vector<T> &v, size_t componentCount, size_t componentStride)
  : _stream(v.data())
  , _componentCount(componentCount)
  , _elementStride(componentStride ? componentStride : componentCount)
  , _basicTypeSize(uint8_t(VertexStreamTypeInfo<T>::size()))
  , _type(VertexStreamTypeInfo<T>::type())
  , _ownPointer(false)
  , _affordances(detail::VertexStreamAffordancesT<T>::instance())
{}

inline VertexStream::VertexStream(const std::vector<Vector3f> &v)
  : _stream(v.data()->v)
  , _componentCount(3)
  , _elementStride(sizeof(Vector3f) / sizeof(float))
  , _basicTypeSize(uint8_t(VertexStreamTypeInfo<float>::size()))
  , _type(DctFloat32)
  , _ownPointer(false)
  , _affordances(detail::VertexStreamAffordancesT<float>::instance())
{}

inline VertexStream::VertexStream(const std::vector<Vector3d> &v)
  : _stream(v.data()->v)
  , _count(unsigned(v.size()))
  , _componentCount(3)
  , _elementStride(sizeof(Vector3d) / sizeof(double))
  , _basicTypeSize(uint8_t(VertexStreamTypeInfo<double>::size()))
  , _type(DctFloat64)
  , _ownPointer(false)
  , _affordances(detail::VertexStreamAffordancesT<double>::instance())
{}

inline VertexStream::VertexStream(VertexStream &&other)
  : VertexStream()
{
  *this = std::move(other);
}

inline VertexStream::VertexStream(const VertexStream &other)
  : _stream(other._stream)
  , _componentCount(other._componentCount)
  , _elementStride(other._elementStride)
  , _basicTypeSize(other._basicTypeSize)
  , _type(other._type)
  , _ownPointer(false)  // Copy assignment. We do not own the pointer.
  , _affordances(other._affordances)
{}

inline VertexStream::~VertexStream()
{
  reset();
}

template <typename T>
inline void VertexStream::set(const T *v, size_t count, size_t componentCount, size_t componentStride, bool ownPointer)
{
  *this = std::move(VertexStream(v, count, componentCount, componentStride, ownPointer));
}

template <typename T>
inline void VertexStream::set(const std::vector<T> &v, size_t componentCount, size_t componentStride)
{
  *this = std::move(VertexStream(v, componentCount, componentStride));
}

inline void VertexStream::set(const std::vector<Vector3f> &v)
{
  *this = std::move(VertexStream(v));
}
inline void VertexStream::set(const std::vector<Vector3d> &v)
{
  *this = std::move(VertexStream(v));
}

inline VertexStream &VertexStream::operator=(VertexStream &&other)
{
  std::swap(_stream, other._stream);
  std::swap(_count, other._count);
  std::swap(_componentCount, other._componentCount);
  std::swap(_elementStride, other._elementStride);
  std::swap(_basicTypeSize, other._basicTypeSize);
  std::swap(_type, other._type);
  std::swap(_ownPointer, other._ownPointer);
  std::swap(_affordances, other._affordances);
  return *this;
}

inline VertexStream &VertexStream::operator=(const VertexStream &other)
{
  reset();
  _stream = other._stream;
  _componentCount = other._componentCount;
  _elementStride = other._elementStride;
  _basicTypeSize = other._basicTypeSize;
  _type = other._type;
  _ownPointer = false;
  _affordances = other._affordances;
  return *this;
}


template <typename T>
inline const T *VertexStream::ptr(size_t element_index) const
{
  TES_ASSERT2(VertexStreamTypeInfo<T>::type() == _type, "Element type mismatch");
  return &static_cast<const T *>(_stream)[element_index];
}


template <typename T>
inline const T *VertexStream::ptrAt(size_t element_index) const
{
  if (VertexStreamTypeInfo<T>::type() == _type)
  {
    return &static_cast<const T *>(_stream)[element_index];
  }
  return nullptr;
}


inline uint16_t VertexStream::estimateTransferCount(size_t elementSize, unsigned overhead, unsigned byteLimit = 0xff00u)
{
  // FIXME: Without additional overhead padding I was getting missing messages at the client with
  // no obvious error path. For this reason, we use 0xff00u, instead of 0xffffu
  //                                    packet header           message                         crc
  const size_t maxTransfer =
    (0xff00u - (sizeof(PacketHeader) + overhead + sizeof(PacketWriter::CrcType))) / elementSize;
  size_t count = byteLimit ? byteLimit / elementSize : maxTransfer;
  if (count > maxTransfer)
  {
    count = maxTransfer;
  }

  return uint16_t(count);
}


namespace detail
{
template <typename T>
VertexStreamAffordances *VertexStreamAffordancesT<T>::instance()
{
  static VertexStreamAffordancesT<T> obj;
  return &obj;
}

template <typename T>
void VertexStreamAffordancesT<T>::release(const void **stream_ptr, bool *has_ownership) const
{
  if (*has_ownership)
  {
    delete reinterpret_cast<const T *>(*stream_ptr);
    *stream_ptr = nullptr;
    *has_ownership = false;
  }
}


template <typename T>
void VertexStreamAffordancesT<T>::takeOwnership(const void **stream_ptr, bool *has_ownership,
                                                const VertexStream &stream) const
{
  if (*has_ownership || *stream_ptr == nullptr)
  {
    // Already has ownership
    return;
  }

  // Allocate a new array.
  T *new_array = new T[stream.count() * stream.elementStride()];
  std::copy(*stream_ptr, *stream_ptr + stream.count() * stream.elementStride(), new_array);
  *stream_ptr = new_array;
  *has_ownership = true;
}


template <typename T>
uint32_t VertexStreamAffordancesT<T>::write(PacketWriter &packet, uint32_t offset, DataStreamType write_as_type,
                                            const VertexStream &stream) const
{
  switch (write_as_type)
  {
  case DctInt8:
    return writeAs<int8_t>(packet, offset, write_as_type, stream);
  case DctUInt8:
    return writeAs<uint8_t>(packet, offset, write_as_type, stream);
  case DctInt16:
    return writeAs<int16_t>(packet, offset, write_as_type, stream);
  case DctUInt16:
    return writeAs<uint16_t>(packet, offset, write_as_type, stream);
  case DctInt32:
    return writeAs<int32_t>(packet, offset, write_as_type, stream);
  case DctUInt32:
    return writeAs<uint32_t>(packet, offset, write_as_type, stream);
  case DctFloat32:
    return writeAs<float>(packet, offset, write_as_type, stream);
  case DctFloat64:
    return writeAs<double>(packet, offset, write_as_type, stream);
  case DctPackedFloat16:
    return writeAsPacked<float, int16_t>(packet, offset, write_as_type, stream);
  case DctPackedFloat32:
    return writeAsPacked<double, int32_t>(packet, offset, write_as_type, stream);
  default:
    // Throw?
    return 0;
  }
}


template <typename T, typename WriteType>
uint32_t VertexStreamAffordancesT<T>::writeAs(PacketWriter &packet, uint32_t offset, DataStreamType write_as_type,
                                              const VertexStream &stream) const
{
  const unsigned itemSize = sizeof(WriteType) * stream.componentCount();
  unsigned writeSize = 0u;

  // Overhead: account for:
  // - uint32_t offset
  // - uint16_t count
  // - uint8_t component count
  // - uint8_t data type
  const unsigned overhead = sizeof(uint32_t) +  // offset
                            sizeof(uint16_t) +  // count
                            sizeof(uint8_t) +   // element stride
                            sizeof(uint8_t);    // data type;

  uint16_t transferCount =
    VertexStream::estimateTransferCount(itemSize, overhead + packet.tell(), packet.bytesRemaining());
  if (transferCount > stream.count() - offset)
  {
    transferCount = uint16_t(stream.count() - offset);
  }

  // Write header
  bool ok = true;
  ok = packet.writeElement(uint32_t(offset)) == sizeof(uint32_t) && ok;
  ok = packet.writeElement(uint16_t(transferCount)) == sizeof(uint16_t) && ok;
  ok = packet.writeElement(uint8_t(stream.componentCount())) == sizeof(uint8_t) && ok;
  ok = packet.writeElement(uint8_t(write_as_type)) == sizeof(uint8_t) && ok;

  if (!ok)
  {
    return 0;
  }

  const T *src = stream.ptr<T>(offset * stream.elementStride());

  if (VertexStreamTypeInfo<T>::type() == VertexStreamTypeInfo<WriteType>::type() &&
      stream.elementStride() == stream.componentCount())
  {
    // We can write the array directly if the T/WriteType types match and the source array is densely packed (element
    // stride matches component count).
    writeCount +=
      unsigned(packet.writeArray(reinterpret_cast<const WriteType *>(src), transferCount * stream.componentCount()));
  }
  else
  {
    // We have either a striding mismatch or a type mismatch. Componentwise write
    for (unsigned i = 0; transferCount; ++i)
    {
      for (unsigned j = 0; j < stream.componentCount(); ++j)
      {
        const WriteType dstValue = static_cast<WriteType>(src[j]);
        writeCount += unsigned(packet.writeElement(dstValue)) / sizeof(dstValue);
      }
      src += stream.elementStride();
    }
  }

  if (writeCount == transferCount)
  {
    return writeCount;
  }

  // Failed to write the expected number of items.
  return 0;
}


template <typename T, typename FloatType, typename PackedType>
uint32_t VertexStreamAffordancesT<T>::writeAsPacked(PacketWriter &packet, uint32_t offset, DataStreamType write_as_type,
                                                    const FloatType *packingOrigin, const float quantisationUnit,
                                                    const VertexStream &stream) const
{
  // packingOrigin is used to define the packing origin. That is, items are packed releative to this.
  // quantisationUnit is the divisor used to quantise data.
  // packedType must be either DctPackedFloat16 or DctPackedFloat32
  // Each component is packed as:
  //    PackedType((vertex[componentIndex] - packedOrigin[componentIndex]) / quantisationUnit)
  const unsigned itemSize = sizeof(PackedType) * stream.componentCount();
  unsigned writeSize = 0u;

  // Overhead: account for:
  // - uint32_t offset
  // - uint16_t count
  // - uint8_t element stride
  // - uint8_t data type
  // - FloatType[stream.componentCount()] packingOrigin
  // - float32 quantisationUnit
  const unsigned overhead = sizeof(uint32_t) +                             // offset
                            sizeof(uint16_t) +                             // count
                            sizeof(uint8_t) +                              // element stride
                            sizeof(uint8_t) +                              // data type
                            sizeof(FloatType) * stream.componentCount() +  // packingOrigin
                            sizeof(quantisationUnit);                      // quantisationUnit

  uint16_t transferCount =
    VertexStream::estimateTransferCount(itemSize, overhead + packet.tell(), packet.bytesRemaining());
  if (transferCount > stream.count() - offset)
  {
    transferCount = uint16_t(stream.count() - offset);
  }

  if (transferCount == 0)
  {
    return 0;
  }

  // Write header
  bool ok = true;
  ok = packet.writeElement(uint32_t(offset)) == sizeof(uint32_t) && ok;
  ok = packet.writeElement(uint16_t(transferCount)) == sizeof(uint16_t) && ok;
  ok = packet.writeElement(uint8_t(stream.componentCount())) == sizeof(uint8_t) && ok;
  ok = packet.writeElement(uint8_t(packedType)) == sizeof(uint8_t) && ok;
  ok = packet.writeElement(quantisationUnit) == sizeof(quantisationUnit) && ok;

  if (packingOrigin)
  {
    ok = packet.writeArray(packingOrigin, stream.componentCount()) == stream.componentCount() && ok;
  }
  else
  {
    const FloatType zero{ 0 };
    for (unsigned i = 0; i < stream.componentCount(); ++i)
    {
      ok = packet.writeElement(zero) == sizeof(FloatType) && ok;
    }
  }

  const T *src = stream.ptr<T>(offset * stream.elementStride());
  unsigned writeCount = 0;

  const FloatType quantisationFactor = FloatType{ 1 } / FloatType{ quantisationUnit };
  for (unsigned i = 0; transferCount; ++i)
  {
    for (unsigned j = 0; j < stream.componentCount(); ++j)
    {
      FloatType dstValue = static_cast<FloatType>(src[j]);
      if (packingOrigin)
      {
        dstValue -= packingOrigin[j];
      }
      dstValue *= quantisationFactor;
      const PackedType packed = PackedType(std::round(dstValue));
      if (std::abs(FloatType(packet) - dstValue) > 1)
      {
        // Failed: quantisation limit reached.
        return 0;
      }
      writeCount += unsigned(packet.writeElement(packed)) / sizeof(packed);
    }
    src += stream.elementStride();
  }

  if (writeCount == transferCount)
  {
    return writeCount;
  }

  // Failed to write the expected number of items.
  return 0;
}

template <typename T>
uint32_t VertexStreamAffordancesT<T>::read(PacketReader &packet, void **stream_ptr, bool *has_ownership) const
{
  uint32_t offset;
  uint16_t count;
  uint8_t componentCount;
  uint8_t packetType;

  bool ok = true;
  ok = packet.readElement(offset) == sizeof(offset) && ok;
  ok = packet.readElement(count) == sizeof(count) && ok;
  ok = packet.readElement(componentCount) == sizeof(componentCount) && ok;
  ok = packet.readElement(packetType) == sizeof(packetType) && ok;

  if (!ok)
  {
    return 0;
  }

  if (*stream_ptr == nullptr || !*has_owership)
  {
    *stream_ptr = new T[count * componentCount];
    *has_ownership = true;
  }

  switch (packetType)
  {
  case DctInt8:
    return readAs<int8_t>(packet, offset, count, componentCount, packetType, stream_ptr);
  case DctUInt8:
    return readAs<uint8_t>(packet, offset, count, componentCount, packetType, stream_ptr);
  case DctInt16:
    return readAs<int16_t>(packet, offset, count, componentCount, packetType, stream_ptr);
  case DctUInt16:
    return readAs<uint16_t>(packet, offset, count, componentCount, packetType, stream_ptr);
  case DctInt32:
    return readAs<int32_t>(packet, offset, count, componentCount, packetType, stream_ptr);
  case DctUInt32:
    return readAs<uint32_t>(packet, offset, count, componentCount, packetType, stream_ptr);
  case DctFloat32:
    return readAs<float>(packet, offset, count, componentCount, packetType, stream_ptr);
  case DctFloat64:
    return readAs<double>(packet, offset, count, componentCount, packetType, stream_ptr);
  case DctPackedFloat16:
    return readAsPacked<float, int16_t>(packet, offset, count, componentCount, packetType, stream_ptr);
  case DctPackedFloat32:
    return readAsPacked<double, int32_t>(packet, offset, count, componentCount, packetType, stream_ptr);
  default:
    // Throw?
    return 0;
  }
}


template <typename T, typename ReadType>
uint32_t VertexStreamAffordancesT<T>::readAs(PacketReader &packet, unsigned offset, unsigned count,
                                             unsigned componentCount, void **stream_ptr) const
{
  T *dst = static_cast<T *>(*stream_ptr);
  dst += offset * componentCount;

  for (unsigned i = 0; i < count; ++i)
  {
    for (unsigned j = 0; j < componentCount; ++j)
    {
      ReadType readValue;
      if (packet.readElement(&readValue) != sizeof(readValue))
      {
        return 0;
      }
      dst[j] = static_cast<T>(readValue);
    }
    dst += componentCount;
  }

  return count;
}


template <typename T, typename FloatType, typename ReadType>
uint32_t VertexStreamAffordancesT<T>::readAsPacked(PacketReader &packet, unsigned offset, unsigned count,
                                                   unsigned componentCount, void **stream_ptr) const
{
  // First read the packing origin.
  std::unique_ptr<FloatType[]> origin = std::make_unique<FloatType[]>(componentCount);
  // std::vector<FloatType> origin(componentCount);

  bool ok = true;
  FloatType qunatisationUnit = 1;
  ok = packet.readElement(qunatisationUnit) == sizeof(qunatisationUnit) && ok;
  ok = packet.readArray(origin.get(), componentCount) == componentCount && ok;

  if (!ok)
  {
    return 0;
  }

  T *dst = static_cast<T *>(*stream_ptr);
  dst += offset * componentCount;

  for (unsigned i = 0; i < count; ++i)
  {
    for (unsigned j = 0; j < componentCount; ++j)
    {
      ReadType readValue;
      if (packet.readElement(&readValue) != sizeof(readValue))
      {
        return 0;
      }
      dst[j] = static_cast<T>(readValue * qunatisationUnit + origin[j]);
    }
    dst += componentCount;
  }

  return count;
}
}  // namespace detail

}  // namespace tes
