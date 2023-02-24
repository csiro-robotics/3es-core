#ifndef TES_VIEW_COMMAND_PLAYBACK_SPEED_H
#define TES_VIEW_COMMAND_PLAYBACK_SPEED_H

#include <3esview/ViewConfig.h>

#include <3esview/command/Command.h>

namespace tes::view::command::playback
{
class TES_VIEWER_API Speed : public Command
{
public:
  Speed();

protected:
  bool checkAdmissible(Viewer &viewer) const override;
  CommandResult invoke(Viewer &viewer, const ExecInfo &info, const Args &args) override;
};
}  // namespace tes::view::command::playback

#endif  // TES_VIEW_COMMAND_PLAYBACK_SPEED_H
