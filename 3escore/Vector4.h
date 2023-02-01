//
// author: Kazys Stepanas
//
#ifndef TES_CORE_VECTOR4_H
#define TES_CORE_VECTOR4_H

#include "CoreConfig.h"

#include "Vector3.h"

namespace tes
{
template <typename T>
class Vector4;

/// Defines a single precision vector.
using Vector4f = Vector4<float>;
/// Defines a double precision vector.
using Vector4d = Vector4<double>;

/// Represents a vector in R4.
template <typename T>
class Vector4
{
public:
  /// The default epsilon value used comparison operators.
  static constexpr T kEpsilon = static_cast<T>(1e-6);

  /// A vector with all zero values.
  static const Vector4<T> Zero;
  /// The vector (1, 1, 1, 1).
  static const Vector4<T> One;
  /// The vector (1, 0, 0, 0).
  static const Vector4<T> AxisX;
  /// The vector (0, 1, 0, 0).
  static const Vector4<T> AxisY;
  /// The vector (0, 0, 1, 0).
  static const Vector4<T> AxisZ;
  /// The vector (0, 0, 0, 1).
  static const Vector4<T> AxisW;

  /// Default constructor: undefined initialisation behaviour.
  Vector4() = default;
  /// Initialises all members to @p scalar.
  /// @param scalar The value for all members.
  Vector4(const T &scalar) noexcept
    : _storage({ scalar, scalar, scalar, scalar })
  {}
  /// Copy constructor.
  /// @param other Vector to copy the value of.
  Vector4(const Vector4<T> &other) noexcept
    : _storage({ other.x(), other.y(), other.z(), other.w() })
  {}

  // NOLINTBEGIN(readability-identifier-length)
  /// Copy constructor from a Vector3.
  /// @param other Vector to copy the value of.
  /// @param w The w component value.
  Vector4(const Vector3<T> &other, const T &w) noexcept
    : _storage({ other.x(), other.y(), other.z(), w })
  {}

  /// Per coordinate initialisation.
  /// @param x The x coordinate.
  /// @param y The y coordinate.
  /// @param z The z coordinate.
  Vector4(const T &x, const T &y, const T &z, const T &w) noexcept
    : _storage({ x, y, z, w })
  {}
  // NOLINTEND(readability-identifier-length)

  /// Initialisation from a array of at least length 4.
  Vector4(const std::array<T, 4> &array) noexcept  // NOLINT(modernize-avoid-c-arrays)
    : _storage({ array[0], array[1], array[2], array[3] })
  {}

  /// Initialisation from a array of at least length 4.
  /// @param array Array to initialise from.
  template <typename U>
  Vector4(const std::array<U, 4> array) noexcept  // NOLINT(modernize-avoid-c-arrays)
    : _storage({ static_cast<T>(array[0]), static_cast<T>(array[1]), static_cast<T>(array[2]),
                 static_cast<T>(array[3]) })
  {}

  /// Initialisation from a array of at least length 4.
  /// No bounds checking is performed.
  /// @param array4 An array of at least length 4. Copies elements (0, 1, 2, 3).
  Vector4(const T array4[4]) noexcept  // NOLINT(modernize-avoid-c-arrays)
    : _storage({ array4[0], array4[1], array4[2], array4[3] })
  {}

  /// Initialisation from a array of at least length 4.
  /// No bounds checking is performed.
  /// @param array4 An array of at least length 4. Copies elements (0, 1, 2, 3).
  template <typename U>
  Vector4(const U array4[4]) noexcept  // NOLINT(modernize-avoid-c-arrays)
    : _storage({ static_cast<T>(array4[0]), static_cast<T>(array4[1]), static_cast<T>(array4[2]),
                 static_cast<T>(array4[3]) })
  {}

  /// Copy constructor from a different numeric type.
  /// @param other Vector to copy the value of.
  template <typename U>
  explicit Vector4(const Vector4<U> &other) noexcept
    : _storage({ static_cast<T>(other.x()), static_cast<T>(other.y()), static_cast<T>(other.z()),
                 static_cast<T>(other.w()) })
  {}

  // NOLINTBEGIN(readability-identifier-length)
  /// Copy constructor from a Vector3 of a different numeric type.
  /// @param other Vector to copy the value of.
  /// @param w The w component value.
  template <typename U>
  explicit Vector4(const Vector3<U> &other, const T &w) noexcept
    : _storage(
        { static_cast<T>(other.x()), static_cast<T>(other.y()), static_cast<T>(other.z()), w })
  {}
  // NOLINTEND(readability-identifier-length)

  /// Simple assignment operator.
  /// @param other Vector to copy the value of.
  /// @return This.
  Vector4<T> &operator=(const Vector4<T> &other)
  {
    _storage = other._storage;
    return *this;
  }

  /// Simple assignment operator from a different numeric type.
  /// @param other Vector to copy the value of.
  /// @return This.
  template <typename U>
  Vector4<T> &operator=(const Vector4<U> &other)
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
  /// @param index Access coordinates by index; 0 = x, 1 = y, 2 = z, 3 = w.
  /// @return The coordinate value.
  T &operator[](int index) { return _storage[index]; }
  /// @overload
  [[nodiscard]] const T &operator[](int index) const { return _storage[index]; }

  /// Exact equality operator. Compares each component with the same operator.
  /// @param other The vector to compare to.
  /// @return True if this is exactly equal to @p other.
  bool operator==(const Vector4<T> &other) const { return _storage == other._storage; }
  /// Exact inequality operator. Compares each component with the same operator.
  /// @param other The vector to compare to.
  /// @return True if this is not exactly equal to @p other.
  bool operator!=(const Vector4<T> &other) const { return _storage != other._storage; }

  /// Unarary negation operator. Equivalent to calling @c negated().
  /// @return A negated copy of the vector.
  [[nodiscard]] Vector4<T> operator-() const { return negated(); }

  /// Equality test with error. Defaults to using @c kEpsilon.
  ///
  /// The vectors are considered equal if the distance between the vectors is
  /// less than @p epsilon.
  /// @param other The vector to compare to.
  /// @param epsilon The error tolerance.
  /// @return True this and @p other are equal with @p epsilon.
  [[nodiscard]] bool isEqual(const Vector4<T> &other, const T &epsilon = kEpsilon) const;

  /// Zero test with error. Defaults to using @c kEpsilon.
  ///
  /// The vector is considered zero if the distance to zero
  /// less than @p epsilon.
  /// @param epsilon The error tolerance.
  /// @return True this within @p epsilon of zero.
  [[nodiscard]] bool isZero(const T &epsilon = kEpsilon) const;

  /// Negates all components of this vector.
  /// @return This.
  [[nodiscard]] Vector4<T> &negate()
  {
    x() = -x();
    y() = -y();
    z() = -y();
    w() = -w();
    return *this;
  }

  /// Returns a negated copy of this vector. This vector is unchanged.
  /// @return The negated value of this vector.
  [[nodiscard]] Vector4<T> negated() const { return Vector4<T>(-x(), -y(), -z(), -w()); }

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
  [[nodiscard]] Vector4<T> normalised(const T &epsilon = kEpsilon) const;

  /// Adds @p other to this vector. Component-wise addition.
  /// @param other The operand.
  /// @return This vector after the operation.
  Vector4<T> &add(const Vector4<T> &other);

  /// Adds @p scalar to all components in this vector.
  /// @param scalar The scalar value to add.
  /// @return This vector after the operation.
  Vector4<T> &add(const T &scalar);

  /// Subtracts @p other from this vector (this - other). Component-wise subtraction.
  /// @param other The operand.
  /// @return This vector after the operation.
  Vector4<T> &subtract(const Vector4<T> &other);

  /// Subtracts @p scalar from all components in this vector.
  /// @param scalar The scalar value to subtract.
  /// @return This vector after the operation.
  Vector4<T> &subtract(const T &scalar);

  /// Multiplies all components in this vector by @p scalar.
  /// @param scalar The scalar value to multiply by.
  /// @return This vector after the operation.
  Vector4<T> &multiply(const T &scalar);

  /// An alias for @p multiply(const T &).
  /// @param scalar The scalar value to multiply by.
  /// @return This vector after the operation.
  Vector4<T> &scale(const T &scalar) { return multiply(scalar); }

  /// Divides all components in this vector by @p scalar.
  /// @param scalar The scalar value to divide by. Performs no operation if @p scalar is zero.
  /// @return This vector after the operation.
  Vector4<T> &divide(const T &scalar);

  /// Calculates the dot product of this.other.
  /// @return The dot product.
  [[nodiscard]] T dot(const Vector4<T> &other) const;

  /// Calculates the dot as if using vectors in R3. That is, w is ignored.
  /// @return The dot product.
  [[nodiscard]] T dot3(const Vector4<T> &other) const;

  /// Calculates the dot product with @p other in R3. W is set to 1.
  /// @return The cross product in R3.
  [[nodiscard]] Vector4<T> cross3(const Vector4<T> &other) const;

  /// Calculates the magnitude of this vector.
  /// @return The magnitude.
  [[nodiscard]] T magnitude() const;

  /// Calculates the magnitude squared of this vector.
  /// @return The magnitude squared.
  [[nodiscard]] T magnitudeSquared() const;

  /// Arithmetic operator.
  Vector4<T> &operator+=(const Vector4 &other) { return add(other); }
  /// Arithmetic operator.
  Vector4<T> &operator+=(const T &scalar) { return add(scalar); }
  /// Arithmetic operator.
  Vector4<T> &operator-=(const Vector4 &other) { return subtract(other); }
  /// Arithmetic operator.
  Vector4<T> &operator-=(const T &scalar) { return subtract(scalar); }
  /// Arithmetic operator.
  Vector4<T> &operator*=(const T &scalar) { return multiply(scalar); }
  /// Arithmetic operator.
  Vector4<T> &operator/=(const T &scalar) { return divide(scalar); }

  /// Downcast this vector to a Vector3. W is lost.
  /// @return The x, y, z components.
  [[nodiscard]] Vector3<T> xyz() const { return Vector3<T>(x(), y(), z()); }

private:
  std::array<T, 4> _storage;
};

TES_EXTERN template class TES_CORE_API Vector4<float>;
TES_EXTERN template class TES_CORE_API Vector4<double>;

//---------------------------------------------------------------------------
// Arithmetic operators
//---------------------------------------------------------------------------

// NOLINTBEGIN(readability-identifier-length)

/// Adds two vectors.
template <class T>
inline [[nodiscard]] Vector4<T> operator+(const Vector4<T> &a, const Vector4<T> &b)
{
  Vector4<T> v(a);
  v.add(b);
  return v;
}

/// Adds two vectors.
template <class T>
inline [[nodiscard]] Vector4<T> operator+(const Vector4<T> &a, const T &b)
{
  Vector4<T> v(a);
  v.add(b);
  return v;
}

/// Adds two vectors.
template <class T>
inline [[nodiscard]] Vector4<T> operator+(const T &a, const Vector4<T> &b)
{
  return b * a;
}

/// Sutracts @p b from @p a.
template <class T>
inline [[nodiscard]] Vector4<T> operator-(const Vector4<T> &a, const Vector4<T> &b)
{
  Vector4<T> v(a);
  v.subtract(b);
  return v;
}

/// Adds two vectors.
template <class T>
inline [[nodiscard]] Vector4<T> operator-(const Vector4<T> &a, const T &b)
{
  Vector4<T> v(a);
  v.subtract(b);
  return v;
}

/// Multiplies a vector by a scalar.
template <class T>
inline [[nodiscard]] Vector4<T> operator*(const Vector4<T> &a, const T &b)
{
  Vector4<T> v(a);
  v.multiply(b);
  return v;
}

/// Multiplies a vector by a scalar.
template <class T>
inline [[nodiscard]] Vector4<T> operator*(const T &a, const Vector4<T> &b)
{
  return b * a;
}

/// Divides a vector by a scalar.
template <class T>
inline [[nodiscard]] Vector4<T> operator/(const Vector4<T> &a, const T &b)
{
  Vector4<T> v(a);
  v.divide(b);
  return v;
}


template <typename T>
inline bool Vector4<T>::isEqual(const Vector4<T> &other, const T &epsilon) const
{
  const T distance_squared = std::abs((*this - other).magnitudeSquared());
  return distance_squared <= epsilon * epsilon;
}


template <typename T>
inline bool Vector4<T>::isZero(const T &epsilon) const
{
  return isEqual(Zero, epsilon);
}


template <typename T>
inline T Vector4<T>::normalise(const T &epsilon)
{
  T mag = magnitude();
  if (mag > epsilon)
  {
    divide(mag);
  }
  return mag;
}


template <typename T>
inline Vector4<T> Vector4<T>::normalised(const T &epsilon) const
{
  T mag = magnitude();
  if (mag > epsilon)
  {
    Vector4<T> v(*this);
    v.divide(mag);
    return v;
  }
  return Zero;
}


template <typename T>
inline Vector4<T> &Vector4<T>::add(const Vector4<T> &other)
{
  x() += other.x();
  y() += other.y();
  z() += other.z();
  w() += other.w();
  return *this;
}


template <typename T>
inline Vector4<T> &Vector4<T>::add(const T &scalar)
{
  x() += scalar;
  y() += scalar;
  z() += scalar;
  w() += scalar;
  return *this;
}


template <typename T>
inline Vector4<T> &Vector4<T>::subtract(const Vector4<T> &other)
{
  x() -= other.x();
  y() -= other.y();
  z() -= other.z();
  w() -= other.w();
  return *this;
}


template <typename T>
inline Vector4<T> &Vector4<T>::subtract(const T &scalar)
{
  x() -= scalar;
  y() -= scalar;
  z() -= scalar;
  w() -= scalar;
  return *this;
}


template <typename T>
inline Vector4<T> &Vector4<T>::multiply(const T &scalar)
{
  x() *= scalar;
  y() *= scalar;
  z() *= scalar;
  w() *= scalar;
  return *this;
}


template <typename T>
inline Vector4<T> &Vector4<T>::divide(const T &scalar)
{
  const T div = static_cast<T>(1) / scalar;
  x() *= div;
  y() *= div;
  z() *= div;
  w() *= div;
  return *this;
}


template <typename T>
inline T Vector4<T>::dot(const Vector4<T> &other) const
{
  return x() * other.x() + y() * other.y() + z() * other.z() + w() * other.w();
}


template <typename T>
inline T Vector4<T>::dot3(const Vector4<T> &other) const
{
  return x() * other.x() + y() * other.y() + z() * other.z() + w() * other.w();
}


template <typename T>
inline Vector4<T> Vector4<T>::cross3(const Vector4<T> &other) const
{
  Vector4<T> v;
  v.x() = y() * other.z() - z() * other.y();
  v.y() = z() * other.x() - x() * other.z();
  v.z() = x() * other.y() - y() * other.x();
  v.w() = static_cast<T>(1);
  return v;
}


template <typename T>
inline T Vector4<T>::magnitude() const
{
  T mag = magnitudeSquared();
  mag = std::sqrt(mag);
  return mag;
}


template <typename T>
inline T Vector4<T>::magnitudeSquared() const
{
  return dot(*this);
}

// NOLINTEND(readability-identifier-length)


template <typename T>
const Vector4<T> Vector4<T>::Zero(static_cast<T>(0));
template <typename T>
const Vector4<T> Vector4<T>::One(static_cast<T>(1));
template <typename T>
const Vector4<T> Vector4<T>::AxisX(1, 0, 0, 0);
template <typename T>
const Vector4<T> Vector4<T>::AxisY(0, 1, 0, 0);
template <typename T>
const Vector4<T> Vector4<T>::AxisZ(0, 0, 1, 0);
template <typename T>
const Vector4<T> Vector4<T>::AxisW(0, 0, 0, 1);
}  // namespace tes

#endif  // TES_CORE_VECTOR4_H
