//
// author: Kazys Stepanas
//
#ifndef TES_CORE_MATRIX3_H
#define TES_CORE_MATRIX3_H

#include "CoreConfig.h"

#include "Vector3.h"

namespace tes
{
// NOLINTBEGIN(readability-identifier-length)

/// A row major 3x3 rotation matrix.
///
/// The matrix is laid out as follows:
/// @code{.unparsed}
///     | rc00  rc01  rc02  |   |  0   1   2 |   | xx  yx  zx |
/// M = | rc10  rc11  rc12  | = |  3   4   5 | = | xy  yy  zy |
///     | rc20  rc21  rc22  |   |  6   7   8 |   | xz  yz  zz |
/// @endcode
/// Where (xx, xy, xz) are the components of the X axis. Similarly, yn and zn
/// form the Y axis and Z axis of the basis vectors respectively.
template <typename T>
class Matrix3
{
public:
  /// Internal storage array type.
  using StorageType = std::array<T, 9>;

  static const Matrix3<T> Zero;      ///< A matrix with all zero elements.
  static const Matrix3<T> Identity;  ///< The identity matrix.

  /// Empty constructor; contents are undefined.
  Matrix3() noexcept = default;
  /// Array initialisation constructor from an array of at least 9 elements. No bounds checking.
  /// @param array9 The array of at least 9 value to initialise from.
  Matrix3(const T array9[9]);  // NOLINT(modernize-avoid-c-arrays)
  /// Construct from a @c std::array.
  ///
  /// Expects a row major format. Use @c transpose() when providing column major data.
  ///
  /// @param array Array to construct from.
  Matrix3(const StorageType &array) noexcept;
  /// Construct from a @c std::array.
  ///
  /// Expects a row major format. Use @c transpose() when providing column major data.
  ///
  /// @param array Array to construct from.
  template <typename U>
  Matrix3(const std::array<U, 9> &array) noexcept;
  /// Copy constructor.
  /// @param other The matrix to copy from.
  Matrix3(const Matrix3<T> &other) noexcept = default;
  /// Copy constructor from a different numeric type.
  /// @param other The matrix to copy from.
  template <typename U>
  Matrix3(const Matrix3<U> &other) noexcept;
  /// Move constructor.
  /// @param other The matrix to move from.
  Matrix3(Matrix3<T> &&other) noexcept = default;

  /// Move assignment.
  /// @param other Matrix to assign from.
  /// @return @c *this
  Matrix3<T> &operator=(Matrix3<T> &&other) noexcept = default;

  /// Copy assignment.
  /// @param other Matrix to assign from.
  /// @return @c *this
  Matrix3<T> &operator=(const Matrix3<T> &other) noexcept = default;

  /// Per element constructor, specifying each row in order.
  /// @param rc00 Element at row/column 00
  /// @param rc01 Element at row/column 01
  /// @param rc02 Element at row/column 02
  /// @param rc10 Element at row/column 10
  /// @param rc11 Element at row/column 11
  /// @param rc12 Element at row/column 12
  /// @param rc20 Element at row/column 20
  /// @param rc21 Element at row/column 21
  /// @param rc22 Element at row/column 22
  Matrix3(const T &rc00, const T &rc01, const T &rc02, const T &rc10, const T &rc11, const T &rc12,
          const T &rc20, const T &rc21, const T &rc22) noexcept;

  /// Row/column access. Not bounds checked.
  /// @param r The row to access [0, 2]
  /// @param c The column to access [0, 2].
  T &operator()(size_t r, size_t c) { return _storage[r * 3 + c]; }
  /// Row/column immutable access. Not bounds checked.
  /// @param r The row to access [0, 2]
  /// @param c The column to access [0, 2].
  [[nodiscard]] const T &operator()(size_t r, size_t c) const { return _storage[r * 3 + c]; }

  /// Indexing operator (no bounds checking).
  /// @param index The element to access [0, 9].
  /// @return The matrix element at @p index.
  T &operator[](size_t index) { return _storage[index]; }

  /// Indexing operator (no bounds checking).
  /// @param index The element to access [0, 9].
  /// @return The matrix element at @p index.
  [[nodiscard]] const T &operator[](size_t index) const { return _storage[index]; }

  /// Access the internal storage for direct memory copies.
  /// @return The internal storage.
  [[nodiscard]] const StorageType &storage() const { return _storage; }

  /// Create a matrix which represents a rotation around the X axis.
  /// @param angle The rotation angle in radians.
  /// @return The rotation matrix.
  [[nodiscard]] static Matrix3<T> rotationX(const T &angle);
  Matrix3<T> &initRotationX(const T &angle)
  {
    *this = rotationX(angle);
    return *this;
  }

  /// Create a matrix which represents a rotation around the Y axis.
  /// @param angle The rotation angle in radians.
  /// @return The rotation matrix.
  [[nodiscard]] static Matrix3<T> rotationY(const T &angle);
  Matrix3<T> &initRotationY(const T &angle)
  {
    *this = rotationY(angle);
    return *this;
  }

  /// Create a matrix which represents a rotation around the Z axis.
  /// @param angle The rotation angle in radians.
  /// @return The rotation matrix.
  [[nodiscard]] static Matrix3<T> rotationZ(const T &angle);
  Matrix3<T> &initRotationZ(const T &angle)
  {
    *this = rotationZ(angle);
    return *this;
  }

  /// Create a matrix which represents a rotation around each axis (Euler angles).
  /// Rotations are applied in x, y, z order.
  /// @param x Rotation around the X axis (radians).
  /// @param y Rotation around the Y axis (radians).
  /// @param z Rotation around the Z axis (radians).
  /// @return The rotation matrix.
  [[nodiscard]] static Matrix3<T> rotation(const T &x, const T &y, const T &z);
  Matrix3<T> &initRotation(const T &x, const T &y, const T &z)
  {
    *this = rotation(x, y, z);
    return *this;
  }

  /// Create a scaling matrix.
  /// @param scale The scaling to apply along each axis.
  /// @return The scaling matrix.
  [[nodiscard]] static Matrix3<T> scaling(const Vector3<T> &scale);
  Matrix3<T> &initScaling(const Vector3<T> &scale)
  {
    *this = scaling(scale);
    return *this;
  }

  /// Create a model or camera matrix at @p eye looking at @p target.
  ///
  /// Supports specifying the up and forward axes (inferring the left/right axis),
  /// where the indices [0, 1, 2] correspond to the axes (X, Y, Z).
  ///
  /// The default behaviour is to use Y as the forward axis and Z as up.
  ///
  /// Note: the resulting matrix can only represent the rotation part of the matrix and
  /// the @p eye translation is essentially dropped. Use @c Matrix4 where the translation
  /// is required.
  ///
  /// @param eye The position of the eye/camera (dropped after rotation is calculated).
  /// @param target The target to look at. Should not be equal to @p eye.
  /// @param axisUp The axis defining the initial up vector. Must be normalised.
  /// @param forwardAxisIndex The index of the forward axis. This is to point at @p target.
  /// @param upAxisIndex The index of the up axis. Must not be equal to @p forwardAxisIndex.
  /// @return A model matrix at @p eye pointing at @p target. Returns identity if
  /// there are errors in the specification of @p forwardAxisIndex and @p upAxisIndex.
  [[nodiscard]] static Matrix3<T> lookAt(const Vector3<T> &eye, const Vector3<T> &target,
                                         const Vector3<T> &axis_up, int forward_axis_index = 1,
                                         int up_axis_index = 2);

  /// Initialise this matrix as a model or camera matrix.
  /// @see @c lookAt().
  /// @param eye The position of the eye/camera (dropped after rotation is calculated).
  /// @param target The target to look at. Should not be equal to @p eye.
  /// @param axisUp The axis defining the initial up vector. Must be normalised.
  /// @param forwardAxisIndex The index of the forward axis. This is to point at @p target.
  /// @param upAxisIndex The index of the up axis. Must not be equal to @p forwardAxisIndex.
  /// @return @c this
  Matrix3<T> &initLookAt(const Vector3<T> &eye, const Vector3<T> &target, const Vector3<T> &axis_up,
                         int forward_axis_index = 1, int up_axis_index = 2)
  {
    *this = lookAt(eye, target, axis_up, forward_axis_index, up_axis_index);
    return *this;
  }

  /// Transposes this matrix.
  /// @return This matrix after the operation.
  Matrix3<T> &transpose();

  /// Returns the transpose of this matrix, leaving this matrix unchanged.
  /// @return The transpose of this matrix.
  [[nodiscard]] Matrix3<T> transposed() const;

  /// Inverts this matrix.
  /// @return This matrix after the operation.
  Matrix3<T> &invert();

  /// Returns the inverse of this matrix, leaving this matrix unchanged.
  /// @return The inverse of this matrix.
  [[nodiscard]] Matrix3<T> inverse() const;

  /// Gets the adjoint of this matrix.
  /// @param[out] adj The adjoint is written here.
  /// @return The determinant.
  T getAdjoint(Matrix3<T> &adj) const;

  /// Inverts this matrix assuming this matrix represents a rigid body transformation.
  ///
  /// A rigid body transformation contains no skewing. That is, the basis vectors
  /// are orthogonal.
  ///
  /// The inverse is calculated by transposing the rotation and inverting the
  /// translation.
  /// @return This matrix after the operation.
  Matrix3<T> &rigidBodyInvert();

  /// Returns the inverse of this matrix assuming this is a rigid body transformation.
  /// See @c rigidBodyInvert().
  /// @return The inverse of this matrix.
  [[nodiscard]] Matrix3<T> rigidBodyInverse() const;

  /// Calculates the determinant of this matrix.
  /// @return The determinant.
  [[nodiscard]] T determinant() const;

  /// Returns the X axis of this matrix (elements (0, 0), (1, 0), (2, 0)).
  /// @return The X axis.
  [[nodiscard]] Vector3<T> axisX() const;
  /// Returns the Y axis of this matrix (elements (0, 1), (1, 1), (2, 1)).
  /// @return The Y axis.
  [[nodiscard]] Vector3<T> axisY() const;
  /// Returns the Z axis of this matrix (elements (0, 2), (1, 2), (2, 2)).
  /// @return The Z axis.
  [[nodiscard]] Vector3<T> axisZ() const;

  /// Returns one of the axes of this matrix.
  /// @param index The index of the axis of interest.
  ///   0 => X, 1 => Y, 2 => Z.
  /// @return The axis of interest.
  [[nodiscard]] Vector3<T> axis(int index) const;

  /// Sets the X axis of this matrix. See @p axisX().
  /// @param axis The value to set the axis to.
  /// @return This matrix after the operation.
  Matrix3<T> &setAxisX(const Vector3<T> &axis);

  /// Sets the Y axis of this matrix. See @p axisY().
  /// @param axis The value to set the axis to.
  /// @return This matrix after the operation.
  Matrix3<T> &setAxisY(const Vector3<T> &axis);

  /// Sets the Z axis of this matrix. See @p axisZ().
  /// @param axis The value to set the axis to.
  /// @return This matrix after the operation.
  Matrix3<T> &setAxisZ(const Vector3<T> &axis);

  /// Sets the indexed axis of this matrix. See @p axis().
  /// @param index The index of the axis of interest.
  ///   0 => X, 1 => Y, 2 => Z.
  /// @param axis The value to set the axis to.
  /// @return This matrix after the operation.
  Matrix3<T> &setAxis(int index, const Vector3<T> &axis);

  /// Returns the scale contained in this matrix. This is the length of each axis.
  /// @return The scale of each rotation axis in this matrix.
  [[nodiscard]] Vector3<T> scale() const;

  /// Scales this matrix, adjusting the scale of each rotation, but leaving the translation.
  /// @param scaling The scaling to apply.
  /// @return This matrix after the operation.
  Matrix3<T> &scale(const Vector3<T> &scaling);

  /// Transforms the vector @p v by this matrix.
  /// @return Av, where A is this matrix.
  [[nodiscard]] Vector3<T> transform(const Vector3<T> &v) const;

  /// An alias for @c transform().
  /// @return Av, where A is this matrix.
  [[nodiscard]] Vector3<T> rotate(const Vector3<T> &v) const;

  /// Numerical equality comparison. Reports @c true if each element of this matrix is within of
  /// @p Epsilon @p a (or equal to).
  /// @return a Matrix to compare to.
  /// @param epsilon Comparison tolerance value.
  /// @return @c true when each element in this matrix is within @p epsilon of each element of @p a.
  [[nodiscard]] bool isEqual(const Matrix3<T> &a, T epsilon = Vector3<T>::Epsilon) const;

private:
  StorageType _storage;
};

/// Defines a single precision 4x4 matrix.
using Matrix3f = Matrix3<float>;
/// Defines a double precision 4x4 matrix.
using Matrix3d = Matrix3<double>;

TES_EXTERN template class TES_CORE_API Matrix3<float>;
TES_EXTERN template class TES_CORE_API Matrix3<double>;

/// Performs the matrix multiplication AB.
/// @return The result of AB.
template <typename T>
[[nodiscard]] Matrix3<T> operator*(const Matrix3<T> &a, const Matrix3<T> &b);

/// Performs the matrix multiplication Av.
/// @return The result of Av.
template <typename T>
[[nodiscard]] Vector3<T> operator*(const Matrix3<T> &a, const Vector3<T> &v);

// NOLINTEND(readability-identifier-length)
}  // namespace tes

#include "Matrix3.inl"

#endif  // TES_CORE_MATRIX3_H
