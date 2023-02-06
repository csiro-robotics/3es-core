#include "Log.h"

#include "CoreUtil.h"

#include <array>
#include <iostream>

namespace tes::log
{
namespace
{
// Clang tidy erroneously considers this a global variable. It should be more of a static variable.
// NOLINTNEXTLINE(readability-identifier-naming)
LogFunction s_log_function;

struct DefaultLogFunctionInit
{
  DefaultLogFunctionInit()
  {
    if (!s_log_function)
    {
      s_log_function = defaultLogger;
    }
  }
};
}  // namespace


void defaultLogger(Level level, const std::string &message)
{
  std::ostream &o =
    (static_cast<unsigned>(level) <= static_cast<unsigned>(Level::Error)) ? std::cerr : std::cout;
  o << message;
}


LogFunction logger()
{
  static const DefaultLogFunctionInit logger_init;
  return s_log_function;
}


void setLogger(LogFunction logger)
{
  s_log_function = std::move(logger);
}


const std::string &toString(Level level)
{
  static const std::array<std::string, 5> names = {
    "Fatal", "Error", "Warn", "Info", "Trace",
  };
  return names[static_cast<unsigned>(level)];
}


const std::string &prefix(Level level)
{
  static const std::array<std::string, 5> prefixes = {
    "[Fatal] : ", "[Error] : ", "[Warn] : ", "[Info] : ", "[Trace] : ",
  };
  return prefixes[static_cast<unsigned>(level)];
}


void log(Level level, const std::string &message)
{
  s_log_function(level, message);
}

void fatal(const std::string &message)
{
  log(Level::Fatal, message);
  throw std::runtime_error(message);
}
}  // namespace tes::log
