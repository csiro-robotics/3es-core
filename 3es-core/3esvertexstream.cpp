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
template class VertexStreamAffordancesT<float>;
template class VertexStreamAffordancesT<double>;
}  // namespace tes::detail


void VertexStream::reset()
{
  if (_ownPointer && _affordances)
  {
    _affordances->release(&_stream, &_ownPointer);
  }
}


void VertexStream::duplicateArray()
{
  // No need to copy if we already own the _stream.
  if (!_ownPointer && _stream != nullptr && _count > 0)
  {
    _affordances->takeOwnership(&_stream, _ & ownPointer, *this);
  }
}


unsigned VertexStream::write(PacketWriter &packet, uint32_t offset)
{
  return _affordances->write(packet, offset, *this);
}


unsigned VertexStream::read(PacketReader &packet)
{
  void *dst = writePtr();
  unsigned res = _affordances->read(packet, &dst, &_ownPointer, *this);
  _stream = dst;
  return res;
}
