#include "Command.h"

namespace tes::view::command
{
Command::Command(const std::string &name, Args &&signature, bool enabled)
  : _name(name)
  , _signature(std::move(signature))
  , _enabled(enabled)
{}


Command::Command(const std::string &name, const Args &signature, bool enabled)
  : _name(name)
  , _signature(signature)
  , _enabled(enabled)
{}


Command::~Command() = default;


bool Command::admissible(Viewer &viewer) const
{
  return checkAdmissible(viewer);
}


CommandResult Command::invoke(Viewer &viewer, const Args &args)
{
  if (!enabled())
  {
    return CommandResult(CommandResult::Code::Disabled, "Command " + name() + " is disabled.");
  }
  if (!admissible(viewer))
  {
    return CommandResult(CommandResult::Code::Inadmissible, "Command " + name() + " is inadmissible.");
  }
  if (!checkSignature(args))
  {
    return CommandResult(CommandResult::Code::InvalidArguments, "Command " + name() + " given invalid arguments.");
  }
  return invoke(viewer, ExecInfo{}, args);
}


bool Command::checkSignature(const Args &args) const
{
  const auto count = std::min(args.count(), _signature.count());
  for (size_t i = 0; i < count; ++i)
  {
    if (args.typeAt(i) != _signature.typeAt(i))
    {
      return false;
    }
  }

  return true;
}
}  // namespace tes::view::command
