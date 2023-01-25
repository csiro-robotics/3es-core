#include "StepForward.h"

#include <3esview/Viewer.h>
#include <3esview/data/DataThread.h>

namespace tes::view::command::playback
{
StepForward::StepForward()
  : Command("stepForward", Args())
{}


bool StepForward::checkAdmissible(Viewer &viewer) const
{
  auto stream = viewer.dataThread();
  return stream != nullptr && !stream->isLiveStream() && stream->paused() &&
         stream->currentFrame() < stream->totalFrames();
}


CommandResult StepForward::invoke(Viewer &viewer, const ExecInfo &info, const Args &args)
{
  (void)info;
  (void)args;
  auto stream = viewer.dataThread();
  if (!stream)
  {
    return { CommandResult::Code::Failed, "Invalid data thread" };
  }
  stream->pause();
  if (stream->currentFrame() < stream->totalFrames())
  {
    stream->setTargetFrame(stream->currentFrame() + 1);
  }
  return { CommandResult::Code::Ok };
}
}  // namespace tes::view::command::playback
