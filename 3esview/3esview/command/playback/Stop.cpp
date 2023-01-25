#include "Stop.h"

#include <3esview/Viewer.h>
#include <3esview/data/DataThread.h>

namespace tes::view::command::playback
{
Stop::Stop()
  : Command("stop", Args())
{}


bool Stop::checkAdmissible(Viewer &viewer) const
{
  return viewer.dataThread() != nullptr;
}


CommandResult Stop::invoke(Viewer &viewer, const ExecInfo &info, const Args &args)
{
  (void)info;
  (void)args;
  viewer.closeOrDisconnect();
  return { CommandResult::Code::Ok };
}
}  // namespace tes::view::command::playback
