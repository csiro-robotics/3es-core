//
// Author: Kazys Stepanas
//
#include "3esthrow.h"

#include "3esexception.h"
#include "3eslog.h"

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
