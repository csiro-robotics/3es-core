#include "SkipBackward.h"

#include <3esview/Viewer.h>
#include <3esview/data/DataThread.h>

namespace tes::view::command::playback
{
SkipBackward::SkipBackward()
  : Command("skipBackward", Args())
{}


bool SkipBackward::checkAdmissible(Viewer &viewer) const
{
  auto stream = viewer.dataThread();
  return stream != nullptr && !stream->isLiveStream() && stream->paused() && stream->currentFrame() > 0;
}


CommandResult SkipBackward::invoke(Viewer &viewer, const ExecInfo &info, const Args &args)
{
  (void)info;
  (void)args;
  auto stream = viewer.dataThread();
  if (!stream)
  {
    return { CommandResult::Code::Failed, "Invalid data thread" };
  }
  stream->pause();
  if (stream->currentFrame() > 0)
  {
    stream->setTargetFrame(0);
  }
  return { CommandResult::Code::Ok };
}
}  // namespace tes::view::command::playback
