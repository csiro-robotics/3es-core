//
// author: Kazys Stepanas
//

#include "3es-core.h"

#include "3escompressionlevel.h"

#ifdef TES_ZLIB
#include <cstring>

#include <zlib.h>
#endif  // TES_ZLIB

namespace tes
{
extern const int TesToGZipCompressionLevel[CL_Levels];

struct CollatedPacketZip
{
#ifdef TES_ZLIB
  static const int WindowBits = 15;
  static const int GZipEncoding = 16;
  static const int DefaultCompressionLevel;

  /// ZLib stream.
  z_stream stream;
  bool inflateMode;

  inline CollatedPacketZip(bool inflate)
  {
    memset(&stream, 0, sizeof(stream));
    inflateMode = inflate;
  }

  inline ~CollatedPacketZip() { reset(); }

  inline void reset()
  {
    // Ensure clean up
    if (!inflateMode)
    {
      if (stream.total_out)
      {
        deflate(&stream, Z_FINISH);
        deflateEnd(&stream);
      }
    }
    else
    {
      if (stream.total_in)
      {
        inflate(&stream, Z_FINISH);
        inflateEnd(&stream);
      }
    }
    memset(&stream, 0, sizeof(stream));
  }
#else   // TES_ZLIB
  inline CollatedPacketZip(bool) {}
  inline void reset() {}
#endif  // TES_ZLIB
};
}  // namespace tes
