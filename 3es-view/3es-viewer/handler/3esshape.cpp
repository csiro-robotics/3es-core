#include "3esshape.h"

#include "painter/3esshapepainter.h"

#include <3esconnection.h>
#include <3escolour.h>
#include <3eslog.h>
#include <3espacketreader.h>

#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Quaternion.h>

#include <cassert>

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


void Shape::beginFrame(unsigned frame_number, unsigned render_mark, bool maintain_transient)
{}


void Shape::endFrame(unsigned frame_number, unsigned render_mark)
{
  (void)frame_number;
  (void)render_mark;
  _painter->endFrame();
}


void Shape::draw(DrawPass pass, unsigned frame_number, unsigned render_mark, const Magnum::Matrix4 &projection_matrix)
{
  (void)frame_number;
  switch (pass)
  {
  case DrawPass::Opaque:
    _painter->drawOpaque(render_mark, projection_matrix);
    break;
  case DrawPass::Transparent:
    _painter->drawTransparent(render_mark, projection_matrix);
    break;
  default:
    break;
  }
}


void Shape::readMessage(PacketReader &reader)
{
  assert(reader.routingId() == routingId());
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
  info = _server_info;
  std::array<uint8_t, (1u << 16u) - 1> buffer;
  PacketWriter writer(buffer.data(), buffer.size());
  CreateMessage create = {};
  ObjectAttributes attrs = {};

  for (auto &&shape : *_painter)
  {
    const auto transform = shape.transform();
    const auto colour = shape.colour();

    create.id = shape.id();
    // @todo create.category = shape.category()
    create.flags = 0;
    if (shape.type() == painter::ShapePainter::Type::Transparent)
    {
      create.flags |= OFTransparent;
    }
    if (shape.type() == painter::ShapePainter::Type::Wireframe)
    {
      create.flags |= OFWire;
    }

    decomposeTransform(transform, attrs);
    attrs.colour = Colour(colour.x(), colour.y(), colour.z(), colour.w()).c;

    // Handle multi shape
    // if (shape.isChain())
    // {
    // }

    writer.reset(routingId(), OIdCreate);
    create.write(writer, attrs);
    writer.finalise();
    out.send(writer.data(), writer.packetSize());
  }
}


Magnum::Matrix4 Shape::composeTransform(const ObjectAttributes &attrs) const
{
  return Magnum::Matrix4::translation(Magnum::Vector3(attrs.position[0], attrs.position[1], attrs.position[2])) *
         Magnum::Matrix4(Magnum::Quaternion(Magnum::Vector3(attrs.rotation[0], attrs.rotation[1], attrs.rotation[2]),
                                            attrs.rotation[3])
                           .toMatrix()) *
         Magnum::Matrix4::scaling(Magnum::Vector3(attrs.scale[0], attrs.scale[1], attrs.scale[2]));
}


void Shape::decomposeTransform(const Magnum::Matrix4 &transform, ObjectAttributes &attrs) const
{
  const auto position = transform[3].xyz();
  attrs.position[0] = position[0];
  attrs.position[1] = position[1];
  attrs.position[2] = position[2];
  const auto rotation = Magnum::Quaternion::fromMatrix(transform.rotation());
  attrs.rotation[0] = rotation.vector()[0];
  attrs.rotation[1] = rotation.vector()[1];
  attrs.rotation[2] = rotation.vector()[2];
  attrs.rotation[3] = rotation.scalar();
  attrs.scale[0] = transform[0].xyz().length();
  attrs.scale[1] = transform[1].xyz().length();
  attrs.scale[2] = transform[2].xyz().length();
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
      _painter->addSubShape(parent_id, draw_type, transform, Magnum::Color4(c.rf(), c.gf(), c.bf(), c.af()));
    }
  }

  return true;
}


bool Shape::handleUpdate(const UpdateMessage &msg, const ObjectAttributes &attrs, PacketReader &reader)
{
  const Id id(msg.id);
  Magnum::Matrix4 transform = {};
  Magnum::Color4 colour = {};
  auto c = Colour(attrs.colour);
  // Handle partial update.
  if (msg.flags & UFUpdateMode)
  {
    ObjectAttributes cur_attrs = {};
    _painter->readProperties(id, false, transform, colour);
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
  const Id id(msg.id);
  return _painter->remove(id);
}


bool Shape::handleData(const DataMessage &msg, PacketReader &reader)
{
  // Not expecting data messages.
  return false;
}
}  // namespace tes::viewer::handler
