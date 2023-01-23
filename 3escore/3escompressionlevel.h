//
// author: Kazys Stepanas
//
#ifndef _3ESCOMPRESSIONLEVEL_H_
#define _3ESCOMPRESSIONLEVEL_H_

#include "3es-core.h"

namespace tes
{
/// Target compression levels.
enum CompressionLevel
{
  CL_None,
  CL_Low,
  CL_Medium,
  CL_High,
  CL_VeryHigh,

  CL_Levels,

  CL_Default = CL_Medium
};
}  // namespace tes

#endif  // _3ESCOMPRESSIONLEVEL_H_
