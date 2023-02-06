//
// Author: Kazys Stepanas
//
#ifndef TES_CORE_THROW_H
#define TES_CORE_THROW_H

#include "CoreConfig.h"

#if TES_EXCEPTIONS
// Return statement present to prevent compilation errors when switching TES_EXCEPTIONS.
#define TES_THROW(exc, return_value)            \
  {                                             \
    tes::logException(exc, __FILE__, __LINE__); \
    throw(exc);                                 \
  }
#define TES_THROW2(exc)                         \
  {                                             \
    tes::logException(exc, __FILE__, __LINE__); \
    throw(exc);                                 \
  }
#else  // TES_EXCEPTIONS
#define TES_THROW(exc, return_value)            \
  {                                             \
    tes::logException(exc, __FILE__, __LINE__); \
    return return_value;                        \
  }

#define TES_THROW2(exc)                         \
  {                                             \
    tes::logException(exc, __FILE__, __LINE__); \
    return;                                     \
  }
#endif  // TES_EXCEPTIONS

namespace tes
{
class Exception;

/// Log an exception using @c log::error().
/// @param e The exception to log.
void TES_CORE_API logException(const Exception &e);
/// Log an exception using @c log::error() with file and line number.
/// @param e The exception to log.
/// @param file The file from which the exception originates. Use @c __FILE__ .
/// @param line The line number from which the exception originates. Use @c __LINE__ .
void TES_CORE_API logException(const Exception &e, const char *file, int line);
}  // namespace tes

#endif  // TES_CORE_THROW_H
