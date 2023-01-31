//
// Author: Kazys Stepanas
//
#include "Throw.h"

#include "Exception.h"
#include "Log.h"

#include <iostream>

namespace tes
{
void logException(const Exception &e)
{
  log::error(e.what());
}


void logException(const Exception &e, const char *file, int line)
{
  log::error(file, "(", line, "): ", e.what());
}
}  // namespace tes
