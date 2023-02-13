//
// author: Kazys Stepanas
//
#include "Shape.h"

#include <3escore/PacketWriter.h>

#include <cstdio>

namespace tes
{
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
  UpdateMessage update;
  update.id = _data.id;
  update.flags = _data.flags;
  stream.reset(routingId(), UpdateMessage::MessageId);
  return update.write(stream, _attributes);
}


bool Shape::writeDestroy(PacketWriter &stream) const
{
  DestroyMessage destroy;
  destroy.id = _data.id;
  stream.reset(routingId(), DestroyMessage::MessageId);
  return destroy.write(stream);
}


bool Shape::readCreate(PacketReader &stream)
{
  // Assume the routing ID has already been read and resolve.
  return _data.read(stream, _attributes);
}


bool Shape::readUpdate(PacketReader &stream)
{
  UpdateMessage update;
  ObjectAttributesd attrs;
  if (update.read(stream, attrs))
  {
    if ((update.flags & UFUpdateMode) == 0)
    {
      // Full update.
      _attributes = attrs;
    }
    else
    {
      // Partial update.
      if (update.flags & UFPosition)
      {
        memcpy(_attributes.position, attrs.position, sizeof(attrs.position));
      }
      if (update.flags & UFRotation)
      {
        memcpy(_attributes.rotation, attrs.rotation, sizeof(attrs.rotation));
      }
      if (update.flags & UFScale)
      {
        memcpy(_attributes.scale, attrs.scale, sizeof(attrs.scale));
      }
      if (update.flags & UFColour)
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


unsigned Shape::enumerateResources(std::vector<ResourcePtr> &resources) const
{
  TES_UNUSED(resources);
  return 0;
}


std::shared_ptr<Shape> Shape::clone() const
{
  auto copy = std::make_shared<Shape>(_routing_id);
  onClone(*copy);
  return copy;
}


void Shape::onClone(Shape &copy) const
{
  copy._data = _data;
  copy._attributes = _attributes;
}
}  // namespace tes
