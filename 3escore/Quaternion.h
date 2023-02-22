//
// author: Kazys Stepanas
//
#ifndef TES_CORE_QUATERNION_H
#define TES_CORE_QUATERNION_H

#include "CoreConfig.h"

#include "Vector3.h"

namespace tes
{
/// A 4D rotational quaternion class.
template <typename T>
class Quaternion
{
public:
  /// The identity quaternion (0, 0, 0, 1).
  static const Quaternion<T> Identity;

  /// Default constructor: undefined initialisation behaviour.
  Quaternion() noexcept = default;

  /// The identity constructor. The boolean parameter is ignored, being
  /// used only for overloading. General usage is to set it to true.
  Quaternion(bool) noexcept { *this = Identity; }  // NOLINT(readability-named-parameter)

  /// Copy constructor.
  /// @param other Vector to copy the value of.
  Quaternion(const Quaternion<T> &other) noexcept
    : _storage(other._storage)
  {}
  /// Copy constructor from a different numeric type.
  /// @param other Vector to copy the value of.
  template <typename U>
  Quaternion(const Quaternion<U> &other) noexcept
    : _storage({ static_cast<T>(other.x()), static_cast<T>(other.y()), static_cast<T>(other.z()),
                 static_cast<T>(other.w()) })
  {}
  // NOLINTBEGIN(readability-identifier-length)
  /// Per coordinate initialisation.
  /// @param x The x coordinate.
  /// @param y The y coordinate.
  /// @param z The z coordinate.
  /// @param w The w coordinate.
  Quaternion(const T &x, const T &y, const T &z, const T &w) noexcept
    : _storage({ x, y, z, w })
  {}
  /// Vector plus scalar initialisation.
  /// @param v Values for x, y and z.
  /// @param w The w coordinate.
  Quaternion(const Vector3<T> &v, const T &w) noexcept
    : _storage({ v.x(), v.y(), v.z(), w })
  {}

  /// Initialisation from a array of at least length 4.
  /// @param array Array to initialise from.
  Quaternion(const std::array<T, 4> &array) noexcept
    : _storage({ array[0], array[1], array[2], array[3] })
  {}
  /// Initialisation from a array of at least length 4.
  /// @param array Array to initialise from.
  template <typename U>
  Quaternion(const std::array<U, 4> &array) noexcept
    : _storage({ static_cast<T>(array[0]), static_cast<T>(array[1]), static_cast<T>(array[2]),
                 static_cast<T>(array[3]) })
  {}

  /// Initialisation from a array of at least length 4.
  /// No bounds checking is performed.
  /// @param array4 An array of at least length 4. Copies elements (0, 1, 2, 3).
  Quaternion(const T array4[4]) noexcept  // NOLINT(modernize-avoid-c-arrays)
    : _storage({ array4[0], array4[1], array4[2], array4[3] })
  {}
  /// Initialisation from a array of at least length 4.
  /// No bounds checking is performed.
  /// @param array4 An array of at least length 4. Copies elements (0, 1, 2, 3).
  template <typename U>
  Quaternion(const U array4[4]) noexcept  // NOLINT(modernize-avoid-c-arrays)
    : _storage({ static_cast<T>(array4[0]), static_cast<T>(array4[1]), static_cast<T>(array4[2]),
                 static_cast<T>(array4[3]) })
  {}

  /// Create the quaternion rotation transforming @p from => @p to.
  /// @param from Starting vector.
  /// @param to Target vector.
  Quaternion(const Vector3<T> &from, const Vector3<T> &to);
  // NOLINTEND(readability-identifier-length)

  /// Simple assignment operator.
  /// @param other Vector to copy the value of.
  /// @return This.
  Quaternion<T> &operator=(const Quaternion<T> &other) = default;

  /// Simple assignment operator from a different numeric type.
  /// @param other Vector to copy the value of.
  /// @return This.
  template <typename U>
  Quaternion<T> &operator=(const Quaternion<U> &other)
  {
    x() = static_cast<T>(other.x());
    y() = static_cast<T>(other.y());
    z() = static_cast<T>(other.z());
    w() = static_cast<T>(other.w());
    return *this;
  }


  /// Return the internal data storage. Used for buffer packing and network transfer.
  /// @return The internal array.
  [[nodiscard]] const std::array<T, 4> &storage() const { return _storage; }

  /// Get the x coordinate for read/write access.
  /// @return The x coordinate.
  T &x() { return _storage[0]; }
  /// Get the x coordinate for read-only access.
  /// @return The x coordinate.
  [[nodiscard]] T x() const { return _storage[0]; }

  /// Get the y coordinate for read/write access.
  /// @return The y coordinate.
  T &y() { return _storage[1]; }
  /// Get the y coordinate for read-only access.
  /// @return The y coordinate.
  [[nodiscard]] T y() const { return _storage[1]; }

  /// Get the z coordinate for read/write access.
  /// @return The z coordinate.
  T &z() { return _storage[2]; }
  /// Get the z coordinate for read-only access.
  /// @return The z coordinate.
  [[nodiscard]] T z() const { return _storage[2]; }

  /// Get the w coordinate for read/write access.
  /// @return The w coordinate.
  T &w() { return _storage[3]; }
  /// Get the w coordinate for read-only access.
  /// @return The w coordinate.
  [[nodiscard]] T w() const { return _storage[3]; }
  /// Index operator. Not bounds checked.
  /// @param index Access coordinates by index; 0 = x, 1 = y, 2 = z.
  /// @return The coordinate value.
  T &operator[](int index) { return _storage[index]; }
  /// @overload
  const T &operator[](int index) const { return _storage[index]; }

  /// Exact equality operator. Compares each component with the same operator.
  /// @param other The vector to compare to.
  /// @return True if this is exactly equal to @p other.
  [[nodiscard]] bool operator==(const Quaternion<T> &other) const;
  /// Exact inequality operator. Compares each component with the same operator.
  /// @param other The vector to compare to.
  /// @return True if this is not exactly equal to @p other.
  [[nodiscard]] bool operator!=(const Quaternion<T> &other) const;

  /// Equality test with error. Defaults to using @c kEpsilon.
  ///
  /// The vectors are considered equal of each individual component is
  /// within @c +/- @p epsilon of it's corresponding component in @p other.
  /// @param other The vector to compare to.
  /// @param epsilon The error tolerance.
  /// @return True this and @p other are equal with @p epsilon.
  [[nodiscard]] bool isEqual(const Quaternion<T> &other,
                             const T &epsilon = Vector3<T>::kEpsilon) const;

  /// Checks if this quaternion is exactly identity.
  /// @return True if this is exactly the identity quaternion.
  [[nodiscard]] bool isIdentity();

  /// Converts this quaternion into a axis of rotation and the rotation angle around that axis
  /// (radians).
  /// @param[out] axis Set to the axis of rotation. Set to (0, 0, 1) if
  ///   this quaternion is identity or near zero length.
  /// @param[out] angle Set to the rotation angle (radians). Zero if this
  ///   quaternion is identity.
  void getAxisAngle(Vector3<T> &axis, T &angle) const;

  /// Sets this quaternion from an axis of rotation and the angle of rotation about that axis
  /// (radians).
  /// @param axis The axis of rotation.
  /// @param angle The rotation angle (radians).
  Quaternion<T> &setAxisAngle(const Vector3<T> &axis, const T &angle);

  /// Inverts this quaternion, making the counter rotation.
  /// @return This after inversion.
  Quaternion<T> &invert();

  /// Calculates and returns the inverse of this quaternion.
  /// @return The inverse of this quaternion.
  [[nodiscard]] Quaternion<T> inverse() const;

  /// Sets this quaternion to its conjugate.
  /// The conjugate is the same quaternion with x, y, z values negated.
  /// @return This after the conjugate calculation.
  Quaternion<T> &conjugate();

  /// Calculates and returns the conjugate of this quaternion.
  /// The conjugate is the same quaternion with x, y, z values negated.
  /// @return This quaternion's conjugate.
  [[nodiscard]] Quaternion<T> conjugated() const;

  /// Attempts to normalise this quaternion.
  ///
  /// Normalisation fails if the length of this quaternion is less than or
  /// equal to @p epsilon. In this case, the quaternion becomes identity.
  ///
  /// @return The magnitude of this quaternion before normalisation or
  /// zero if normalisation failed.
  T normalise(const T &epsilon = Vector3<T>::kEpsilon);

  /// Returns a normalised copy of this quaternion.
  ///
  /// Normalisation fails if the length of this quaternion is less than or
  /// equal to @p epsilon.
  ///
  /// @return A normalised copy of this quaternion, or a zero quaternion if
  /// if normalisation failed.
  [[nodiscard]] Quaternion<T> normalised(const T &epsilon = Vector3<T>::kEpsilon) const;

  /// Returns the magnitude of this quaternion.
  [[nodiscard]] T magnitude() const;

  /// Returns the magnitude squared of this quaternion.
  [[nodiscard]] T magnitudeSquared() const;

  /// Calculates the dot product of @c this and @p other.
  [[nodiscard]] T dot(const Quaternion<T> &other) const;

  /// Transforms @p v by this quaternion rotation.
  /// @return The transformed vector.
  [[nodiscard]] Vector3<T> transform(const Vector3<T> &vec) const;

  /// Multiply all components of this quaternion by a scalar.
  /// @return This after the multiplication.
  Quaternion<T> &multiply(const T &scalar);

  // NOLINTBEGIN(readability-identifier-length)
  /// Performs a spherical linear interpolation of one quaternion to another.
  /// @param from The quaternion rotation to interpolate from.
  /// @param to The quaternion rotation to interpolate to.
  /// @param t The interpolation "time", [0, 1].
  /// @return The interpolated result.
  [[nodiscard]] static Quaternion<T> slerp(const Quaternion<T> &from, const Quaternion<T> &to, T t,
                                           T epsilon = Vector3<T>::kEpsilon);
  // NOLINTEND(readability-identifier-length)

  Quaternion<T> &operator*=(const Quaternion<T> &other);
  Quaternion<T> &operator*=(const T &scalar) { return multiply(scalar); }

private:
  std::array<T, 4> _storage;
};


// NOLINTBEGIN(readability-identifier-length)
/// Multiplies one quaternion by another. This gives the combined rotation of @p b then @p a.
/// @param a A quaternion operand.
/// @param b A quaternion operand.
/// @return The rotation of @p b by @p a.
template <typename T>
[[nodiscard]] Quaternion<T> operator*(const Quaternion<T> &a, const Quaternion<T> &b);

/// Transforms a vector by a quaternion, rotating it.
/// @param a The quaternion rotation.
/// @param v The vector to rotate.
/// @return The rotation of @p v by @p a.
template <typename T>
[[nodiscard]] Vector3<T> operator*(const Quaternion<T> &a, const Vector3<T> &v);
// NOLINTEND(readability-identifier-length)

/// Defines a single precision quaternion.
using Quaternionf = Quaternion<float>;
/// Defines a double precision quaternion.
using Quaterniond = Quaternion<double>;

TES_EXTERN template class TES_CORE_API Quaternion<float>;
TES_EXTERN template class TES_CORE_API Quaternion<double>;
}  // namespace tes

#include "Quaternion.inl"

#endif  // TES_CORE_QUATERNION_H
