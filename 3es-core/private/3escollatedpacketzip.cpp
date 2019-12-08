#include "3escollatedpacketzip.h"

namespace tes
{
const int TesToGZipCompressionLevel[CL_Levels] = {
  0,  // CL_None,
  3,  // CL_Low,
  5,  // CL_Medium
  7,  // CL_High,
#ifdef TES_ZLIB
  Z_BEST_COMPRESSION  // CL_VeryHigh (9)
#else                 // TES_ZLIB
  9
#endif                // TES_ZLIB
};

#ifdef TES_ZLIB
const int CollatedPacketZip::DefaultCompressionLevel = tes::TesToGZipCompressionLevel[CL_Default];
#endif  // TES_ZLIB
}  // namespace tes
