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
#include "playback/StepBackward.h"
#include "playback/StepForward.h"
#include "playback/Stop.h"

#include <memory>

namespace tes::view::command
{
void registerDefaultCommands(Set &commands)
{
  commands.registerCommand(std::make_shared<connection::Close>(), "ctrl+shift+c");
  commands.registerCommand(std::make_shared<connection::OpenFile>(), "ctrl+o");
  commands.registerCommand(std::make_shared<connection::OpenTcp>(), "ctrl+c");
  commands.registerCommand(std::make_shared<playback::Loop>(), "ctrl+l");
  commands.registerCommand(std::make_shared<playback::Pause>(), "space");
  commands.registerCommand(std::make_shared<playback::SkipBackward>(), "ctrl+.");
  commands.registerCommand(std::make_shared<playback::SkipForward>(), "ctrl+,");
  commands.registerCommand(std::make_shared<playback::SkipToFrame>());
  commands.registerCommand(std::make_shared<playback::StepBackward>(), ",");
  commands.registerCommand(std::make_shared<playback::StepForward>(), ".");
  commands.registerCommand(std::make_shared<playback::Stop>());
}
}  // namespace tes::view::command
