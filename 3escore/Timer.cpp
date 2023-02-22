
#include "Timer.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <sstream>

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif  // !_CRT_SECURE_NO_WARNINGS
#endif  // _MSC_VER

using Clock = std::chrono::high_resolution_clock;
namespace tes
{

struct TimerData
{
  Clock::time_point start_time;
  Clock::time_point end_time;

  void init() { start_time = end_time = Clock::now(); }

  void start() { start_time = Clock::now(); }

  void mark() { end_time = Clock::now(); }

  [[nodiscard]] double elapsedS() const
  {
    const int64_t elapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    const double micro_to_sec = 1e-6;
    return static_cast<double>(elapsed) * micro_to_sec;
  }

  [[nodiscard]] int64_t elapsedMS() const
  {
    const int64_t elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    return elapsed;
  }

  [[nodiscard]] int64_t elapsedUS() const
  {
    const int64_t elapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    return elapsed;
  }

  [[nodiscard]] int64_t elapsedNS() const
  {
    const int64_t elapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    return elapsed;
  }
};


namespace
{
TimerData *getTimerData(std::array<char, Timer::kDataSize> &data)
{
  static_assert(sizeof(TimerData) <= Timer::kDataSize, "Timer data storage is not large enough.");
  auto *td = reinterpret_cast<TimerData *>(data.data());
  return td;
}

const TimerData *getTimerData(const std::array<char, Timer::kDataSize> &data)
{
  static_assert(sizeof(TimerData) <= Timer::kDataSize, "Timer data storage is not large enough.");
  const auto *td = reinterpret_cast<const TimerData *>(data.data());
  return td;
}
}  // namespace


Timer::Timer()
{
  TimerData *td = getTimerData(_data);
  // Placement new
  new (td) TimerData;
  td->init();
}


Timer::~Timer()
{
  TimerData *td = getTimerData(_data);
  td->~TimerData();
}


void Timer::start()
{
  TimerData *td = getTimerData(_data);
  td->start();
}


int64_t Timer::restart()
{
  TimerData *td = getTimerData(_data);
  td->mark();
  const int64_t elapsed_ms = td->elapsedMS();
  td->start();
  return elapsed_ms;
}


void Timer::mark()
{
  TimerData *td = getTimerData(_data);
  td->mark();
}


bool Timer::hasElapsedMs(int64_t milliseconds)
{
  TimerData *td = getTimerData(_data);
  td->mark();
  return td->elapsedMS() >= milliseconds;
}


int64_t Timer::elapsedNowMS()
{
  TimerData *td = getTimerData(_data);
  td->mark();
  return td->elapsedMS();
}


int64_t Timer::elapsedNowUS()
{
  TimerData *td = getTimerData(_data);
  td->mark();
  return td->elapsedUS();
}


void Timer::elapsed(unsigned &seconds, unsigned &milliseconds, unsigned &microseconds) const
{
  split(elapsedUS(), seconds, milliseconds, microseconds);
}


void Timer::elapsed(Timing &timing) const
{
  const TimerData *td = getTimerData(_data);
  const int64_t time_ns = td->elapsedNS();
  split(time_ns, timing);
}


void Timer::elapsedNow(Timing &timing)
{
  TimerData *td = getTimerData(_data);
  td->mark();
  const int64_t time_ns = td->elapsedNS();
  split(time_ns, timing);
}


void Timer::split(int64_t time_ns, Timing &timing)
{
  const int64_t us = time_ns / 1000ll;
  const int64_t ms = us / 1000ll;
  timing.s = time_ns / 1'000'000'000ll;  // NOLINT(readability-magic-numbers)
  timing.ms = static_cast<uint16_t>(ms % 1000ll);
  timing.us = static_cast<uint16_t>(us % 1000ll);
  timing.ns = static_cast<uint16_t>(time_ns % 1000ll);
}


void Timer::split(int64_t time_us, unsigned &seconds, unsigned &milliseconds,
                  unsigned &microseconds)
{
  const int64_t ms = time_us / 1000ll;
  seconds = static_cast<unsigned>(ms / 1000ll);
  milliseconds = static_cast<unsigned>(ms % 1000ll);
  microseconds = static_cast<unsigned>(time_us % 1000ll);
}


double Timer::elapsedS() const
{
  const TimerData *td = getTimerData(_data);
  return td->elapsedS();
}


int64_t Timer::elapsedMS() const
{
  const TimerData *td = getTimerData(_data);
  return td->elapsedMS();
}


int64_t Timer::elapsedUS() const
{
  const TimerData *td = getTimerData(_data);
  return td->elapsedUS();
}


namespace
{
bool addTimeStringUnit(std::stringstream &str, unsigned &seconds, const unsigned seconds_in_unit,
                       const char *unit_name, bool have_previous_unit)
{
  if (seconds >= seconds_in_unit)
  {
    const unsigned units = seconds / seconds_in_unit;
    if (have_previous_unit)
    {
      str << ' ';
    }
    str << units << " " << unit_name << ((units > 1) ? "s" : "");
    seconds = seconds % seconds_in_unit;
    return true;
  }
  return false;
}
}  // namespace


std::string timeValueString(Timer &t)
{
  unsigned s = 0;
  unsigned ms = 0;
  unsigned us = 0;
  t.elapsed(s, ms, us);
  return timeValueString(s, ms, us);
}


std::string timeValueString(unsigned s, unsigned ms, unsigned us)
{
  std::stringstream str;

  const unsigned seconds_in_minute = 60u;
  const unsigned seconds_in_hour = seconds_in_minute * 60u;
  const unsigned seconds_in_day = seconds_in_hour * 24u;

  bool have_large_units = false;
  have_large_units =
    addTimeStringUnit(str, s, seconds_in_day, "day", have_large_units) || have_large_units;
  have_large_units =
    addTimeStringUnit(str, s, seconds_in_hour, "hour", have_large_units) || have_large_units;
  have_large_units =
    addTimeStringUnit(str, s, seconds_in_minute, "minute", have_large_units) || have_large_units;

  if (s)
  {
    if (have_large_units)
    {
      str << ", ";
    }
    str << (static_cast<double>(s) + ms / 1000.0) << "s";
  }
  else if (ms)
  {
    if (have_large_units)
    {
      str << ", ";
    }
    str << (static_cast<double>(ms) + us / 1000.0) << "ms";
  }
  else if (!have_large_units || us)
  {
    if (have_large_units)
    {
      str << ", ";
    }
    str << us << "us";
  }

  return str.str();
}


std::string timeValueString(double seconds)
{
  unsigned s = 0;
  unsigned ms = 0;
  unsigned us = 0;
  s = static_cast<unsigned>(floor(seconds));
  seconds -= floor(seconds);
  seconds *= 1000.0;  // Now in milliseconds.
  ms = static_cast<unsigned>(floor(seconds));
  seconds -= floor(seconds);
  seconds *= 1000.0;  // Now in microseconds.
  us = static_cast<unsigned>(floor(seconds));
  return timeValueString(s, ms, us);
}
}  // namespace tes
