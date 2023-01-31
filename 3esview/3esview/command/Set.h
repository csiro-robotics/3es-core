//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_COMMAND_SET_H
#define TES_VIEW_COMMAND_SET_H

#include <3esview/ViewConfig.h>

#include "Command.h"
#include "Shortcut.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace tes::view
{
class Viewer;
}

namespace tes::view::command
{
/// A collection of commands available for execution.
class TES_VIEWER_API Set
{
public:
  /// A command item in the set.
  struct Item
  {
    /// The command.
    std::shared_ptr<Command> command;
    /// The shortcut which can be used to execute the command.
    Shortcut shortcut;
  };

  using Commands = std::unordered_map<std::string, Item>;

  /// Constructor
  Set();
  /// Destructor.
  ~Set();

  /// Register a command.
  /// @param command The command to register.
  /// @param shortcut_sequence Optional shortcut sequence for the command.
  /// @return True on success, false when a command of the given name exists.
  bool registerCommand(std::shared_ptr<Command> command, const Shortcut &shortcut = Shortcut());

  /// Access the internal command set - readonly.
  const Commands &commands() const { return _commands; }

  /// Lookup a command by name.
  /// @param name The command name to lookup.
  /// @return The command item found. On success @c Item::command is non null.
  Item lookupName(const std::string &name) const;

  /// Lookup a command by shortcut squenence.
  /// @param sequence The shortcut sequence to lookup.
  /// @return The command item found. On success @c Item::command is non null.
  Item lookupShortcut(const Shortcut &shortcut) const;

private:
  Commands _commands;
};
}  // namespace tes::view::command

#endif  // TES_VIEW_COMMAND_SET_H
