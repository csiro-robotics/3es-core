//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_COMMAND_COMMAND_ARGS_H
#define TES_VIEW_COMMAND_COMMAND_ARGS_H

#include <3esview/ViewConfig.h>

#include <any>
#include <vector>

namespace tes::view::command
{
/// Arguments set for a @c Command invocation.
class TES_VIEWER_API Args
{
public:
  /// Constructor.
  Args() = default;

  /// Copy constructor.
  /// @param other Object to copy.
  Args(const Args &other) = default;

  /// Move constructor.
  /// @param other Object to move.
  Args(Args &&other) = default;

  /// Construct with a single argument.
  /// @tparam T
  /// @param arg
  template <typename... Types>
  Args(Types... args)
  {
    pack(0, args...);
  }

  /// Destructor.
  ~Args() = default;

  /// Copy assignment.
  /// @param other Object to copy.
  Args &operator=(const Args &other) = default;

  /// Move assignment.
  /// @param other Object to move.
  Args &operator=(Args &&other) = default;

  /// Get the number of arguments available.
  /// @return The number of arguments set.
  size_t count() const { return _args.size(); }

  /// Check if the argument set is empty.
  /// @return True when empty.
  bool empty() const { return _args.empty(); }

  /// Get the argument at the given index s the given type.
  ///
  /// Throws @c std::bad_any_cast if the type @p T does not match the stored type.
  ///
  /// Throws @c std::out_of_range if @p index greater than or equal to @c count().
  ///
  /// @tparam T The type to extract as.
  /// @param index The argument index.
  /// @return The argument value.
  template <typename T>
  T at(size_t index) const
  {
    return std::any_cast<T>(_args.at(index));
  }

  /// Query the argument type at the given index.
  ///
  /// Throws @c std::out_of_range if @p index greater than or equal to @c count().
  ///
  /// @param index The index of the argument.
  /// @return The @c typeid of the argument at @p index.
  const std::type_info &typeAt(size_t index) const { return _args.at(index).type(); }

  /// Unpack a single argument into @p arg .
  ///
  /// Throws @c std::bad_any_cast if the type @p T does not match the stored type.
  ///
  /// @tparam T The argument type.
  /// @param arg Where to unpack into.
  /// @return The number of arguments successfully unpacked. In this case, 1 on success, zero on failure.
  template <typename T>
  size_t unpack(T &arg) const
  {
    return getAndUnpack(Args(), 0, arg);
  }

  /// Unpack any number of arguments into @p args.
  ///
  /// This will try unpack up to @c count() arguments.
  ///
  /// Throws @c std::bad_any_cast if any of the template @p Types do not match the stores types.
  ///
  /// @tparam ...Types The template types to unpack.
  /// @param args Where to unapack into.
  /// @return The number of arguments successfully unpacked => `[0, count())`.
  template <typename... Types>
  size_t unpack(Types... args) const
  {
    return getAndUnpack(Args(), 0, &args...);
  }

  /// Unpack a single argument into @p arg and use @p defaults if this object does not have the appropriate number of
  /// arguments.
  ///
  /// Throws @c std::bad_any_cast if the type @p T does not match the stored type.
  ///
  /// @tparam T The argument type.
  /// @param defaults The default argument values.
  /// @param arg Where to unpack into.
  /// @return The number of arguments successfully unpacked. In this case, 1 on success, zero on failure.
  template <typename T>
  size_t unpackDefaulted(const Args &defaults, T &arg) const
  {
    return getAndUnpack(defaults, 0, arg);
  }

  /// Unpack any number of arguments into @p args and use @p defaults if this object does not have the appropriate
  /// number of arguments.
  ///
  /// This will try unpack up to @c count() or @p defaults.count() arguments, whichever is larger.
  ///
  /// Throws @c std::bad_any_cast if any of the template @p Types do not match the stores types.
  ///
  /// @tparam ...Types The template types to unpack.
  /// @param defaults The default argument values.
  /// @param args Where to unapack into.
  /// @return The number of arguments successfully unpacked => `[0, count())`.
  template <typename... Types>
  size_t unpackDefaulted(const Args &defaults, Types... args) const
  {
    return getAndUnpack(defaults, 0, &args...);
  }

private:
  /// Pack @p arg at the given index.
  ///
  /// This is the termination condition for recursive calls to @p pack, thus @p index is expected to be the index of
  /// the last time.
  ///
  /// @tparam T The type of @p arg.
  /// @param index The index to pack at. Generally expected to be the last argument to pack.
  /// @param arg The value to pack.
  /// @return The number of arguments packed => `index + 1`.
  template <typename T>
  size_t pack(size_t index, const T &arg)
  {
    if (_args.size() < index + 1)
    {
      _args.resize(index + 1);
    }
    _args[index] = arg;
    return index + 1;
  }

  /// Pack @p arg at @p index then recursively pack @p args.
  /// @tparam T The type of @p arg.
  /// @tparam ...Types Remaining template types.
  /// @param index The index at which to pack @p arg.
  /// @param arg The argument value to pack at @p index.
  /// @param ...args Additional arguments.
  /// @return The number of arguments packed. Typically this determines the @p count().
  template <typename T, typename... Types>
  size_t pack(size_t index, const T &arg, Types... args)
  {
    if (_args.size() < index + 1)
    {
      _args.resize(index + 1);
    }
    _args[index] = arg;
    return pack(index + 1, &args...);
  }

  /// Unpack argument at @p index into @p arg.
  ///
  /// Uses @p defaults when @p index is out of range for @c count() still in range for @c defaults.count().
  ///
  /// Throws @c std::bad_any_cast if the type @p T does not match the stored type.
  ///
  /// @tparam T The argument type.
  /// @param defaults The default argument values.
  /// @param index The index of the argument.
  /// @param arg Where to unpack into.
  /// @return The number of arguments successfully unpacked so far. In this case, `index + 1` on success, @c count() on
  /// failure unless `index + 1` equals @c count().
  template <typename T>
  size_t getAndUnpack(const Args &defaults, size_t index, T &arg) const
  {
    if (index >= count())
    {
      if (index >= defaults.count())
      {
        return std::max(defaults.count(), count());
      }
      arg = defaults.at(index);
    }
    else
    {
      arg = at(index);
    }
    return index + 1;
  }

  /// Unpack argument at @p index into @p arg then recursively unpack remaining @p args.
  ///
  /// Uses @p defaults when @p index is out of range for @c count() still in range for @c defaults.count().
  ///
  /// Throws @c std::bad_any_cast if the type @p T does not match the stored type.
  ///
  /// @tparam T The argument type.
  /// @param defaults The default argument values.
  /// @param index The index of the argument.
  /// @param arg Where to unpack into.
  /// @return The number of arguments successfully unpacked so far. In this case, `index + 1` on success up to
  /// @c count() on failure.
  template <typename T, typename... Types>
  size_t getAndUnpack(const Args &defaults, size_t index, T &arg, Types... args) const
  {
    if (index >= count())
    {
      if (index >= defaults.count())
      {
        return std::max(defaults.count(), count());
      }
      arg = defaults.at(index);
    }
    else
    {
      arg = at(index);
    }
    return getAndUnpack(index + 1, &args...)
  }

  std::vector<std::any> _args;
};
}  // namespace tes::view::command

#endif  // TES_VIEW_COMMAND_COMMAND_ARGS_H
