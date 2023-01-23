//
// author: Kazys Stepanas
//
#ifndef _3ESDEBUG_H_
#define _3ESDEBUG_H_

#include "CoreConfig.h"

#if TES_ASSERT_ENABLE
#define TES_ASSERT2(x, msg)                          \
  if (!(x))                                          \
  {                                                  \
    tes::assertionFailure("Assertion failed: " msg); \
  }
#define TES_ASSERT(x) TES_ASSERT2(x, #x)
#else  // TES_ASSERT_ENABLE
#define TES_ASSERT(x)
#define TES_ASSERT2(x, msg)
#endif  // TES_ASSERT_ENABLE

namespace tes
{
/// Trigger a programmatic breakpoint. Behaviour varies between platforms.
void TES_CORE_API debugBreak();

/// Called on assertion failures. Prints @p msg and triggers a programmatic breakpoint.
/// @param msg The assertion message to display.
void TES_CORE_API assertionFailure(const char *msg = "");
}  // namespace tes

#endif  // _3ESDEBUG_H_
