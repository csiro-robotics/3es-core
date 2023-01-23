#include "Message.h"

#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Quaternion.h>

namespace tes::view::handler
{
Message::Message(uint16_t routing_id, const std::string &name)
  : _routing_id(routing_id)
  , _name(name)
{}


Message::~Message() = default;


void Message::updateServerInfo(const ServerInfoMessage &info)
{
  _server_info = info;
}


Magnum::Matrix4 Message::composeTransform(const ObjectAttributes &attrs)
{
  return Magnum::Matrix4::translation(Magnum::Vector3(attrs.position[0], attrs.position[1], attrs.position[2])) *
         Magnum::Matrix4(Magnum::Quaternion(Magnum::Vector3(attrs.rotation[0], attrs.rotation[1], attrs.rotation[2]),
                                            attrs.rotation[3])
                           .toMatrix()) *
         Magnum::Matrix4::scaling(Magnum::Vector3(attrs.scale[0], attrs.scale[1], attrs.scale[2]));
}


void Message::decomposeTransform(const Magnum::Matrix4 &transform, ObjectAttributes &attrs)
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
}  // namespace tes::view::handler
