#include "Pause.h"

#include <3esview/Viewer.h>
#include <3esview/data/DataThread.h>

namespace tes::view::command::playback
{
Pause::Pause()
  : Command("pause", Args(true))
{}


bool Pause::checkAdmissible(Viewer &viewer) const
{
  auto stream = viewer.dataThread();
  return stream != nullptr && !stream->isLiveStream();
}


CommandResult Pause::invoke(Viewer &viewer, const ExecInfo &info, const Args &args)
{
  (void)info;
  auto stream = viewer.dataThread();
  if (!stream)
  {
    return { CommandResult::Code::Failed, "Invalid data thread" };
  }
  bool pause = true;
  if (!args.empty())
  {
    pause = args.at<bool>(0);
  }
  else
  {
    pause = !stream->paused();
  }

  if (pause)
  {
    stream->pause();
  }
  else
  {
    stream->unpause();
  }
  return { CommandResult::Code::Ok };
}
}  // namespace tes::view::command::playback
