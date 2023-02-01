//
// author: Kazys Stepanas
//
#ifndef TES_CORE_MATHS_H
#define TES_CORE_MATHS_H

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif  // _USE_MATH_DEFINES
#include <cmath>
#include <cinttypes>

namespace tes
{
/// Conversion from degrees to radians.
/// @param angle The angle to convert (degrees).
/// @return The equivalent angle in radians.
template <typename T>
inline T degToRad(const T &angle = T(1))
{
  return angle / T(180) * T(M_PI);
}


/// Conversion from radians to degrees.
/// @param angle The angle to convert (radians).
/// @return The equivalent angle in degrees.
template <typename T>
inline T radToDeg(const T &angle = T(1))
{
  return angle * T(180) / T(M_PI);
}


/// Round up to the next power of 2.
///
/// From: https://graphics.stanford.edu/~seander/bithacks.html
/// @param value The value to round up.
/// @return The next power of 2 larger than v.
inline uint32_t nextLog2(uint32_t value)
{
  value--;
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  value++;
  return value;
}


/// @overload
inline uint64_t nextLog2(uint64_t value)
{
  value--;
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  value |= value >> 32;
  value++;
  return value;
}


/// Helper function to square a number.
/// @tparam T Mathematical type which supports @c operator*() .
/// @param value The value to square.
/// @return `value * value`
template <typename T>
inline T sqr(const T &value)
{
  return value * value;
}
}  // namespace tes

#endif  // TES_CORE_MATHS_H
