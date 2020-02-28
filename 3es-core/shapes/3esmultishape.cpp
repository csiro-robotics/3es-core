#include "3esmultishape.h"

using namespace tes;

MultiShape::~MultiShape()
{
  if (_ownShapes)
  {
    for (unsigned i = 0; i < _itemCount; ++i)
    {
      delete _shapes[i];
    }

    delete [] _shapes;
  }
}


bool MultiShape::isComplex() const
{
  return true;
}


bool MultiShape::writeCreate(PacketWriter &stream) const
{
  if (!Shape::writeCreate(stream))
  {
    return false;
  }

  bool ok = true;
  // Write the total number of items.
  stream.writeElement(_itemCount) == sizeof(_itemCount) && ok;
  // Write the number of items in the creation message.
  const uint16_t creationBlockCount = uint16_t(std::min<unsigned>(_itemCount, BlockCountLimit));
  stream.writeElement(creationBlockCount) == sizeof(creationBlockCount) && ok;

  for (unsigned i = 0; i < creationBlockCount; ++i)
  {
    ok = _shapes[i]->data().attributes.write(stream) && ok;
  }

  return ok;
}


int MultiShape::writeData(PacketWriter &stream, unsigned &progressMarker) const
{
  if (_itemCount <= BlockCountLimit)
  {
    // Nothing more to write. Creation packet was enough.
    return 0;
  }

  DataMessage msg;
  msg.id = _data.id;
  stream.reset(routingId(), DataMessage::MessageId);
  bool ok = msg.write(stream);

  if (!ok)
  {
    return -1;
  }

  const unsigned itemOffset = (progressMarker + BlockCountLimit);
  const unsigned remainingItems = _itemCount - itemOffset;
  const uint16_t blockCount = uint16_t(std::min<unsigned>(remainingItems, BlockCountLimit));
  stream.writeElement(blockCount) == sizeof(blockCount) && ok;

  for (unsigned i = 0; i < blockCount; ++i)
  {
    ok = _shapes[itemOffset + i]->data().attributes.write(stream) && ok;
  }

  progressMarker += blockCount;

  if (!ok)
  {
    // Error.
    return -1;
  }

  if (remainingItems > blockCount)
  {
    // More to come.
    return 1;
  }

  // All done.
  return 0;
}


MultiShape &MultiShape::takeOwnership()
{
  if (!_ownShapes)
  {
    Shape **shapes = new Shape*[_itemCount];
    for (unsigned i = 0; i < _itemCount; ++i)
    {
      shapes[i] = _shapes[i];
    }
    _shapes = shapes;
    _ownShapes = true;
  }

  return *this;
}
