#include "MultiShape.h"

#include <3escore/CoreUtil.h>

namespace tes
{
const unsigned MultiShape::BlockCountLimitSingle = 1024u;
const unsigned MultiShape::ShapeCountLimit = 0xffffu;

MultiShape::~MultiShape()
{
  if (_own_shapes)
  {
    for (unsigned i = 0; i < _item_count; ++i)
    {
      delete _shapes[i];
    }

    delete[] _shapes;
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
  ok = stream.writeElement(_item_count) == sizeof(_item_count) && ok;

  // Write the number of items in the creation message.
  const auto creation_block_count = int_cast<uint16_t>(std::min(_item_count, blockCountLimit()));
  ok = stream.writeElement(creation_block_count) == sizeof(creation_block_count) && ok;

  for (unsigned i = 0; ok && i < creation_block_count; ++i)
  {
    ok = _shapes[i]->attributes().write(stream, _shapes[i]->data().flags & OFDoublePrecision) && ok;
  }

  return ok;
}


int MultiShape::writeData(PacketWriter &stream, unsigned &progress_marker) const
{
  if (_item_count <= blockCountLimit())
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

  const unsigned item_offset = (progress_marker + blockCountLimit());
  const unsigned remaining_items = _item_count - item_offset;
  const auto block_count =
    int_cast<uint16_t>(std::min<unsigned>(remaining_items, blockCountLimit()));
  ok = stream.writeElement(block_count) == sizeof(block_count) && ok;

  for (unsigned i = 0; i < block_count; ++i)
  {
    const Shape *shape = _shapes[item_offset + i];
    ok = shape->attributes().write(stream, shape->data().flags & OFDoublePrecision) && ok;
  }

  progress_marker += block_count;

  if (!ok)
  {
    // Error.
    return -1;
  }

  if (remaining_items > block_count)
  {
    // More to come.
    return 1;
  }

  // All done.
  return 0;
}


MultiShape &MultiShape::takeOwnership()
{
  if (!_own_shapes)
  {
    auto **shapes = new Shape *[_item_count];
    for (unsigned i = 0; i < _item_count; ++i)
    {
      shapes[i] = _shapes[i];
    }
    _shapes = shapes;
    _own_shapes = true;
  }

  return *this;
}
}  // namespace tes
