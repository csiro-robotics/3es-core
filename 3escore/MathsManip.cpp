// Copyright (c) 2018
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Kazys Stepanas
#include "MathsManip.h"

namespace
{
struct ModeId
{
  int id = std::ios_base::xalloc();
};

int matModeId()
{
  static const ModeId id;
  return id.id;
}

int v4WModeId()
{
  static const ModeId id;
  return id.id;
}

int quatWModeId()
{
  static const ModeId id;
  return id.id;
}

}  // namespace


namespace tes
{
MatrixMode getMatMode(std::ostream &stream)
{
  const auto val = stream.iword(matModeId());
  return static_cast<MatrixMode>(val);
}
WMode getQuatWMode(std::ostream &stream)
{
  const auto val = stream.iword(v4WModeId());
  return static_cast<WMode>(val);
}
WMode getV4WMode(std::ostream &stream)
{
  const auto val = stream.iword(quatWModeId());
  return static_cast<WMode>(val);
}
}  // namespace tes

std::ostream &tesMatrixMode(std::ostream &stream, tes::MatrixMode mode)
{
  stream.iword(matModeId()) = static_cast<int>(mode);
  return stream;
}


std::ostream &tesV4WMode(std::ostream &stream, tes::WMode mode)
{
  stream.iword(v4WModeId()) = static_cast<int>(mode);
  return stream;
}


std::ostream &tesQuatWMode(std::ostream &stream, tes::WMode mode)
{
  stream.iword(quatWModeId()) = static_cast<int>(mode);
  return stream;
}
