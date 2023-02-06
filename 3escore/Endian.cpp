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
  auto *data_copy = reinterpret_cast<uint8_t *>(alloca(size));
  std::memcpy(data_copy, data, size);
  for (size_t i = 0; i < size / 2; ++i)
  {
    const auto index_a = i;
    const auto index_b = size - i - 1;
    data[index_a] = data_copy[index_b];
    data[index_b] = data_copy[index_a];
  }
}
}  // namespace tes
