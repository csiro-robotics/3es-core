#include "Close.h"

#include <3esview/Viewer.h>

namespace tes::view::command::connection
{
Close::Close()
  : Command("close", Args())
{}


bool Close::checkAdmissible(Viewer &viewer) const
{
  return viewer.dataThread() != nullptr;
}


CommandResult Close::invoke(Viewer &viewer, const ExecInfo &info, const Args &args)
{
  (void)info;
  (void)args;
  viewer.closeOrDisconnect();
  return { CommandResult::Code::Ok };
}
}  // namespace tes::view::command::connection
