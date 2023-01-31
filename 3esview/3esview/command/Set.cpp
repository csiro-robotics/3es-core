#include "Set.h"

namespace tes::view::command
{
Set::Set() = default;


Set::~Set() = default;


bool Set::registerCommand(std::shared_ptr<Command> command, const Shortcut &shortcut)
{
  if (!command)
  {
    return false;
  }
  auto search = _commands.find(command->name());
  if (search != _commands.end())
  {
    return false;
  }

  _commands.emplace(command->name(), Item{ command, shortcut });
  return true;
}


Set::Item Set::lookupName(const std::string &name) const
{
  auto search = _commands.find(name);
  if (search == _commands.end())
  {
    return {};
  }

  return search->second;
}


Set::Item Set::lookupShortcut(const Shortcut &shortcut) const
{
  for (const auto &[name, item] : _commands)
  {
    if (item.shortcut == shortcut)
    {
      return item;
    }
  }

  return {};
}
}  // namespace tes::view::command
