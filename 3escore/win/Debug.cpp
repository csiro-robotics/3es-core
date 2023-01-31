//
// author: Kazys Stepanas
//
#include "../Debug.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace tes
{
void debugBreak()
{
  DebugBreak();
}


void assertionFailure(const char *msg)
{
  OutputDebugStringA(msg);
  OutputDebugStringA("\n");
  DebugBreak();
}
}  // namespace tes
