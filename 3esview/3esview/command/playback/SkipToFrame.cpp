#include "SkipToFrame.h"

#include <3esview/Viewer.h>
#include <3esview/data/DataThread.h>

namespace tes::view::command::playback
{
SkipToFrame::SkipToFrame()
  : Command("skipToFrame", Args(unsigned(0u)))
{}


bool SkipToFrame::checkAdmissible(Viewer &viewer) const
{
  auto stream = viewer.dataThread();
  return stream != nullptr && !stream->isLiveStream() && stream->paused();
}


CommandResult SkipToFrame::invoke(Viewer &viewer, const ExecInfo &info, const Args &args)
{
  (void)info;
  auto stream = viewer.dataThread();
  if (!stream)
  {
    return { CommandResult::Code::Failed, "Invalid data thread" };
  }
  stream->pause();
  const auto frame = arg<unsigned>(0, args);
  stream->setTargetFrame(std::min(frame, stream->totalFrames()));
  return { CommandResult::Code::Ok };
}
}  // namespace tes::view::command::playback
