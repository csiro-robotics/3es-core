#include "Log.h"

#include <array>
#include <iostream>

namespace tes::log
{
namespace
{
LogFunction log_function = defaultLogger;
}  // namespace


void defaultLogger(Level level, const std::string &message)
{
  std::ostream &o = (unsigned(level) <= unsigned(Level::Error)) ? std::cerr : std::cout;
  o << message;
}


LogFunction logger()
{
  return log_function;
}


void setLogger(LogFunction logger)
{
  log_function = logger;
}


const std::string &toString(Level level)
{
  static const std::array<std::string, 5> names = {
    "Fatal", "Error", "Warn", "Info", "Trace",
  };
  return names[unsigned(level)];
}


const std::string &prefix(Level level)
{
  static const std::array<std::string, 5> prefixes = {
    "[Fatal] : ", "[Error] : ", "[Warn] : ", "[Info] : ", "[Trace] : ",
  };
  return prefixes[unsigned(level)];
}


void log(Level level, const std::string &message)
{
  log_function(level, message);
}

void fatal(const std::string &message)
{
  log(Level::Fatal, message);
  throw std::runtime_error(message);
}
}  // namespace tes::log
