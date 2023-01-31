#ifndef TES_VIEW_COMMAND_PLAYBACK_SKIP_TO_FRAME_H
#define TES_VIEW_COMMAND_PLAYBACK_SKIP_TO_FRAME_H

#include <3esview/ViewConfig.h>

#include <3esview/command/Command.h>

namespace tes::view::command::playback
{
class TES_VIEWER_API SkipToFrame : public Command
{
public:
  SkipToFrame();

protected:
  bool checkAdmissible(Viewer &viewer) const override;
  CommandResult invoke(Viewer &viewer, const ExecInfo &info, const Args &args) override;
};
}  // namespace tes::view::command::playback

#endif  // TES_VIEW_COMMAND_PLAYBACK_SKIP_TO_FRAME_H
