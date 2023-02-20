//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_COMMAND_COMMAND_RESULT_H
#define TES_VIEW_COMMAND_COMMAND_RESULT_H

#include <3esview/ViewConfig.h>

#include <string>

namespace tes::view::command
{
/// Object returned by @c Command::execute() used to indicate success and illustrate errors.
class TES_VIEWER_API CommandResult
{
public:
  /// A code used to indicate the result success or failure status.
  enum class Code : int
  {
    /// Indicates success.
    Ok,
    /// Cancellation code: not an error.
    Cancel,
    /// An invalid result indicating no action has been taken. This is the default constructed value
    /// for @c code().
    Invalid,
    /// Indicates the command is current disabled and cannot execute.
    Disabled,
    /// Indicates the command is inadmissible and cannot execute in the current context.
    Inadmissible,
    /// Invalid arguments passed to the command.
    InvalidArguments,
    /// The command has failed.
    Failed
  };

  CommandResult() = default;

  CommandResult(Code code)
    : _code(code)
  {}

  CommandResult(Code code, const std::string &reason)
    : _code(code)
    , _reason(reason)
  {}

  std::string reason() const { return _reason; }
  Code code() const { return _code; }

  operator bool() const { return _code == Code::Ok; }
  bool operator!() const { return _code != Code::Ok; }

private:
  Code _code = Code::Invalid;
  std::string _reason;
};
}  // namespace tes::view::command

#endif  // TES_VIEW_COMMAND_COMMAND_RESULT_H
