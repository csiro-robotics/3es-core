//
// Author: Kazys Stepanas
//
#include "3esthrow.h"

#include "3esexception.h"

#include <iostream>

namespace tes
{
void log(const Exception &e)
{
  std::cerr << e.what() << std::endl;
}


void log(const Exception &e, const char *file, int line)
{
  std::cerr << file << '(' << line << "): " << e.what() << std::endl;
}
}
