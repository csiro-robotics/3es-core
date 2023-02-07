
#ifndef TES_CORE_TIMER_H
#define TES_CORE_TIMER_H

#include "CoreConfig.h"

#include <array>
#include <cinttypes>
#include <string>

namespace tes
{
/// A timing information structure.
struct TES_CORE_API Timing
{
  /// Number of seconds elapsed.
  int64_t s = 0;
  /// Number of milliseconds [0, 1000).
  uint16_t ms = 0;
  /// Number of microseconds [0, 1000).
  uint16_t us = 0;
  /// Number of nanoseconds [0, 1000).
  uint16_t ns = 0;
};

/// A high precision timer implementation. Actual precision is platform dependent.
///
/// On Windows, uses @c QueryPerformanceCounter(). Unix based platforms use @c gettimeofday().
///
/// General usage is to call @c start() at the start of timing and @c mark() at the end. Various
/// elapsed methods may be used to determine the elapsed time.
///
/// A timer may be restarted by calling @c start() and @c mark() again. A timer cannot be paused.
class TES_CORE_API Timer
{
public:
  /// Size of the @c _data array.
  static constexpr unsigned kDataSize = 16;

  /// Constructor. Verifies data size.
  Timer();
  /// Destructor.
  ~Timer();

  /// Starts the timer by recording the current time.
  void start();

  /// Restarts the timer and returns the time elapsed until this call.
  /// @return The time elapsed (ms) from the last @c start() or @c restart() call to this call.
  int64_t restart();

  /// Records the current time as the end time for elapsed calls.
  void mark();

  /// Checks to see if the given time interval has elapsed.
  /// This destroys information recorded on the last @c mark() call.
  /// @param milliseconds The elapsed timer interval to check (milliseconds).
  /// @return True if @p milliseconds has elapsed since @c start() was called.
  [[nodiscard]] bool hasElapsedMs(int64_t milliseconds);

  /// Return the time elapsed now. Similar to calling @c mark() then @c elapsedMS().
  /// This destroys information recorded on the last @c mark() call.
  /// @return The time elapsed to now in milliseconds.
  [[nodiscard]] int64_t elapsedNowMS();

  /// Return the time elapsed now. Similar to calling @c mark() then @c elapsedUS().
  /// This destroys information recorded on the last @c mark() call.
  /// @return The time elapsed to now in microseconds.
  [[nodiscard]] int64_t elapsedNowUS();

  /// Calculates the elapsed time between @c start() and @c mark().
  /// The result is broken up into seconds, ms and us.
  /// @param[out] seconds The number of whole seconds elapsed.
  /// @param[out] milliseconds The number of whole milliseconds elapsed [0, 999].
  /// @param[out] microseconds The number of whole microseconds elapsed [0, 999].
  void elapsed(unsigned &seconds, unsigned &milliseconds, unsigned &microseconds) const;

  /// Calculates the elapsed time between @c start() and @c mark().
  /// The result is broken up into seconds, ms and us.
  /// @param[out] timing Elapsed timing written here.
  void elapsed(Timing &timing) const;

  /// Marks the current end time (@c mark()) and calculates the elapsed time since @c start().
  /// The result is broken up into seconds, ms and us.
  /// @param[out] timing Elapsed timing written here.
  void elapsedNow(Timing &timing);

  /// Splits a nanoseconds value.
  /// @param time_ns The nanoseconds only value to split.
  /// @param[out] timing Elapsed timing written here.
  static void split(int64_t time_ns, Timing &timing);

  /// Splits a microsecond value into seconds and milliseconds.
  /// @param time_us The microseconds only value to split.
  /// @param seconds The number of whole seconds in @p time_us
  /// @param milliseconds The number of whole milliseconds in @p time_us.
  /// @param microseconds The remaining microseconds left in @p time_us. Always < 1000.
  static void split(int64_t time_us, unsigned &seconds, unsigned &milliseconds,
                    unsigned &microseconds);

  /// Determines the elapsed time between recorded start and mark times.
  /// Elapsed time is returned in seconds with a fractional component.
  ///
  /// Undefined before calling @c start() and @c mark().
  /// @return The elapsed time in seconds.
  [[nodiscard]] double elapsedS() const;

  /// Determines the elapsed time between recorded start and mark times.
  /// Elapsed time is returned in whole milliseconds.
  ///
  /// Undefined before calling @c start() and @c mark().
  /// @return The elapsed time in milliseconds.
  [[nodiscard]] int64_t elapsedMS() const;

  /// Determines the elapsed time between recorded start and mark times.
  /// Elapsed time is returned in whole microseconds. Precision may be less than
  /// a microsecond depending on the platform.
  ///
  /// Undefined before calling @c start() and @c mark().
  /// @return The elapsed time in microseconds.
  [[nodiscard]] int64_t elapsedUS() const;

private:
  /// Internal data allocation.
  std::array<char, kDataSize> _data;
};

/// Converts a @c Timer to a time string indicating the elapsed time.
///
/// The string is built differently depending on the amount of time elapsed.
/// For values greater than one second, the display string is formatted:
/// @code{.unparsed}
///   [# day[s],] [# hour[s],] [# minute[s],] [#.#s]
/// @endcode
/// Where '#' is replaced by the appropriate digits. Each element is display
/// only if it is non-zero. Plurals are expressed for values greater than 1.
///
/// Times less than one second and greater than one millisecond are displayed:
/// @code{.unparsed}
///   #.#ms
/// @endcode
///
/// Otherwise, the string is formatted in microseconds:
/// @code{.unparsed}
///   #.#us
/// @endcode
///
/// @param[in,out] buffer The buffer into which to write the time value string.
///   The time value string is written here as detailed above.
///   Buffer overflows are guarded against.
/// @param buffer_length The number of bytes available in @p buffer.
/// @param t Timer to convert to a string.
/// @return A pointer to @c buffer.
std::string TES_CORE_API timeValueString(Timer &t);


/// @overload
std::string TES_CORE_API timeValueString(unsigned s, unsigned ms = 0, unsigned us = 0u);

/// @overload
std::string TES_CORE_API timeValueString(double seconds);
}  // namespace tes

#endif  // TES_CORE_TIMER_H
