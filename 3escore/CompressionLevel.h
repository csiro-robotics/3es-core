//
// author: Kazys Stepanas
//
#ifndef TES_CORE_COMPRESSION_LEVEL_H
#define TES_CORE_COMPRESSION_LEVEL_H

#include "CoreConfig.h"

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

#endif  // TES_CORE_COMPRESSION_LEVEL_H
