//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_COMMAND_DEFAULT_COMMANDS_H
#define TES_VIEW_COMMAND_DEFAULT_COMMANDS_H

#include <3esview/ViewConfig.h>

namespace tes::view
{
class Viewer;
}

namespace tes::view::command
{
class Set;

/// Register the default command set with @p commands.
/// @param commands The command set to register in.
void TES_VIEWER_API registerDefaultCommands(Set &commands);
}  // namespace tes::view::command

#endif  // TES_VIEW_COMMAND_DEFAULT_COMMANDS_H
