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
{}

template <typename T>
inline VertexStream::VertexStream(const std::vector<T> &v, size_t componentCount, size_t componentStride)
  : _stream(v.data())
  , _componentCount(componentCount)
  , _elementStride(componentStride ? componentStride : componentCount)
  , _basicTypeSize(uint8_t(VertexStreamTypeInfo<T>::size()))
  , _type(VertexStreamTypeInfo<T>::type())
  , _ownPointer(false)
{}

inline VertexStream::VertexStream(const std::vector<Vector3f> &v)
  : _stream(v.data()->v)
  , _componentCount(3)
  , _elementStride(sizeof(Vector3f) / sizeof(float))
  , _basicTypeSize(uint8_t(VertexStreamTypeInfo<float>::size()))
  , _type(DctFloat32)
  , _ownPointer(false)
{}

inline VertexStream::VertexStream(const std::vector<Vector3d> &v)
  : _stream(v.data()->v)
  , _count(unsigned(v.size()))
  , _componentCount(3)
  , _elementStride(sizeof(Vector3d) / sizeof(double))
  , _basicTypeSize(uint8_t(VertexStreamTypeInfo<double>::size()))
  , _type(DctFloat64)
  , _ownPointer(false)
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
  return *this;
}


template <typename T>
inline const T *VertexStream::ptr(size_t element_index)
{
  TES_ASSERT2(VertexStreamTypeInfo<T>::type() == _type, "Element type mismatch");
  return &static_cast<const T *>(_stream)[element_index];
}


template <typename T>
inline const T *VertexStream::ptrAt(size_t element_index)
{
  if (VertexStreamTypeInfo<T>::type() == _type)
  {
    return &static_cast<const T *>(_stream)[element_index];
  }
  return nullptr;
}


inline uint16_t VertexStream::estimateTransferCount(size_t elementSize, unsigned overhead, unsigned byteLimit)
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

}  // namespace tes
