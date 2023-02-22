#include "Loop.h"

#include <3esview/Viewer.h>
#include <3esview/data/DataThread.h>

namespace tes::view::command::playback
{
Loop::Loop()
  : Command("loop", Args(true))
{}


bool Loop::checkAdmissible(Viewer &viewer) const
{
  auto stream = viewer.dataThread();
  return stream != nullptr && !stream->isLiveStream();
}


CommandResult Loop::invoke(Viewer &viewer, const ExecInfo &info, const Args &args)
{
  (void)info;
  auto stream = viewer.dataThread();
  if (!stream)
  {
    return { CommandResult::Code::Failed, "Invalid data thread" };
  }
  const auto loop = arg<bool>(0, args);
  stream->setLooping(loop);
  return { CommandResult::Code::Ok };
}
}  // namespace tes::view::command::playback
