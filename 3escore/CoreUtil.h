//
// author: Kazys Stepanas
//
#ifndef TES_CORE_CORE_UTIL_H
#define TES_CORE_CORE_UTIL_H

#include "CoreConfig.h"

#include "Colour.h"
#include "Exception.h"
#include "Throw.h"
#include "Vector4.h"

#include <limits>

namespace tes
{
/// A utility function for moving a pointer by a given byte stride.
/// @param ptr The pointer to move.
/// @param stride The stride in bytes to move the pointer on by.
/// @return The address of ptr + stride (byte move).
/// @tparam T The pointer type.
/// @tparam ST The stride type. Must be an integer type.
template <typename T, typename ST>
inline T *moveByStride(T *ptr, ST stride)
{
  char *mem = reinterpret_cast<char *>(ptr);
  mem += stride;
  return reinterpret_cast<T *>(mem);
}


/// @overload
template <typename T, typename ST>
inline const T *moveByStride(const T *ptr, ST stride)
{
  const char *mem = reinterpret_cast<const char *>(ptr);
  mem += stride;
  return reinterpret_cast<const T *>(mem);
}


/// Convert a @c Colour to @c Vector4.
///
/// Colour channels [R, G, B, A] line up with vertex channels [x, y, z, w].
template <typename T>
inline Vector4<T> toVector(const Colour &c)
{
  return Vector4<T>(static_cast<T>(c.rf()), static_cast<T>(c.gf()), static_cast<T>(c.bf()),
                    static_cast<T>(c.af()));
}
template Vector4<float> TES_CORE_API toVector(const Colour &c);
template Vector4<double> TES_CORE_API toVector(const Colour &c);


/// Convert a @c Colour to a 4-component vector.
///
/// RGBA channels are mapped to XYZA respectively. Values are scaled [0, 1] depending on the input
/// value [0, 255].
/// @param c The colour to convert.
/// @return The floating point representation of the colour.
inline Vector4f TES_CORE_API toVectorf(const Colour &c)
{
  return toVector<float>(c);
}

/// Convert a @c Colour to a 4-component vector.
///
/// RGBA channels are mapped to XYZA respectively. Values are scaled [0, 1] depending on the input
/// value [0, 255].
/// @param c The colour to convert.
/// @return The floating point (double) representation of the colour.
inline Vector4d TES_CORE_API toVectord(const Colour &c)
{
  return toVector<double>(c);
}


/// Convert a @c Vector4 to a @c Colour. Some precision will be lost.
///
/// Colour channels [R, G, B, A] line up with vertex channels [x, y, z, w].
/// @param v The vector argument to convert.
/// @return The @c Colour representation of @p v .
template <typename T>
inline Colour toColour(const Vector4<T> &v)
{
  Colour c;
  c.setRf(static_cast<float>(v.x()));
  c.setGf(static_cast<float>(v.y()));
  c.setBf(static_cast<float>(v.z()));
  c.setAf(static_cast<float>(v.w()));
  return c;
}


template Colour TES_CORE_API toColour(const Vector4<float> &v);
template Colour TES_CORE_API toColour(const Vector4<double> &v);


/// A helper to cast between integer types with a bounds check.
///
/// If @p value is out of range for type @p Int type, then @c TES_THROW() is used, either throwing
/// an @c Exception or just logging and continuing, depending on how the library has been compiled.
///
/// @tparam Int The integer type to cast to.
/// @tparam SrcInt The integer type to cast from
/// @param value The value to cast.
/// @return The @p value cast to type @p Int .
template <typename Int, typename SrcInt>
Int int_cast(SrcInt value)  // NOLINT(readability-identifier-naming)
{
  static_assert(std::is_integral<Int>::value, "Expecting integer type");
  static_assert(std::is_integral<SrcInt>::value, "Expecting integer type");
  // NOLINTNEXTLINE(misc-redundant-expression)
  const auto lowest = std::numeric_limits<Int>::lowest();
  const auto highest = std::numeric_limits<Int>::max();
  bool overflow = false;
  if constexpr (std::is_signed<Int>::value == std::is_signed<SrcInt>::value)
  {
    // Source and target types are both signed or unsigned.
    overflow = value < lowest || value > highest;
  }
  else if constexpr (std::is_signed<Int>::value)
  {
    // Target type is signed, source is unsigned. Only check upper limit.
    overflow = value > highest;
  }
  else
  {
    // Target type is unsigned and source is signed.
    overflow = value < 0 || value > highest;
  }
  if (overflow)
  {
    // The source value is out of range for the destination type.
    TES_THROW(Exception("Integer overflow"), Int(value));
  }
  return static_cast<Int>(value);
}
}  // namespace tes

#endif  // TES_CORE_CORE_UTIL_H
