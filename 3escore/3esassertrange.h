//
// author: Kazys Stepanas
//
#ifndef _3ESASSERTRANGE_H_
#define _3ESASSERTRANGE_H_

#include "3es-core.h"

#include "3esdebug.h"

#include <cstddef>
#include <limits>

namespace tes
{
#ifndef DOXYGEN_SHOULD_SKIP_THIS
template <typename TO, typename FROM>
struct AssertRange
{
  inline void operator()(FROM ii)
  {
    TES_ASSERT(std::numeric_limits<TO>::min() <= FROM(ii));
    TES_ASSERT(FROM(ii) <= std::numeric_limits<TO>::max());
  }
};

template <>
struct AssertRange<unsigned, int>
{
  inline void operator()(int ii) { TES_ASSERT(0 <= ii); }
};

template <>
struct AssertRange<int, unsigned>
{
  inline void operator()(unsigned ii) { TES_ASSERT(ii <= unsigned(std::numeric_limits<int>::max())); }
};

#ifdef TES_64
template <>
struct AssertRange<size_t, int>
{
  inline void operator()(int ii) { TES_ASSERT(0 <= ii); }
};

template <>
struct AssertRange<int, size_t>
{
  inline void operator()(size_t ii) { TES_ASSERT(ii <= size_t(std::numeric_limits<int>::max())); }
};

template <>
struct AssertRange<unsigned, size_t>
{
  inline void operator()(size_t ii) { TES_ASSERT(ii <= size_t(std::numeric_limits<unsigned>::max())); }
};
#endif  // TES_64
#endif  // DOXYGEN_SHOULD_SKIP_THIS
}  // namespace tes

#endif  // _3ESASSERTRANGE_H_
