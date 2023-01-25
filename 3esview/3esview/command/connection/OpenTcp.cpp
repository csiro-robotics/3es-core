#include "OpenTcp.h"

#include <3esview/Viewer.h>

namespace tes::view::command::connection
{
OpenTcp::OpenTcp()
  : Command("openTcp", Args(std::string(), Viewer::defaultPort(), true))
{}


bool OpenTcp::checkAdmissible(Viewer &viewer) const
{
  return viewer.dataThread() == nullptr;
}


CommandResult OpenTcp::invoke(Viewer &viewer, const ExecInfo &info, const Args &args)
{
  (void)info;
  const auto host = arg<std::string>(0, args);
  const auto port = arg<uint16_t>(1, args);
  const auto allow_reconnect = arg<bool>(2, args);
  if (!viewer.connect(host, port, allow_reconnect))
  {
    return { CommandResult::Code::Failed, "Failed to connect to " + host + ":" + std::to_string(port) };
  }
  return { CommandResult::Code::Ok };
}
}  // namespace tes::view::command::connection
