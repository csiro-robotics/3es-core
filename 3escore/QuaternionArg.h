//
// author: Kazys Stepanas
//
#ifndef TES_CORE_QUATERNION_ARG_H
#define TES_CORE_QUATERNION_ARG_H

#include "CoreConfig.h"

#include "Quaternion.h"

namespace tes
{
/// A helper structure used to convert from float or double pointers to @c Quaternionf arguments.
struct QuaternionArg
{
  /// Single precision pointer constructor.
  /// @param q Quaternion array.
  QuaternionArg(const float q[4])  // NOLINT(modernize-avoid-c-arrays)
    : q(q)
  {}
  /// Double precision pointer constructor.
  /// @param q Quaternion array.
  QuaternionArg(const double q[4])  // NOLINT(modernize-avoid-c-arrays)
    : q(Quaterniond(q))
  {}
  /// Single precision quaternion constructor.
  /// @param q Quaternion value.
  QuaternionArg(const Quaternionf &q)
    : q(q)
  {}
  /// Double precision quaternion constructor.
  /// @param q Quaternion value.
  QuaternionArg(const Quaterniond &q)
    : q(q)
  {}

  /// Component wise constructor.
  /// @param x X value.
  /// @param y Y value.
  /// @param z Z value.
  /// @param w W value.
  QuaternionArg(float x, float y, float z, float w)
    : q(x, y, z, w)
  {}

  /// Convert to @c Quaternionf.
  /// @return The single precision quaternion.
  [[nodiscard]] operator Quaternionf() const { return q; }

  /// Indexing operator.
  /// @param i The element index [0, 3].
  /// @return The requested element
  [[nodiscard]] float operator[](int i) const { return q[i]; }

  /// Quaternion value.
  Quaternionf q;
};
}  // namespace tes

#endif  // TES_CORE_QUATERNION_ARG_H
