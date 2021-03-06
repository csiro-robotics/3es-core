// Copyright (c) 2018
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Kazys Stepanas
#include "3esmathsmanip.h"

namespace
{
int matModeId = std::ios_base::xalloc();
int v4WModeId = std::ios_base::xalloc();
int quatWModeId = std::ios_base::xalloc();
}  // namespace


namespace tes
{
int getMatMode(std::ostream &o)
{
  return int(o.iword(matModeId));
}
int getQuatWMode(std::ostream &o)
{
  return int(o.iword(v4WModeId));
}
int getV4WMode(std::ostream &o)
{
  return int(o.iword(quatWModeId));
}
}  // namespace tes

std::ostream &matmode(std::ostream &o, int mode)
{
  o.iword(matModeId) = mode;
  return o;
}


std::ostream &v4wmode(std::ostream &o, int mode)
{
  o.iword(v4WModeId) = mode;
  return o;
}


std::ostream &quatwmode(std::ostream &o, int mode)
{
  o.iword(quatWModeId) = mode;
  return o;
}
