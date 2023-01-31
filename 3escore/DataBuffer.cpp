//
// author: Kazys Stepanas
//
#include "DataBuffer.h"

#include "PacketWriter.h"

#include <array>

using namespace tes;


namespace tes::detail
{
DataBufferAffordances::~DataBufferAffordances() = default;

template class DataBufferAffordancesT<int8_t>;
template class DataBufferAffordancesT<uint8_t>;
template class DataBufferAffordancesT<int16_t>;
template class DataBufferAffordancesT<uint16_t>;
template class DataBufferAffordancesT<int32_t>;
template class DataBufferAffordancesT<uint32_t>;
template class DataBufferAffordancesT<int64_t>;
template class DataBufferAffordancesT<uint64_t>;
template class DataBufferAffordancesT<float>;
template class DataBufferAffordancesT<double>;
}  // namespace tes::detail


void DataBuffer::reset()
{
  if (ownPointer() && _affordances)
  {
    _affordances->release(&_stream, ownPointer());
    _flags &= uint8_t(~Flag::OwnPointer);
  }
}


DataBuffer &DataBuffer::duplicate()
{
  // No need to copy if we already own the _stream.
  if (!ownPointer() && _stream != nullptr && _count > 0)
  {
    _affordances->takeOwnership(&_stream, ownPointer(), *this);
    _flags |= Flag::OwnPointer;
  }
  return *this;
}


unsigned DataBuffer::write(PacketWriter &packet, uint32_t offset, unsigned byteLimit, uint32_t receiveOffset) const
{
  return _affordances->write(packet, offset, type(), byteLimit, receiveOffset, *this);
}


unsigned DataBuffer::writePacked(PacketWriter &packet, uint32_t offset, double quantisation_unit, unsigned byteLimit,
                                 uint32_t receiveOffset) const
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
  return _affordances->write(packet, offset, packed_type, byteLimit, receiveOffset, *this, quantisation_unit);
}


unsigned DataBuffer::read(PacketReader &packet)
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


unsigned DataBuffer::read(PacketReader &packet, unsigned offset, unsigned count)
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
