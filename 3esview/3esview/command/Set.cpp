#include "Set.h"

namespace tes::view::command
{
Set::Set() = default;


Set::~Set() = default;


bool Set::registerCommand(std::shared_ptr<Command> command, const std::string &shortcut_sequence)
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

  _commands.emplace(command->name(), Item{ command, shortcut_sequence });
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


Set::Item Set::lookupShortcut(const std::string &sequence) const
{
  for (const auto &[name, item] : _commands)
  {
    if (item.shortcut_sequence == sequence)
    {
      return item;
    }
  }

  return {};
}
}  // namespace tes::view::command
