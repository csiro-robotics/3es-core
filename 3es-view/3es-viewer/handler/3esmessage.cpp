#include "3esmessage.h"

namespace tes::viewer::handler
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
}  // namespace tes::viewer::handler
