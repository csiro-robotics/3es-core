//
// author: Kazys Stepanas
//
#ifndef TES_CORE_MATHS_H
#define TES_CORE_MATHS_H

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif  // _USE_MATH_DEFINES
#include <cinttypes>
#include <cmath>

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


/// Calculate the next power of 2 equal to or greater than @p v.
/// @param v The base, integer value.
template <typename T>
inline T nextLog2(T v)
{
  size_t next;
  bool isPow2;
  isPow2 = v && !(v & (v - 1));
  next = static_cast<T>(1) << (static_cast<T>(1) + static_cast<T>(std::floor(std::log2(float(v)))));
  return isPow2 ? v : next;
}


/// Round up to the next power of 2.
///
/// From: https://graphics.stanford.edu/~seander/bithacks.html
/// @param value The value to round up.
/// @return The next power of 2 larger than v.
template <>
inline uint32_t nextLog2(uint32_t value)
{
  value--;
  value |= value >> 1u;
  value |= value >> 2u;
  value |= value >> 4u;
  value |= value >> 8u;
  value |= value >> 16u;
  value++;
  return value;
}


/// @overload
template <>
inline int nextLog2(int value)
{
  // NOLINTBEGIN(hicpp-signed-bitwise)
  value--;
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  // NOLINTEND(hicpp-signed-bitwise)
  value++;
  return value;
}


/// @overload
template <>
inline uint64_t nextLog2(uint64_t value)
{
  value--;
  value |= value >> 1u;
  value |= value >> 2u;
  value |= value >> 4u;
  value |= value >> 8u;
  value |= value >> 16u;
  value |= value >> 32u;
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
