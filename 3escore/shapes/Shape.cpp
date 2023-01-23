//
// author: Kazys Stepanas
//
#include "3esshape.h"

#include <3espacketwriter.h>

#include <cstdio>

using namespace tes;

void Shape::updateFrom(const Shape &other)
{
  _attributes = other._attributes;
}


bool Shape::writeCreate(PacketWriter &stream) const
{
  stream.reset(routingId(), CreateMessage::MessageId);
  return _data.write(stream, _attributes);
}


bool Shape::writeUpdate(PacketWriter &stream) const
{
  UpdateMessage up;
  up.id = _data.id;
  up.flags = _data.flags;
  stream.reset(routingId(), UpdateMessage::MessageId);
  return up.write(stream, _attributes);
}


bool Shape::writeDestroy(PacketWriter &stream) const
{
  DestroyMessage dm;
  dm.id = _data.id;
  stream.reset(routingId(), DestroyMessage::MessageId);
  return dm.write(stream);
}


bool Shape::readCreate(PacketReader &stream)
{
  // Assume the routing ID has already been read and resolve.
  return _data.read(stream, _attributes);
}


bool Shape::readUpdate(PacketReader &stream)
{
  UpdateMessage up;
  ObjectAttributesd attrs;
  if (up.read(stream, attrs))
  {
    if ((up.flags & UFUpdateMode) == 0)
    {
      // Full update.
      _attributes = attrs;
    }
    else
    {
      // Partial update.
      if (up.flags & UFPosition)
      {
        memcpy(_attributes.position, attrs.position, sizeof(attrs.position));
      }
      if (up.flags & UFRotation)
      {
        memcpy(_attributes.rotation, attrs.rotation, sizeof(attrs.rotation));
      }
      if (up.flags & UFScale)
      {
        memcpy(_attributes.scale, attrs.scale, sizeof(attrs.scale));
      }
      if (up.flags & UFColour)
      {
        _attributes.colour = attrs.colour;
      }
    }
    return true;
  }
  return false;
}


bool Shape::readData(PacketReader &stream)
{
  TES_UNUSED(stream);
  return false;
}


unsigned Shape::enumerateResources(const Resource **resources, unsigned capacity, unsigned fetchOffset) const
{
  TES_UNUSED(resources);
  TES_UNUSED(capacity);
  TES_UNUSED(fetchOffset);
  return 0;
}


Shape *Shape::clone() const
{
  Shape *copy = new Shape(_routingId);
  onClone(copy);
  return copy;
}


void Shape::onClone(Shape *copy) const
{
  copy->_data = _data;
  copy->_attributes = _attributes;
}
