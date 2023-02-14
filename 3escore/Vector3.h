//
// author: Kazys Stepanas
//
#ifndef TES_CORE_VECTOR3_H
#define TES_CORE_VECTOR3_H

#include "CoreConfig.h"

#include <array>
#include <cmath>
#include <cstdlib>

namespace tes
{
template <typename T>
class Vector3;
/// Defines a single precision vector.
using Vector3f = Vector3<float>;
/// Defines a double precision vector.
using Vector3d = Vector3<double>;

/// Represents a vector in R3.
template <typename T>
class Vector3
{
public:
  /// The default epsilon value used comparison operators.
  static constexpr T kEpsilon = static_cast<T>(1e-6);

  /// A vector with all zero values.
  static const Vector3<T> Zero;
  /// The vector (1, 1, 1).
  static const Vector3<T> One;
  /// The vector (1, 0, 0).
  static const Vector3<T> AxisX;
  /// The vector (0, 1, 0).
  static const Vector3<T> AxisY;
  /// The vector (0, 0, 1).
  static const Vector3<T> AxisZ;

  /// Default constructor: undefined initialisation behaviour.
  Vector3() noexcept = default;
  /// Initialises all members to @p scalar.
  /// @param scalar The value for all members.
  Vector3(const T &scalar) noexcept
    : _storage({ scalar, scalar, scalar })
  {}
  /// Copy constructor.
  /// @param other Vector to copy the value of.
  Vector3(const Vector3<T> &other) noexcept
    : _storage(other._storage)
  {}

  // NOLINTBEGIN(readability-identifier-length)
  /// Per coordinate initialisation.
  /// @param x The x coordinate.
  /// @param y The y coordinate.
  /// @param z The z coordinate.
  Vector3(const T &x, const T &y, const T &z) noexcept
    : _storage({ x, y, z })
  {}
  // NOLINTEND(readability-identifier-length)

  /// Initialisation from a array of at least length 3.
  /// @param array Array to initialise from.
  Vector3(const std::array<T, 3> &array) noexcept
    : _storage(array)
  {}

  /// Initialisation from a array of at least length 3.
  /// @param array Array to initialise from.
  template <typename U>
  Vector3(const std::array<U, 3> &array) noexcept
    : _storage({ static_cast<T>(array[0]), static_cast<T>(array[1]), static_cast<T>(array[2]) })
  {}

  /// Initialisation from a array of at least length 3.
  /// No bounds checking is performed.
  /// @param array3 An array of at least length 3. Copies elements (0, 1, 2).
  Vector3(const T array3[3]) noexcept  // NOLINT(modernize-avoid-c-arrays)
    : _storage({ array3[0], array3[1], array3[2] })
  {}

  /// Initialisation from a array of at least length 3.
  /// No bounds checking is performed.
  /// @param array3 An array of at least length 3. Copies elements (0, 1, 2).
  template <typename U>
  Vector3(const U array3[3]) noexcept  // NOLINT(modernize-avoid-c-arrays)
    : _storage({ static_cast<T>(array3[0]), static_cast<T>(array3[1]), static_cast<T>(array3[2]) })
  {}

  /// Copy constructor from a different numeric type.
  /// @param other Vector to copy the value of.
  template <typename U>
  Vector3(const Vector3<U> &other) noexcept
    : _storage({ static_cast<T>(other.x()), static_cast<T>(other.y()), static_cast<T>(other.z()) })
  {}

  /// Simple assignment operator.
  /// @param other Vector to copy the value of.
  /// @return This.
  Vector3<T> &operator=(const Vector3<T> &other)
  {
    _storage = other._storage;
    return *this;
  }

  /// Simple assignment operator from a different numeric type.
  /// @param other Vector to copy the value of.
  /// @return This.
  template <typename U>
  Vector3<T> &operator=(const Vector3<U> &other)
  {
    _storage[0] = static_cast<T>(other.storage()[0]);
    _storage[1] = static_cast<T>(other.storage()[1]);
    _storage[2] = static_cast<T>(other.storage()[2]);
    return *this;
  }

  /// Return the internal data storage. Used for buffer packing and network transfer.
  /// @return The internal array.
  [[nodiscard]] const std::array<T, 3> &storage() const { return _storage; }

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

  /// Index operator. Not bounds checked.
  /// @param index Access coordinates by index; 0 = x, 1 = y, 2 = z.
  /// @return The coordinate value.
  T &operator[](int index) { return _storage[index]; }
  /// @overload
  [[nodiscard]] const T &operator[](int index) const { return _storage[index]; }

  /// Index operator. Not bounds checked.
  /// @param index Access coordinates by index; 0 = x, 1 = y, 2 = z.
  /// @return The coordinate value.
  T &operator[](unsigned index) { return _storage[index]; }
  /// @overload
  [[nodiscard]] const T &operator[](unsigned index) const { return _storage[index]; }

  /// Exact equality operator. Compares each component with the same operator.
  /// @param other The vector to compare to.
  /// @return True if this is exactly equal to @p other.
  bool operator==(const Vector3<T> &other) const { return _storage == other._storage; }
  /// Exact inequality operator. Compares each component with the same operator.
  /// @param other The vector to compare to.
  /// @return True if this is not exactly equal to @p other.
  bool operator!=(const Vector3<T> &other) const { return _storage != other._storage; }

  /// Unarary negation operator. Equivalent to calling @c negated().
  /// @return A negated copy of the vector.
  [[nodiscard]] Vector3<T> operator-() const { return negated(); }

  /// Equality test with error. Defaults to using @c kEpsilon.
  ///
  /// The vectors are considered equal if the distance between the vectors is
  /// less than @p epsilon.
  /// @param other The vector to compare to.
  /// @param epsilon The error tolerance.
  /// @return True this and @p other are equal with @p epsilon.
  [[nodiscard]] bool isEqual(const Vector3<T> &other, const T &epsilon = kEpsilon) const;

  /// Zero test with error. Defaults to using @c kEpsilon.
  ///
  /// The vector is considered zero if the distance to zero
  /// less than @p epsilon.
  /// @param epsilon The error tolerance.
  /// @return True this within @p epsilon of zero.
  [[nodiscard]] bool isZero(const T &epsilon = kEpsilon) const;

  /// Negates all components of this vector.
  /// @return This.
  Vector3<T> &negate()
  {
    x() = -x();
    y() = -y();
    z() = -y();
    return *this;
  }

  /// Returns a negated copy of this vector. This vector is unchanged.
  /// @return The negated value of this vector.
  [[nodiscard]] Vector3<T> negated() const { return Vector3<T>(-x(), -y(), -z()); }

  /// Attempts to normalise this vector.
  ///
  /// Normalisation fails if the length of this vector is less than or
  /// equal to @p epsilon. In this case, the vector remains unchanged.
  ///
  /// @return The length of this vector before normalisation or
  /// zero if normalisation failed.
  T normalise(const T &epsilon = kEpsilon);

  /// Returns a normalised copy of this vector.
  ///
  /// Normalisation fails if the length of this vector is less than or
  /// equal to @p epsilon.
  ///
  /// @return A normalised copy of this vector, or a zero vector if
  /// if normalisation failed.
  [[nodiscard]] Vector3<T> normalised(const T &epsilon = kEpsilon) const;

  /// Adds @p other to this vector. Component-wise addition.
  /// @param other The operand.
  /// @return This vector after the operation.
  Vector3<T> &add(const Vector3<T> &other);

  /// Adds @p scalar to all components in this vector.
  /// @param scalar The scalar value to add.
  /// @return This vector after the operation.
  Vector3<T> &add(const T &scalar);

  /// Subtracts @p other from this vector (this - other). Component-wise subtraction.
  /// @param other The operand.
  /// @return This vector after the operation.
  Vector3<T> &subtract(const Vector3<T> &other);

  /// Subtracts @p scalar from all components in this vector.
  /// @param scalar The scalar value to subtract.
  /// @return This vector after the operation.
  Vector3<T> &subtract(const T &scalar);

  /// Multiplies all components in this vector by @p scalar.
  /// @param scalar The scalar value to multiply by.
  /// @return This vector after the operation.
  Vector3<T> &multiply(const T &scalar);

  /// An alias for @p multiply(const T &).
  /// @param scalar The scalar value to multiply by.
  /// @return This vector after the operation.
  Vector3<T> &scale(const T &scalar) { return multiply(scalar); }

  /// Divides all components in this vector by @p scalar.
  /// @param scalar The scalar value to divide by. Performs no operation if @p scalar is zero.
  /// @return This vector after the operation.
  Vector3<T> &divide(const T &scalar);

  /// Calculates the dot product of this.other.
  /// @return The dot product.
  [[nodiscard]] T dot(const Vector3<T> &other) const;

  /// Calculates the cross product of this x other.
  /// @return The cross product vector.
  [[nodiscard]] Vector3<T> cross(const Vector3<T> &other) const;

  /// Calculates the magnitude of this vector.
  /// @return The magnitude.
  [[nodiscard]] T magnitude() const;

  /// Calculates the magnitude squared of this vector.
  /// @return The magnitude squared.
  [[nodiscard]] T magnitudeSquared() const;

  /// Arithmetic operator.
  Vector3<T> &operator+=(const Vector3 &other) { return add(other); }
  /// Arithmetic operator.
  Vector3<T> &operator+=(const T &scalar) { return add(scalar); }
  /// Arithmetic operator.
  Vector3<T> &operator-=(const Vector3 &other) { return subtract(other); }
  /// Arithmetic operator.
  Vector3<T> &operator-=(const T &scalar) { return subtract(scalar); }
  /// Arithmetic operator.
  Vector3<T> &operator*=(const T &scalar) { return multiply(scalar); }
  /// Arithmetic operator.
  Vector3<T> &operator/=(const T &scalar) { return divide(scalar); }

  // Swizzle operations.

  /// Return a copy of this vector. Provided for swizzle completeness.
  [[nodiscard]] Vector3<T> xyz() const { return Vector3<T>(*this); }
  /// Return a copy of this vector. Provided for swizzle completeness.
  [[nodiscard]] Vector3<T> xzy() const { return Vector3<T>(x(), z(), y()); }
  /// Swizzle operation.
  [[nodiscard]] Vector3<T> yzx() const { return Vector3<T>(y(), z(), x()); }
  /// Swizzle operation.
  [[nodiscard]] Vector3<T> yxz() const { return Vector3<T>(y(), x(), z()); }
  /// Swizzle operation.
  [[nodiscard]] Vector3<T> zxy() const { return Vector3<T>(z(), x(), y()); }
  /// Swizzle operation.
  [[nodiscard]] Vector3<T> zyx() const { return Vector3<T>(x(), y(), x()); }

private:
  std::array<T, 3> _storage;
};

TES_EXTERN template class TES_CORE_API Vector3<float>;
TES_EXTERN template class TES_CORE_API Vector3<double>;


//---------------------------------------------------------------------------
// Arithmetic operators
//---------------------------------------------------------------------------

// NOLINTBEGIN(readability-identifier-length)

/// Adds two vectors.
template <class T>
[[nodiscard]] inline Vector3<T> operator+(const Vector3<T> &a, const Vector3<T> &b)
{
  Vector3<T> vec(a);
  vec.add(b);
  return vec;
}

/// Adds two vectors.
template <class T>
[[nodiscard]] inline Vector3<T> operator+(const Vector3<T> &a, const T &b)
{
  Vector3<T> vec(a);
  vec.add(b);
  return vec;
}

/// Adds two vectors.
template <class T>
[[nodiscard]] inline Vector3<T> operator+(const T &a, const Vector3<T> &b)
{
  return b * a;
}

/// Sutracts @p b from @p a.
template <class T>
[[nodiscard]] inline Vector3<T> operator-(const Vector3<T> &a, const Vector3<T> &b)
{
  Vector3<T> vec(a);
  vec.subtract(b);
  return vec;
}

/// Adds two vectors.
template <class T>
[[nodiscard]] inline Vector3<T> operator-(const Vector3<T> &a, const T &b)
{
  Vector3<T> vec(a);
  vec.subtract(b);
  return vec;
}

/// Multiplies a vector by a scalar.
template <class T>
[[nodiscard]] inline Vector3<T> operator*(const Vector3<T> &a, const T &b)
{
  Vector3<T> vec(a);
  vec.multiply(b);
  return vec;
}

/// Multiplies a vector by a scalar.
template <class T>
[[nodiscard]] inline Vector3<T> operator*(const T &a, const Vector3<T> &b)
{
  return b * a;
}

/// Divides a vector by a scalar.
template <class T>
[[nodiscard]] inline Vector3<T> operator/(const Vector3<T> &a, const T &b)
{
  Vector3<T> vec(a);
  vec.divide(b);
  return vec;
}


template <typename T>
inline bool Vector3<T>::isEqual(const Vector3<T> &other, const T &epsilon) const
{
  const T distance_squared = std::abs((*this - other).magnitudeSquared());
  return distance_squared <= epsilon * epsilon;
}


template <typename T>
inline bool Vector3<T>::isZero(const T &epsilon) const
{
  return isEqual(Zero, epsilon);
}


template <typename T>
inline T Vector3<T>::normalise(const T &epsilon)
{
  T mag = magnitude();
  if (mag > epsilon)
  {
    divide(mag);
  }
  return mag;
}


template <typename T>
inline Vector3<T> Vector3<T>::normalised(const T &epsilon) const
{
  T mag = magnitude();
  if (mag > epsilon)
  {
    Vector3<T> vec(*this);
    vec.divide(mag);
    return vec;
  }
  return Zero;
}


template <typename T>
inline Vector3<T> &Vector3<T>::add(const Vector3<T> &other)
{
  x() += other.x();
  y() += other.y();
  z() += other.z();
  return *this;
}


template <typename T>
inline Vector3<T> &Vector3<T>::add(const T &scalar)
{
  x() += scalar;
  y() += scalar;
  z() += scalar;
  return *this;
}


template <typename T>
inline Vector3<T> &Vector3<T>::subtract(const Vector3<T> &other)
{
  x() -= other.x();
  y() -= other.y();
  z() -= other.z();
  return *this;
}


template <typename T>
inline Vector3<T> &Vector3<T>::subtract(const T &scalar)
{
  x() -= scalar;
  y() -= scalar;
  z() -= scalar;
  return *this;
}


template <typename T>
inline Vector3<T> &Vector3<T>::multiply(const T &scalar)
{
  x() *= scalar;
  y() *= scalar;
  z() *= scalar;
  return *this;
}


template <typename T>
inline Vector3<T> &Vector3<T>::divide(const T &scalar)
{
  const T div = static_cast<T>(1) / scalar;
  x() *= div;
  y() *= div;
  z() *= div;
  return *this;
}


template <typename T>
inline T Vector3<T>::dot(const Vector3<T> &other) const
{
  return x() * other.x() + y() * other.y() + z() * other.z();
}


template <typename T>
inline Vector3<T> Vector3<T>::cross(const Vector3<T> &other) const
{
  Vector3<T> vec;
  vec.x() = y() * other.z() - z() * other.y();
  vec.y() = z() * other.x() - x() * other.z();
  vec.z() = x() * other.y() - y() * other.x();
  return vec;
}


template <typename T>
inline T Vector3<T>::magnitude() const
{
  T mag = magnitudeSquared();
  mag = std::sqrt(mag);
  return mag;
}


template <typename T>
inline T Vector3<T>::magnitudeSquared() const
{
  return dot(*this);
}

// NOLINTEND(readability-identifier-length)


template <typename T>
const Vector3<T> Vector3<T>::Zero(static_cast<T>(0));
template <typename T>
const Vector3<T> Vector3<T>::One(static_cast<T>(1));
template <typename T>
const Vector3<T> Vector3<T>::AxisX(1, 0, 0);
template <typename T>
const Vector3<T> Vector3<T>::AxisY(0, 1, 0);
template <typename T>
const Vector3<T> Vector3<T>::AxisZ(0, 0, 1);
}  // namespace tes

#endif  // TES_CORE_VECTOR3_H
