//
// author: Kazys Stepanas
//

#include <3escore/CoreConfig.h>

#include <3escore/CompressionLevel.h>

#include <array>

#ifdef TES_ZLIB
#include <cstring>

#include <zlib.h>
#endif  // TES_ZLIB

namespace tes
{
extern const std::array<int, ClLevels> kTesToGZipCompressionLevel;

struct CollatedPacketZip
{
#ifdef TES_ZLIB
  static const int WindowBits = 15;
  static const int GZipEncoding = 16;
  static const int DefaultCompressionLevel;

  /// ZLib stream.
  z_stream stream = {};
  bool inflate_mode = false;

  inline CollatedPacketZip(bool inflate)
  {
    memset(&stream, 0, sizeof(stream));
    inflate_mode = inflate;
  }

  inline ~CollatedPacketZip() { reset(); }

  inline void reset()
  {
    // Ensure clean up
    if (!inflate_mode)
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
