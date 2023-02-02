//
// author: Kazys Stepanas
//
#include "Endian.h"

#include <cstdlib>
#include <cstring>

#ifdef WIN32
#include <malloc.h>
#endif  // WIN32

namespace tes
{
void endianSwap(uint8_t *data, size_t size)
{
  switch (size)
  {
  case 0:
  case 1:
    return;
  case 2:
    return endianSwap2(data);
    ;
  case 4:
    return endianSwap4(data);
    ;
  case 8:
    return endianSwap8(data);
    ;
  case 16:
    return endianSwap16(data);
    ;
  default:
    break;
  }

  auto *data_copy = reinterpret_cast<uint8_t *>(alloca(size));
  std::memcpy(data_copy, data, size);
  for (size_t i = 0; i < size / 2; ++i)
  {
    data[i] = data_copy[size - i - 1];
  }
}
}  // namespace tes
