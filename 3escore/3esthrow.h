//
// Author: Kazys Stepanas
//
#ifndef _3ESTHROW_H
#define _3ESTHROW_H

#include "3es-core.h"

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

void _3es_coreAPI logException(const Exception &e);
void _3es_coreAPI logException(const Exception &e, const char *file, int line);
}  // namespace tes

#endif  // _3ESTHROW_H