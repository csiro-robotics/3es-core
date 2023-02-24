//
// Author: Kazys Stepanas
//
#include "DefaultCommands.h"

#include "Set.h"

#include "connection/Close.h"
#include "connection/OpenFile.h"
#include "connection/OpenTcp.h"
#include "playback/Loop.h"
#include "playback/Pause.h"
#include "playback/SkipBackward.h"
#include "playback/SkipForward.h"
#include "playback/SkipToFrame.h"
#include "playback/Speed.h"
#include "playback/StepBackward.h"
#include "playback/StepForward.h"
#include "playback/Stop.h"

#include <memory>

namespace tes::view::command
{
void registerDefaultCommands(Set &commands)
{
  commands.registerCommand(std::make_shared<connection::Close>(), Shortcut("ctrl+shift+c"));
  commands.registerCommand(std::make_shared<connection::OpenFile>(), Shortcut("ctrl+o"));
  commands.registerCommand(std::make_shared<connection::OpenTcp>(), Shortcut("ctrl+c"));
  commands.registerCommand(std::make_shared<playback::Loop>(), Shortcut("ctrl+l"));
  commands.registerCommand(std::make_shared<playback::Pause>(), Shortcut("space"));
  commands.registerCommand(std::make_shared<playback::SkipBackward>(), Shortcut("ctrl+."));
  commands.registerCommand(std::make_shared<playback::SkipForward>(), Shortcut("ctrl+,"));
  commands.registerCommand(std::make_shared<playback::SkipToFrame>());
  commands.registerCommand(std::make_shared<playback::Speed>());
  commands.registerCommand(std::make_shared<playback::StepBackward>(), Shortcut(","));
  commands.registerCommand(std::make_shared<playback::StepForward>(), Shortcut("."));
  commands.registerCommand(std::make_shared<playback::Stop>(), Shortcut("ctrl+R"));
}
}  // namespace tes::view::command
