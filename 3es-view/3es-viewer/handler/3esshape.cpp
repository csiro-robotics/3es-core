#include "3esshape.h"

#include "painter/3esshapepainter.h"

#include <3esconnection.h>
#include <3escolour.h>
#include <3esdebug.h>
#include <3eslog.h>
#include <3espacketreader.h>

namespace tes::viewer::handler
{
Shape::Shape(uint16_t routing_id, const std::string &name, std::shared_ptr<painter::ShapePainter> painter)
  : Message(routing_id, name)
  , _painter(std::exchange(painter, nullptr))
{}


void Shape::initialise()
{}


void Shape::reset()
{
  _painter->reset();
}


void Shape::beginFrame(const FrameStamp &stamp)
{
  (void)stamp;
}


void Shape::endFrame(const FrameStamp &stamp)
{
  (void)stamp;
  _painter->commit();
}


void Shape::draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params)
{
  switch (pass)
  {
  case DrawPass::Opaque:
    _painter->drawOpaque(stamp, params.projection_matrix);
    break;
  case DrawPass::Transparent:
    _painter->drawTransparent(stamp, params.projection_matrix);
    break;
  default:
    break;
  }
}


void Shape::readMessage(PacketReader &reader)
{
  TES_ASSERT(reader.routingId() == routingId());
  ObjectAttributes attrs = {};
  bool ok = false;
  bool logged = false;
  switch (reader.messageId())
  {
  case OIdCreate: {
    CreateMessage msg;
    ok = msg.read(reader, attrs) && handleCreate(msg, attrs, reader);
    break;
  }
  case OIdDestroy: {
    DestroyMessage msg;
    ok = msg.read(reader) && handleDestroy(msg, reader);
    break;
  }
  case OIdUpdate: {
    UpdateMessage msg;
    ok = msg.read(reader, attrs) && handleUpdate(msg, attrs, reader);
    break;
  }
  default:
    log::error(name(), " : unhandled shape message type: ", unsigned(reader.messageId()));
    logged = true;
    break;
  }

  if (!ok && !logged)
  {
    log::error(name(), " : failed to decode message type: ", unsigned(reader.messageId()));
  }
}


void Shape::serialise(Connection &out, ServerInfoMessage &info)
{
  (void)info;
  info = _server_info;
  std::array<uint8_t, (1u << 16u) - 1> buffer;
  PacketWriter writer(buffer.data(), uint16_t(buffer.size()));
  CreateMessage create = {};
  ObjectAttributes attrs = {};

  const std::array<painter::ShapePainter::Type, 3> shape_types = { painter::ShapePainter::Type::Solid,
                                                                   painter::ShapePainter::Type::Wireframe,
                                                                   painter::ShapePainter::Type::Transparent };

  for (auto shape_type : shape_types)
  {
    auto end = _painter->end(shape_type);
    for (auto shape = _painter->begin(shape_type); shape != end; ++shape)
    {
      const auto transform = shape->transform;
      const auto colour = shape->colour;

      create.id = shape->id.id();
      create.category = shape->id.category();
      create.flags = 0;
      if (shape_type == painter::ShapePainter::Type::Transparent)
      {
        create.flags |= OFTransparent;
      }
      if (shape_type == painter::ShapePainter::Type::Wireframe)
      {
        create.flags |= OFWire;
      }

      decomposeTransform(transform, attrs);
      attrs.colour = Colour(colour.x(), colour.y(), colour.z(), colour.w()).c;

      writer.reset(routingId(), OIdCreate);
      bool ok = true;
      ok = create.write(writer, attrs) && ok;

      if (shape->child_count)
      {
        // Handle multi shape
        uint32_t child_count = shape->child_count;
        writer.writeElement(child_count) == sizeof(child_count) && ok;

        for (uint32_t i = 0; i < child_count; ++i)
        {
          auto child = shape.getChild(i);

          decomposeTransform(transform, attrs);
          attrs.colour = Colour(colour.x(), colour.y(), colour.z(), colour.w()).c;
          ok = attrs.write(writer) && ok;
        }
      }

      ok = writer.finalise() && ok;
      if (ok)
      {
        out.send(writer.data(), writer.packetSize());
      }
      else
      {
        log::error("Failed to serialise shapes: ", name());
        break;
      }
    }
  }
}


Magnum::Matrix4 Shape::composeTransform(const ObjectAttributes &attrs) const
{
  return Message::composeTransform(attrs);
}


void Shape::decomposeTransform(const Magnum::Matrix4 &transform, ObjectAttributes &attrs) const
{
  Message::decomposeTransform(transform, attrs);
}


bool Shape::handleCreate(const CreateMessage &msg, const ObjectAttributes &attrs, PacketReader &reader)
{
  painter::ShapePainter::Type draw_type = painter::ShapePainter::Type::Solid;

  if (msg.flags & OFTransparent)
  {
    draw_type = painter::ShapePainter::Type::Transparent;
  }
  if (msg.flags & OFWire)
  {
    draw_type = painter::ShapePainter::Type::Wireframe;
  }

  const Id id(msg.id);

  if (msg.flags & OFReplace)
  {
    _painter->remove(id);
  }

  auto transform = composeTransform(attrs);
  auto c = Colour(attrs.colour);
  const auto parent_id = _painter->add(id, draw_type, transform, Magnum::Color4(c.rf(), c.gf(), c.bf(), c.af()));

  if (msg.flags & OFMultiShape)
  {
    // Multi shape message.
    uint32_t shape_count = 0;
    ObjectAttributes multi_attrs = {};
    reader.readElement(shape_count);
    for (unsigned i = 0; i < shape_count; ++i)
    {
      if (!multi_attrs.read(reader, (msg.flags & OFDoublePrecision) != 0))
      {
        log::error(name(), " : failed to read multi shape part");
      }
      transform = composeTransform(attrs);
      c = Colour(attrs.colour);
      _painter->addChild(parent_id, draw_type, transform, Magnum::Color4(c.rf(), c.gf(), c.bf(), c.af()));
    }
  }

  return true;
}


bool Shape::handleUpdate(const UpdateMessage &msg, const ObjectAttributes &attrs, PacketReader &reader)
{
  (void)reader;
  const Id id(msg.id);
  Magnum::Matrix4 transform = {};
  Magnum::Color4 colour = {};
  auto c = Colour(attrs.colour);
  // Handle partial update.
  if (msg.flags & UFUpdateMode)
  {
    ObjectAttributes cur_attrs = {};
    _painter->readShape(id, transform, colour);
    if (msg.flags & (UFPosition | UFRotation | UFScale))
    {
      decomposeTransform(transform, cur_attrs);
      // Decompose the transform.
      if (msg.flags & UFPosition)
      {
        cur_attrs.position[0] = attrs.position[0];
        cur_attrs.position[1] = attrs.position[1];
        cur_attrs.position[2] = attrs.position[2];
      }
      if (msg.flags & UFRotation)
      {
        cur_attrs.rotation[0] = attrs.rotation[0];
        cur_attrs.rotation[1] = attrs.rotation[1];
        cur_attrs.rotation[2] = attrs.rotation[2];
        cur_attrs.rotation[3] = attrs.rotation[3];
      }
      if (msg.flags & UFScale)
      {
        cur_attrs.scale[0] = attrs.scale[0];
        cur_attrs.scale[1] = attrs.scale[1];
        cur_attrs.scale[2] = attrs.scale[2];
      }
      transform = composeTransform(cur_attrs);
    }
    if ((msg.flags & UFColour) == 0)
    {
      c = Colour(cur_attrs.colour);
    }
  }
  else
  {
    transform = composeTransform(attrs);
    colour = Magnum::Color4(c.rf(), c.gf(), c.bf(), c.af());
  }
  _painter->update(id, transform, colour);
  return true;
}


bool Shape::handleDestroy(const DestroyMessage &msg, PacketReader &reader)
{
  (void)msg;
  (void)reader;
  const Id id(msg.id);
  return _painter->remove(id);
}


bool Shape::handleData(const DataMessage &msg, PacketReader &reader)
{
  (void)msg;
  (void)reader;
  // Not expecting data messages.
  return false;
}
}  // namespace tes::viewer::handler
