//
// author: Kazys Stepanas
//
#include <cstdlib>

namespace tes
{
// NOLINTBEGIN(readability-identifier-length)
template <typename T>
Matrix4<T> operator*(const Matrix4<T> &a, const Matrix4<T> &b)
{
  Matrix4<T> m;
  m(0, 0) = a(0, 0) * b(0, 0) + a(0, 1) * b(1, 0) + a(0, 2) * b(2, 0) + a(0, 3) * b(3, 0);
  m(0, 1) = a(0, 0) * b(0, 1) + a(0, 1) * b(1, 1) + a(0, 2) * b(2, 1) + a(0, 3) * b(3, 1);
  m(0, 2) = a(0, 0) * b(0, 2) + a(0, 1) * b(1, 2) + a(0, 2) * b(2, 2) + a(0, 3) * b(3, 2);
  m(0, 3) = a(0, 0) * b(0, 3) + a(0, 1) * b(1, 3) + a(0, 2) * b(2, 3) + a(0, 3) * b(3, 3);

  m(1, 0) = a(1, 0) * b(0, 0) + a(1, 1) * b(1, 0) + a(1, 2) * b(2, 0) + a(1, 3) * b(3, 0);
  m(1, 1) = a(1, 0) * b(0, 1) + a(1, 1) * b(1, 1) + a(1, 2) * b(2, 1) + a(1, 3) * b(3, 1);
  m(1, 2) = a(1, 0) * b(0, 2) + a(1, 1) * b(1, 2) + a(1, 2) * b(2, 2) + a(1, 3) * b(3, 2);
  m(1, 3) = a(1, 0) * b(0, 3) + a(1, 1) * b(1, 3) + a(1, 2) * b(2, 3) + a(1, 3) * b(3, 3);

  m(2, 0) = a(2, 0) * b(0, 0) + a(2, 1) * b(1, 0) + a(2, 2) * b(2, 0) + a(2, 3) * b(3, 0);
  m(2, 1) = a(2, 0) * b(0, 1) + a(2, 1) * b(1, 1) + a(2, 2) * b(2, 1) + a(2, 3) * b(3, 1);
  m(2, 2) = a(2, 0) * b(0, 2) + a(2, 1) * b(1, 2) + a(2, 2) * b(2, 2) + a(2, 3) * b(3, 2);
  m(2, 3) = a(2, 0) * b(0, 3) + a(2, 1) * b(1, 3) + a(2, 2) * b(2, 3) + a(2, 3) * b(3, 3);

  m(3, 0) = a(3, 0) * b(0, 0) + a(3, 1) * b(1, 0) + a(3, 2) * b(2, 0) + a(3, 3) * b(3, 0);
  m(3, 1) = a(3, 0) * b(0, 1) + a(3, 1) * b(1, 1) + a(3, 2) * b(2, 1) + a(3, 3) * b(3, 1);
  m(3, 2) = a(3, 0) * b(0, 2) + a(3, 1) * b(1, 2) + a(3, 2) * b(2, 2) + a(3, 3) * b(3, 2);
  m(3, 3) = a(3, 0) * b(0, 3) + a(3, 1) * b(1, 3) + a(3, 2) * b(2, 3) + a(3, 3) * b(3, 3);

  return m;
}

template <typename T>
Vector3<T> operator*(const Matrix4<T> &a, const Vector3<T> &v)
{
  Vector3<T> r;

  r.x() = a(0, 0) * v[0] + a(0, 1) * v[1] + a(0, 2) * v[2] + a(0, 3) * static_cast<T>(1);
  r.y() = a(1, 0) * v[0] + a(1, 1) * v[1] + a(1, 2) * v[2] + a(1, 3) * static_cast<T>(1);
  r.z() = a(2, 0) * v[0] + a(2, 1) * v[1] + a(2, 2) * v[2] + a(2, 3) * static_cast<T>(1);

  return r;
}

template <typename T>
Vector4<T> operator*(const Matrix4<T> &a, const Vector4<T> &v)
{
  Vector4<T> r;

  r.x() = a(0, 0) * v[0] + a(0, 1) * v[1] + a(0, 2) * v[2] + a(0, 3) * v[3];
  r.y() = a(1, 0) * v[0] + a(1, 1) * v[1] + a(1, 2) * v[2] + a(1, 3) * v[3];
  r.z() = a(2, 0) * v[0] + a(2, 1) * v[1] + a(2, 2) * v[2] + a(2, 3) * v[3];
  r.w() = a(3, 0) * v[0] + a(3, 1) * v[1] + a(3, 2) * v[2] + a(3, 3) * v[3];

  return r;
}


template <typename T>
const Matrix4<T> Matrix4<T>::Zero(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
template <typename T>
const Matrix4<T> Matrix4<T>::Identity(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

template <typename T>
Matrix4<T>::Matrix4(const T array16[16])  // NOLINT(modernize-avoid-c-arrays)
{
  for (int i = 0; i < 16; ++i)
  {
    _storage[i] = array16[i];
  }
}

template <typename T>
Matrix4<T>::Matrix4(const StorageType &array) noexcept
  : _storage(array)
{}

template <typename T>
template <typename U>
Matrix4<T>::Matrix4(const std::array<U, 16> &array) noexcept
{
  for (int i = 0; i < 16; ++i)
  {
    _storage[i] = static_cast<T>(array[i]);
  }
}

template <typename T>
template <typename U>
Matrix4<T>::Matrix4(const Matrix4<U> &other) noexcept
{
  for (int i = 0; i < 16; ++i)
  {
    _storage[i] = static_cast<T>(other[i]);
  }
}

template <typename T>
Matrix4<T>::Matrix4(const T &rc00, const T &rc01, const T &rc02, const T &rc03, const T &rc10,
                    const T &rc11, const T &rc12, const T &rc13, const T &rc20, const T &rc21,
                    const T &rc22, const T &rc23, const T &rc30, const T &rc31, const T &rc32,
                    const T &rc33) noexcept
{
  (*this)(0, 0) = rc00;
  (*this)(0, 1) = rc01;
  (*this)(0, 2) = rc02;
  (*this)(0, 3) = rc03;
  (*this)(1, 0) = rc10;
  (*this)(1, 1) = rc11;
  (*this)(1, 2) = rc12;
  (*this)(1, 3) = rc13;
  (*this)(2, 0) = rc20;
  (*this)(2, 1) = rc21;
  (*this)(2, 2) = rc22;
  (*this)(2, 3) = rc23;
  (*this)(3, 0) = rc30;
  (*this)(3, 1) = rc31;
  (*this)(3, 2) = rc32;
  (*this)(3, 3) = rc33;
}

template <typename T>
Matrix4<T> Matrix4<T>::rotationX(const T &angle)
{
  Matrix4<T> m = Identity;
  T s = std::sin(angle);
  T c = std::cos(angle);
  m[5] = m[10] = c;
  m[6] = -s;
  m[9] = s;
  return m;
}

template <typename T>
inline Matrix4<T> Matrix4<T>::rotationY(const T &angle)
{
  Matrix4<T> m = Identity;
  T s = std::sin(angle);
  T c = std::cos(angle);
  m[0] = m[10] = c;
  m[8] = -s;
  m[2] = s;
  return m;
}

template <typename T>
inline Matrix4<T> Matrix4<T>::rotationZ(const T &angle)
{
  Matrix4<T> m = Identity;
  T s = std::sin(angle);
  T c = std::cos(angle);
  m[0] = m[5] = c;
  m[1] = -s;
  m[4] = s;
  return m;
}

template <typename T>
inline Matrix4<T> Matrix4<T>::rotation(const T &x, const T &y, const T &z)
{
  Matrix4<T> m = rotationZ(z);
  m = rotationY(y) * m;
  m = rotationX(x) * m;
  return m;
}

template <typename T>
inline Matrix4<T> Matrix4<T>::translation(const Vector3<T> &trans)
{
  Matrix4<T> m = Identity;
  m.setTranslation(trans);
  return m;
}

template <typename T>
inline Matrix4<T> Matrix4<T>::rotationTranslation(const T &x, const T &y, const T &z,
                                                  const Vector3<T> &trans)
{
  Matrix4<T> m = rotation(x, y, z);
  m.setTranslation(trans);
  return m;
}

template <typename T>
inline Matrix4<T> Matrix4<T>::scaling(const Vector3<T> &scale)
{
  Matrix4<T> m = Identity;
  m(0, 0) = scale.x();
  m(1, 1) = scale.y();
  m(2, 2) = scale.z();
  return m;
}

template <typename T>
Matrix4<T> Matrix4<T>::lookAt(const Vector3<T> &eye, const Vector3<T> &target,
                              const Vector3<T> &axis_up, int forward_axis_index, int up_axis_index)
{
  if (forward_axis_index == up_axis_index || forward_axis_index < 0 || forward_axis_index > 2 ||
      up_axis_index < 0 || up_axis_index > 2)
  {
    // Bad axis specification.
    return Identity;
  }

  std::array<Vector3<T>, 3> axes;
  int side_axis_index = 0;
  if (forward_axis_index == 1 && up_axis_index == 2 ||
      up_axis_index == 1 && forward_axis_index == 2)
  {
    side_axis_index = 0;
  }
  if (forward_axis_index == 0 && up_axis_index == 2 ||
      up_axis_index == 0 && forward_axis_index == 2)
  {
    side_axis_index = 1;
  }
  if (forward_axis_index == 0 && up_axis_index == 1 ||
      up_axis_index == 0 && forward_axis_index == 1)
  {
    side_axis_index = 2;
  }
  axes[forward_axis_index] = (target - eye).normalised();
  axes[side_axis_index] = axes[forward_axis_index].cross(axis_up).normalised();
  axes[up_axis_index] = axes[side_axis_index].cross(axes[forward_axis_index]);

  Matrix4<T> m = Identity;
  m.setAxis(side_axis_index, axes[side_axis_index]);
  m.setAxis(forward_axis_index, axes[forward_axis_index]);
  m.setAxis(up_axis_index, axes[up_axis_index]);
  m.setTranslation(eye);

  return m;
}

template <typename T>
inline Matrix4<T> &Matrix4<T>::transpose()
{
  T temp;
  temp = (*this)(0, 1);
  (*this)(0, 1) = (*this)(1, 0);
  (*this)(1, 0) = temp;

  temp = (*this)(0, 2);
  (*this)(0, 2) = (*this)(2, 0);
  (*this)(2, 0) = temp;

  temp = (*this)(0, 3);
  (*this)(0, 3) = (*this)(3, 0);
  (*this)(3, 0) = temp;

  temp = (*this)(1, 2);
  (*this)(1, 2) = (*this)(2, 1);
  (*this)(2, 1) = temp;

  temp = (*this)(1, 3);
  (*this)(1, 3) = (*this)(3, 1);
  (*this)(3, 1) = temp;

  temp = (*this)(2, 3);
  (*this)(2, 3) = (*this)(3, 2);
  (*this)(3, 2) = temp;
  return *this;
}

template <typename T>
inline Matrix4<T> Matrix4<T>::transposed() const
{
  const Matrix4<T> m((*this)(0, 0), (*this)(1, 0), (*this)(2, 0), (*this)(3, 0), (*this)(0, 1),
                     (*this)(1, 1), (*this)(2, 1), (*this)(3, 1), (*this)(0, 2), (*this)(1, 2),
                     (*this)(2, 2), (*this)(3, 2), (*this)(0, 3), (*this)(1, 3), (*this)(2, 3),
                     (*this)(3, 3));
  return m;
}

template <typename T>
Matrix4<T> &Matrix4<T>::invert()
{
  // Inversion with Cramer's rule
  //
  // 1. Transpose the matrix.
  // 2. Calculate cofactors of matrix elements. Form a new matrix from cofactors of the given matrix
  // elements.
  // 3. Calculate the determinant of the given matrix.
  // 4. Multiply the matrix obtained in step 3 by the reciprocal of the determinant.

  // NOLINTBEGIN(readability-magic-numbers)
  Matrix4<T> transpose = transposed();  // transposed source matrix
  std::array<T, 12> pairs;              // temp array for cofactors
  T det;                                // determinant

  // calculate pairs for first 8 elements
  pairs[0] = transpose[10] * transpose[15];
  pairs[1] = transpose[14] * transpose[11];
  pairs[2] = transpose[6] * transpose[15];
  pairs[3] = transpose[14] * transpose[7];
  pairs[4] = transpose[6] * transpose[11];
  pairs[5] = transpose[10] * transpose[7];
  pairs[6] = transpose[2] * transpose[15];
  pairs[7] = transpose[14] * transpose[3];
  pairs[8] = transpose[2] * transpose[11];
  pairs[9] = transpose[10] * transpose[3];
  pairs[10] = transpose[2] * transpose[7];
  pairs[11] = transpose[6] * transpose[3];

  // calculate first 8 elements (cofactors)
  _storage[0] = pairs[0] * transpose[5] + pairs[3] * transpose[9] + pairs[4] * transpose[13];
  _storage[0] -= pairs[1] * transpose[5] + pairs[2] * transpose[9] + pairs[5] * transpose[13];
  _storage[4] = pairs[1] * transpose[1] + pairs[6] * transpose[9] + pairs[9] * transpose[13];
  _storage[4] -= pairs[0] * transpose[1] + pairs[7] * transpose[9] + pairs[8] * transpose[13];
  _storage[8] = pairs[2] * transpose[1] + pairs[7] * transpose[5] + pairs[10] * transpose[13];
  _storage[8] -= pairs[3] * transpose[1] + pairs[6] * transpose[5] + pairs[11] * transpose[13];
  _storage[12] = pairs[5] * transpose[1] + pairs[8] * transpose[5] + pairs[11] * transpose[9];
  _storage[12] -= pairs[4] * transpose[1] + pairs[9] * transpose[5] + pairs[10] * transpose[9];
  _storage[1] = pairs[1] * transpose[4] + pairs[2] * transpose[8] + pairs[5] * transpose[12];
  _storage[1] -= pairs[0] * transpose[4] + pairs[3] * transpose[8] + pairs[4] * transpose[12];
  _storage[5] = pairs[0] * transpose[0] + pairs[7] * transpose[8] + pairs[8] * transpose[12];
  _storage[5] -= pairs[1] * transpose[0] + pairs[6] * transpose[8] + pairs[9] * transpose[12];
  _storage[9] = pairs[3] * transpose[0] + pairs[6] * transpose[4] + pairs[11] * transpose[12];
  _storage[9] -= pairs[2] * transpose[0] + pairs[7] * transpose[4] + pairs[10] * transpose[12];
  _storage[13] = pairs[4] * transpose[0] + pairs[9] * transpose[4] + pairs[10] * transpose[8];
  _storage[13] -= pairs[5] * transpose[0] + pairs[8] * transpose[4] + pairs[11] * transpose[8];

  // calculate pairs for second 8 elements (cofactors)
  pairs[0] = transpose[8] * transpose[13];
  pairs[1] = transpose[12] * transpose[9];
  pairs[2] = transpose[4] * transpose[13];
  pairs[3] = transpose[12] * transpose[5];
  pairs[4] = transpose[4] * transpose[9];
  pairs[5] = transpose[8] * transpose[5];
  pairs[6] = transpose[0] * transpose[13];
  pairs[7] = transpose[12] * transpose[1];
  pairs[8] = transpose[0] * transpose[9];
  pairs[9] = transpose[8] * transpose[1];
  pairs[10] = transpose[0] * transpose[5];
  pairs[11] = transpose[4] * transpose[1];

  // calculate second 8 elements (cofactors)
  _storage[2] = pairs[0] * transpose[7] + pairs[3] * transpose[11] + pairs[4] * transpose[15];
  _storage[2] -= pairs[1] * transpose[7] + pairs[2] * transpose[11] + pairs[5] * transpose[15];
  _storage[6] = pairs[1] * transpose[3] + pairs[6] * transpose[11] + pairs[9] * transpose[15];
  _storage[6] -= pairs[0] * transpose[3] + pairs[7] * transpose[11] + pairs[8] * transpose[15];
  _storage[10] = pairs[2] * transpose[3] + pairs[7] * transpose[7] + pairs[10] * transpose[15];
  _storage[10] -= pairs[3] * transpose[3] + pairs[6] * transpose[7] + pairs[11] * transpose[15];
  _storage[14] = pairs[5] * transpose[3] + pairs[8] * transpose[7] + pairs[11] * transpose[11];
  _storage[14] -= pairs[4] * transpose[3] + pairs[9] * transpose[7] + pairs[10] * transpose[11];
  _storage[3] = pairs[2] * transpose[10] + pairs[5] * transpose[14] + pairs[1] * transpose[6];
  _storage[3] -= pairs[4] * transpose[14] + pairs[0] * transpose[6] + pairs[3] * transpose[10];
  _storage[7] = pairs[8] * transpose[14] + pairs[0] * transpose[2] + pairs[7] * transpose[10];
  _storage[7] -= pairs[6] * transpose[10] + pairs[9] * transpose[14] + pairs[1] * transpose[2];
  _storage[11] = pairs[6] * transpose[6] + pairs[11] * transpose[14] + pairs[3] * transpose[2];
  _storage[11] -= pairs[10] * transpose[14] + pairs[2] * transpose[2] + pairs[7] * transpose[6];
  _storage[15] = pairs[10] * transpose[10] + pairs[4] * transpose[2] + pairs[9] * transpose[6];
  _storage[15] -= pairs[8] * transpose[6] + pairs[11] * transpose[10] + pairs[5] * transpose[2];

  // calculate determinant
  det = transpose[0] * _storage[0] + transpose[4] * _storage[4] + transpose[8] * _storage[8] +
        transpose[12] * _storage[12];

  // calculate matrix inverse
  const T det_inv = static_cast<T>(1) / det;
  for (unsigned i = 0; i < _storage.size(); ++i)
  {
    _storage[i] *= det_inv;
  }
  // NOLINTEND(readability-magic-numbers)

  return *this;
}

template <typename T>
Matrix4<T> Matrix4<T>::inverse() const
{
  Matrix4<T> m = *this;
  m.invert();
  return m;
}

template <typename T>
inline Matrix4<T> &Matrix4<T>::rigidBodyInvert()
{
  // Transpose 3x3.
  T temp;
  temp = (*this)(0, 1);
  (*this)(0, 1) = (*this)(1, 0);
  (*this)(1, 0) = temp;

  temp = (*this)(0, 2);
  (*this)(0, 2) = (*this)(2, 0);
  (*this)(2, 0) = temp;

  temp = (*this)(1, 2);
  (*this)(1, 2) = (*this)(2, 1);
  (*this)(2, 1) = temp;

  // Negate translation.
  (*this)(0, 3) = -(*this)(0, 3);
  (*this)(1, 3) = -(*this)(1, 3);
  (*this)(2, 3) = -(*this)(2, 3);

  // Multiply by the negated translation.
  Vector3<T> v;
  v.x() =
    (*this)(0, 0) * (*this)(0, 3) + (*this)(0, 1) * (*this)(1, 3) + (*this)(0, 2) * (*this)(2, 3);
  v.y() =
    (*this)(1, 0) * (*this)(0, 3) + (*this)(1, 1) * (*this)(1, 3) + (*this)(1, 2) * (*this)(2, 3);
  v.z() =
    (*this)(2, 0) * (*this)(0, 3) + (*this)(2, 1) * (*this)(1, 3) + (*this)(2, 2) * (*this)(2, 3);

  // Set the new translation.
  setTranslation(v);

  return *this;
}

template <typename T>
inline Matrix4<T> Matrix4<T>::rigidBodyInverse() const
{
  Matrix4<T> m = *this;
  m.rigidBodyInvert();
  return m;
}

template <typename T>
T Matrix4<T>::determinant() const
{
  // NOLINTBEGIN(readability-magic-numbers)
  Matrix4<T> transpose(transposed());  // transposed source matrix
  std::array<T, 12> pairs;             // temp array for cofactors
  std::array<T, 4> tmp;

  // calculate pairs for first 8 elements
  pairs[0] = transpose[10] * transpose[15];
  pairs[1] = transpose[14] * transpose[11];
  pairs[2] = transpose[6] * transpose[15];
  pairs[3] = transpose[14] * transpose[7];
  pairs[4] = transpose[6] * transpose[11];
  pairs[5] = transpose[10] * transpose[7];
  pairs[6] = transpose[2] * transpose[15];
  pairs[7] = transpose[14] * transpose[3];
  pairs[8] = transpose[2] * transpose[11];
  pairs[9] = transpose[10] * transpose[3];
  pairs[10] = transpose[2] * transpose[7];
  pairs[11] = transpose[6] * transpose[3];

  // calculate first 8 elements (cofactors)
  tmp[0] = pairs[0] * transpose[5] + pairs[3] * transpose[9] + pairs[4] * transpose[13];
  tmp[0] -= pairs[1] * transpose[5] + pairs[2] * transpose[9] + pairs[5] * transpose[13];
  tmp[1] = pairs[1] * transpose[1] + pairs[6] * transpose[9] + pairs[9] * transpose[13];
  tmp[1] -= pairs[0] * transpose[1] + pairs[7] * transpose[9] + pairs[8] * transpose[13];
  tmp[2] = pairs[2] * transpose[1] + pairs[7] * transpose[5] + pairs[10] * transpose[13];
  tmp[2] -= pairs[3] * transpose[1] + pairs[6] * transpose[5] + pairs[11] * transpose[13];
  tmp[3] = pairs[5] * transpose[1] + pairs[8] * transpose[5] + pairs[11] * transpose[9];
  tmp[3] -= pairs[4] * transpose[1] + pairs[9] * transpose[5] + pairs[10] * transpose[9];

  // calculate determinant
  return (transpose[0] * tmp[0] + transpose[4] * tmp[1] + transpose[8] * tmp[2] +
          transpose[12] * tmp[3]);
  // NOLINTEND(readability-magic-numbers)
}

template <typename T>
inline Vector3<T> Matrix4<T>::axisX() const
{
  return axis(0);
}

template <typename T>
inline Vector3<T> Matrix4<T>::axisY() const
{
  return axis(1);
}

template <typename T>
inline Vector3<T> Matrix4<T>::axisZ() const
{
  return axis(2);
}

template <typename T>
inline Vector3<T> Matrix4<T>::axisT() const
{
  return axis(3);
}

template <typename T>
inline Vector3<T> Matrix4<T>::translation() const
{
  return axis(3);
}

template <typename T>
inline Vector3<T> Matrix4<T>::axis(int index) const
{
  const Vector3<T> v((*this)(0, index), (*this)(1, index), (*this)(2, index));
  return v;
}

template <typename T>
inline Matrix4<T> &Matrix4<T>::setAxisX(const Vector3<T> &axis)
{
  return setAxis(0, axis);
}

template <typename T>
inline Matrix4<T> &Matrix4<T>::setAxisY(const Vector3<T> &axis)
{
  return setAxis(1, axis);
}

template <typename T>
inline Matrix4<T> &Matrix4<T>::setAxisZ(const Vector3<T> &axis)
{
  return setAxis(2, axis);
}

template <typename T>
inline Matrix4<T> &Matrix4<T>::setAxisT(const Vector3<T> &axis)
{
  return setAxis(3, axis);
}

template <typename T>
inline Matrix4<T> &Matrix4<T>::setTranslation(const Vector3<T> &axis)
{
  return setAxis(3, axis);
}

template <typename T>
inline Matrix4<T> &Matrix4<T>::setAxis(int index, const Vector3<T> &axis)
{
  (*this)(0, index) = axis.x();
  (*this)(1, index) = axis.y();
  (*this)(2, index) = axis.z();
  return *this;
}

template <typename T>
inline Vector3<T> Matrix4<T>::scale() const
{
  const Vector3<T> v(axisX().magnitude(), axisY().magnitude(), axisZ().magnitude());
  return v;
}

template <typename T>
inline Matrix4<T> &Matrix4<T>::scale(const Vector3<T> &scaling)
{
  (*this)(0, 0) *= scaling.x();
  (*this)(1, 0) *= scaling.x();
  (*this)(2, 0) *= scaling.x();
  (*this)(3, 0) *= scaling.x();

  (*this)(0, 1) *= scaling.y();
  (*this)(1, 1) *= scaling.y();
  (*this)(2, 1) *= scaling.y();
  (*this)(3, 1) *= scaling.y();

  (*this)(0, 2) *= scaling.z();
  (*this)(1, 2) *= scaling.z();
  (*this)(2, 2) *= scaling.z();
  (*this)(3, 2) *= scaling.z();

  return *this;
}

template <class T>
inline Vector3<T> Matrix4<T>::removeScale()
{
  Vector3<T> scale(axisX().magnitude(), axisY().magnitude(), axisZ().magnitude());
  this->scale(Vector3<T>(static_cast<T>(1) / scale.x(), static_cast<T>(1) / scale.y(),
                         static_cast<T>(1) / scale.z()));
  return scale;
}

template <typename T>
inline Vector3<T> Matrix4<T>::transform(const Vector3<T> &v) const
{
  return *this * v;
}

template <typename T>
inline Vector3<T> Matrix4<T>::rotate(const Vector3<T> &v) const
{
  Vector3<T> r;

  r.x() = (*this)(0, 0) * v[0] + (*this)(0, 1) * v[1] + (*this)(0, 2) * v[2];
  r.y() = (*this)(1, 0) * v[0] + (*this)(1, 1) * v[1] + (*this)(1, 2) * v[2];
  r.z() = (*this)(2, 0) * v[0] + (*this)(2, 1) * v[1] + (*this)(2, 2) * v[2];

  return r;
}

template <typename T>
inline Vector4<T> Matrix4<T>::transform(const Vector4<T> &v) const
{
  return *this * v;
}

template <typename T>
inline Vector4<T> Matrix4<T>::rotate(const Vector4<T> &v) const
{
  Vector4<T> r;

  r.x() = (*this)(0, 0) * v[0] + (*this)(0, 1) * v[1] + (*this)(0, 2) * v[2];
  r.y() = (*this)(1, 0) * v[0] + (*this)(1, 1) * v[1] + (*this)(1, 2) * v[2];
  r.z() = (*this)(2, 0) * v[0] + (*this)(2, 1) * v[1] + (*this)(2, 2) * v[2];
  r.w() = (*this)(3, 0) * v[0] + (*this)(3, 1) * v[1] + (*this)(3, 2) * v[2];

  return r;
}


template <typename T>
inline bool Matrix4<T>::isEqual(const Matrix4<T> &a, const T epsilon) const
{
  // NOLINTBEGIN(readability-magic-numbers)
  return std::abs(_storage[0] - a[0]) <= epsilon && std::abs(_storage[1] - a[1]) <= epsilon &&
         std::abs(_storage[2] - a[2]) <= epsilon && std::abs(_storage[3] - a[3]) <= epsilon &&
         std::abs(_storage[4] - a[4]) <= epsilon && std::abs(_storage[5] - a[5]) <= epsilon &&
         std::abs(_storage[6] - a[6]) <= epsilon && std::abs(_storage[7] - a[7]) <= epsilon &&
         std::abs(_storage[8] - a[8]) <= epsilon && std::abs(_storage[9] - a[9]) <= epsilon &&
         std::abs(_storage[10] - a[10]) <= epsilon && std::abs(_storage[11] - a[11]) <= epsilon &&
         std::abs(_storage[12] - a[12]) <= epsilon && std::abs(_storage[13] - a[13]) <= epsilon &&
         std::abs(_storage[14] - a[14]) <= epsilon && std::abs(_storage[15] - a[15]) <= epsilon;
  // NOLINTEND(readability-magic-numbers)
}
// NOLINTEND(readability-identifier-length)
}  // namespace tes
