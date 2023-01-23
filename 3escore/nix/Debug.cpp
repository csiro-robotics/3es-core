//
// author: Kazys Stepanas
//
#include "../Debug.h"

#include <csignal>
#include <cstdio>

namespace tes
{
void debugBreak()
{
  std::raise(SIGINT);
}


void assertionFailure(const char *msg)
{
  fprintf(stderr, "%s\n", msg);
  std::raise(SIGINT);
}
}  // namespace tes
