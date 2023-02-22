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
  ClNone,
  ClLow,
  ClMedium,
  ClHigh,
  ClVeryHigh,

  ClLevels,

  ClDefault = ClMedium
};
}  // namespace tes

#endif  // TES_CORE_COMPRESSION_LEVEL_H
