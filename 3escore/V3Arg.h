//
// author: Kazys Stepanas
//
#ifndef TES_CORE_V3_ARG_H
#define TES_CORE_V3_ARG_H

#include "CoreConfig.h"

#include "Vector3.h"

namespace tes
{
/// A helper structure used to convert from float or double pointers to @c Vector3d arguments.
struct V3Arg
{
  /// Single precision pointer constructor.
  /// @param v Vector 3 array.
  V3Arg(const float v[3])  // NOLINT(modernize-avoid-c-arrays)
    : v3(v)
  {}
  /// @overload
  V3Arg(const std::array<float, 3> &v)
    : v3(v)
  {}
  /// Double precision pointer constructor.
  /// @param v Vector 3  array.
  V3Arg(const double v[3])  // NOLINT(modernize-avoid-c-arrays)
    : v3(Vector3d(v))
  {}
  /// @overload
  V3Arg(const std::array<double, 3> &v)
    : v3(v)
  {}
  /// Single precision vector constructor.
  /// @param v Vector 3 value.
  V3Arg(const Vector3f &v)
    : v3(v)
  {}
  /// Double precision vector constructor.
  /// @param v Vector 3 value.
  V3Arg(const Vector3d &v)
    : v3(v)
  {}

  /// Component wise constructor.
  /// @param x X value.
  /// @param y Y value.
  /// @param z Z value.
  inline V3Arg(float x, float y, float z)
    : v3(Vector3f(x, y, z))
  {}

  /// Component wise constructor.
  /// @param x X value.
  /// @param y Y value.
  /// @param z Z value.
  inline V3Arg(double x, double y, double z)
    : v3(Vector3d(x, y, z))
  {}

  /// Copy constructor.
  /// @param other The value to copy.
  V3Arg(const V3Arg &other) = default;

  /// Convert to @c Vector3f.
  /// @return The single precision vector 3.
  [[nodiscard]] operator Vector3f() const { return v3; }

  /// Indexing operator.
  /// @param i The element index [0, 2].
  /// @return The requested element
  [[nodiscard]] double operator[](int i) const { return v3[i]; }

  /// Vector 3 value.
  Vector3f v3;
};
}  // namespace tes

#endif  // TES_CORE_V3_ARG_H
