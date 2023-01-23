//
// Author: Kazys Stepanas
//
#ifndef TES_CORE_THROW_H
#define TES_CORE_THROW_H

#include "CoreConfig.h"

#if TES_EXCEPTIONS
// Return statement present to prevent compilation errors when switching TES_EXCEPTIONS.
#define TES_THROW(exc, return_value) \
  {                                  \
    throw(exc);                      \
    return return_value;             \
  }
#define TES_THROW2(exc) throw exc
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

void TES_CORE_API logException(const Exception &e);
void TES_CORE_API logException(const Exception &e, const char *file, int line);
}  // namespace tes

#endif  // TES_CORE_THROW_H
