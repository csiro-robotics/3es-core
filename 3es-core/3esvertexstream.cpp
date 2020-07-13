//
// author: Kazys Stepanas
//
#include "3esvertexstream.h"

#include "3espacketwriter.h"

#include <array>

using namespace tes;


namespace tes::detail
{
VertexStreamAffordances::~VertexStreamAffordances() = default;

template class VertexStreamAffordancesT<int8_t>;
template class VertexStreamAffordancesT<uint8_t>;
template class VertexStreamAffordancesT<int16_t>;
template class VertexStreamAffordancesT<uint16_t>;
template class VertexStreamAffordancesT<int32_t>;
template class VertexStreamAffordancesT<uint32_t>;
template class VertexStreamAffordancesT<int64_t>;
template class VertexStreamAffordancesT<uint64_t>;
template class VertexStreamAffordancesT<float>;
template class VertexStreamAffordancesT<double>;
}  // namespace tes::detail


void VertexStream::reset()
{
  if (ownPointer() && _affordances)
  {
    _affordances->release(&_stream, ownPointer());
    _flags &= uint8_t(~Flag::OwnPointer);
  }
}


void VertexStream::duplicate()
{
  // No need to copy if we already own the _stream.
  if (!ownPointer() && _stream != nullptr && _count > 0)
  {
    _affordances->takeOwnership(&_stream, ownPointer(), *this);
    _flags |= Flag::OwnPointer;
  }
}


unsigned VertexStream::write(PacketWriter &packet, uint32_t offset, unsigned byteLimit) const
{
  return _affordances->write(packet, offset, type(), byteLimit, *this);
}


unsigned VertexStream::writePacked(PacketWriter &packet, uint32_t offset, float quantisation_unit,
                                   unsigned byteLimit) const
{
  DataStreamType packed_type = type();
  switch (packed_type)
  {
  case DctFloat32:
    packed_type = DctPackedFloat16;
    break;
  case DctFloat64:
    packed_type = DctPackedFloat32;
    break;
  default:
    break;
  }
  return _affordances->write(packet, offset, packed_type, byteLimit, *this, quantisation_unit);
}


unsigned VertexStream::read(PacketReader &packet)
{
  void *dst = writePtr();
  bool own_pointer = ownPointer();
  unsigned res = _affordances->read(packet, &dst, &_count, &own_pointer, *this);
  if (_stream != dst)
  {
    // If we reallocated, then we will have allocated more compactly.
    _elementStride = _componentCount;
  }
  _flags |= uint8_t(!!own_pointer * Flag::OwnPointer);
  _stream = dst;
  return res;
}


unsigned VertexStream::read(PacketReader &packet, unsigned offset, unsigned count)
{
  void *dst = writePtr();
  bool own_pointer = ownPointer();
  unsigned res = _affordances->read(packet, &dst, &_count, &own_pointer, *this, offset, count);
  if (_stream != dst)
  {
    // If we reallocated, then we will have allocated more compactly.
    _elementStride = _componentCount;
  }
  _flags |= uint8_t(!!own_pointer * Flag::OwnPointer);
  _stream = dst;
  return res;
}
