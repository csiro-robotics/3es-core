#include "Speed.h"

#include <3esview/Viewer.h>
#include <3esview/data/DataThread.h>

namespace tes::view::command::playback
{
Speed::Speed()
  : Command("playbackSpeed", Args(1.0f))
{}


bool Speed::checkAdmissible(Viewer &viewer) const
{
  auto stream = viewer.dataThread();
  return stream != nullptr && !stream->isLiveStream();
}


CommandResult Speed::invoke(Viewer &viewer, const ExecInfo &info, const Args &args)
{
  (void)info;
  auto stream = viewer.dataThread();
  if (!stream)
  {
    return { CommandResult::Code::Failed, "Invalid data thread" };
  }

  const float speed = arg<float>(0, args);
  stream->setPlaybackSpeed(speed);
  return { CommandResult::Code::Ok };
}
}  // namespace tes::view::command::playback
