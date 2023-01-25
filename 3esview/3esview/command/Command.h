//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_COMMAND_COMMAND_H
#define TES_VIEW_COMMAND_COMMAND_H

#include <3esview/ViewConfig.h>

#include "Args.h"
#include "CommandResult.h"

namespace tes::view
{
class Viewer;
}

namespace tes::view::command
{
/// The base class for a UI command.
///
/// Each command has a name, and execute function and can be enabled/disabled.
///
/// Possible other core features:
///
/// - Context identifying when it's valid to use this command.
/// - Asynchronous execution (opt in). Needs a shared command thread to run on.
class TES_VIEWER_API Command
{
public:
  /// Constructor.
  /// @param name The command name. Must be unique.
  /// @param signature A set of arguments which defines the command signature. This also determines the default
  /// argument values for the command when arguments are omitted.
  /// @param enabled The initial enabled status for the command.
  Command(const std::string &name, Args &&signature, bool enabled = true);
  /// @overload
  Command(const std::string &name, const Args &signature, bool enabled = true);
  /// Destructor.
  virtual ~Command();

  /// Get the name of the command. Command names must be unique.
  /// @return The command name.
  const std::string &name() const { return _name; }

  /// Get the command argument signature and default values.
  /// @return The argument signature.
  const Args &signature() const { return _signature; }

  /// Check if the command is explicitly enabled or not.
  /// @return True when enabled.
  bool enabled() const { return _enabled; }

  /// Set the @c enabled() status of the command.
  /// @return True when enabled
  void setEnabled(bool enable) { _enabled = enable; }

  /// Checks if the command is currently admissible for the given @p viewer and can be executed.
  ///
  /// This does not check if the command is enabled.
  ///
  /// @param viewer The viewer to execute on.
  /// @return True when admissible.
  bool admissible(Viewer &viewer) const;

  /// Synchronously execute the command.
  ///
  /// This checks the command is both @c enabled() and @c admissible() returning appropriate result codes on failure.
  ///
  /// @param args The command arguments to pass.
  /// @return A @c CommandResult indicating the execution results.
  CommandResult invoke(Viewer &viewer, const Args &args);

  // Speculative:
  // std::future<CommandResult> executeAsync();

  /// Check the call signature for @p args matches @c signature().
  ///
  /// This checks that the types of each item in @p args matches that of @c signature() up to @c signature().count().
  /// Extraneous arguments are ignored.
  ///
  /// @param args Arguments to check against @c signature().
  /// @return True if the corresponding types match.
  bool checkSignature(const Args &args) const;

protected:
  struct ExecInfo
  {
  };

  /// Check if the command is currently admissible.
  /// @param viewer The viewer to execute on.
  /// @return True if admissible.
  virtual bool checkAdmissible(Viewer &viewer) const = 0;

  /// Do the work to execute the command.
  ///
  /// The @p args parameter provides the arguments passed to the invocation. This may be fewer than those provided by
  /// @c signature(). The typical way to unpack arguments with provision for using defaults from @c signature() is
  /// to unpack arguments as follows:
  ///
  /// @code
  /// CommandResult MyCommand1::doWork(const ExecInfo &info, const Args &args)
  /// {
  ///   // Argument unpacking example 1.
  ///   int arg0 = 0;
  ///   float arg1 = 0;
  ///   std::string arg2;
  ///   args.unpack(signature(), arg0, arg1, arg2);
  /// }
  ///
  /// CommandResult MyCommand2::doWork(const ExecInfo &info, const Args &args)
  /// {
  ///   // Argument unpacking example 2.
  ///   auto arg0 = arg<int>(0, args);
  ///   auto arg1 = arg<float>(1, args);
  ///   auto arg2 = arg<std::string>(2, args);
  /// }
  /// @endcode
  ///
  /// @param viewer The viewer to execute on.
  /// @param info Reserved for future use to provide information about the execution context (e.g., async or not).
  /// @param args The arguments passed for execution. See remarks on retrieving arguments.
  /// @return A result indicating the success or failure result. On failure the @c CommandResult::reason() must be
  /// set.
  virtual CommandResult invoke(Viewer &viewer, const ExecInfo &info, const Args &args) = 0;

  /// Get the value of the argument at @p index falling back to defaults from @c signature() if required.
  /// @tparam T The type to retrieve as. Must match the given @p args type or @c std::bad_any_cast is thrown.
  /// @param index Index of the argument to retrieve. Must be in range for either @p args or @c signature() or
  ///   @c std::out_of_range is thrown.
  /// @param args The arguments passed for this invocation.
  /// @return The argument value.
  template <typename T>
  T arg(size_t index, const Args &args) const
  {
    if (index < args.count())
    {
      return args.at<T>(index);
    }
    return signature().at<T>(index);
  }

private:
  std::string _name;
  const Args _signature;
  bool _enabled = true;
};
}  // namespace tes::view::command

#endif  // TES_VIEW_COMMAND_COMMAND_H
