//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SHAPES_QUAT_ARG_H
#define TES_CORE_SHAPES_QUAT_ARG_H

#include <3escore/CoreConfig.h>

#include "Quaternion.h"
#include "V3Arg.h"

namespace tes
{
/// A helper structure used to convert from float or double pointers to @c Quaterniond arguments.
struct QuatArg
{
  /// Single precision pointer constructor.
  /// @param v Vector 3 array.
  inline QuatArg(const float q[4])
    : q(Quaternionf(v))
  {}
  /// Double precision pointer constructor.
  /// @param v Vector 3  array.
  inline QuatArg(const double q[4])
    : q(v)
  {}
  /// Single precision vector constructor.
  /// @param v Vector 3 value.
  inline QuatArg(const Quaternionf &v)
    : q(v)
  {}
  /// Double precision vector constructor.
  /// @param v Vector 3 value.
  inline QuatArg(const Quaterniond &v)
    : q(v)
  {}

  /// Component wise constructor.
  /// @param x X value.
  /// @param y Y value.
  /// @param z Z value.
  inline QuatArg(float x, float y, float z, float w)
    : q(Quaternionf(x, y, z, w))
  {}

  /// Component wise constructor.
  /// @param x X value.
  /// @param y Y value.
  /// @param z Z value.
  inline QuatArg(double x, double y, double z, double w)
    : q(x, y, z, w)
  {}

  /// Copy constructor.
  /// @param other The value to copy.
  inline QuatArg(const QuatArg &other)
    : q(other.q)
  {}

  /// Convert to @c Quaternionf.
  /// @return The single precision vector 3.
  inline operator Quaternionf() const { return q; }

  /// Indexing operator.
  /// @param i The element index [0, 2].
  /// @return The requested element
  inline double operator[](int i) const { return q[i]; }

  /// Quaternion value.
  Quaterniond q;
};
}  // namespace tes

#endif  // TES_CORE_SHAPES_QUAT_ARG_H
