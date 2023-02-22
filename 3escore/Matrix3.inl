//
// author: Kazys Stepanas
//
#include <cstdlib>

namespace tes
{
// NOLINTBEGIN(readability-identifier-length)
template <typename T>
Matrix3<T> operator*(const Matrix3<T> &a, const Matrix3<T> &b)
{
  Matrix3<T> m;
  m(0, 0) = a(0, 0) * b(0, 0) + a(0, 1) * b(1, 0) + a(0, 2) * b(2, 0);
  m(0, 1) = a(0, 0) * b(0, 1) + a(0, 1) * b(1, 1) + a(0, 2) * b(2, 1);
  m(0, 2) = a(0, 0) * b(0, 2) + a(0, 1) * b(1, 2) + a(0, 2) * b(2, 2);

  m(1, 0) = a(1, 0) * b(0, 0) + a(1, 1) * b(1, 0) + a(1, 2) * b(2, 0);
  m(1, 1) = a(1, 0) * b(0, 1) + a(1, 1) * b(1, 1) + a(1, 2) * b(2, 1);
  m(1, 2) = a(1, 0) * b(0, 2) + a(1, 1) * b(1, 2) + a(1, 2) * b(2, 2);

  m(2, 0) = a(2, 0) * b(0, 0) + a(2, 1) * b(1, 0) + a(2, 2) * b(2, 0);
  m(2, 1) = a(2, 0) * b(0, 1) + a(2, 1) * b(1, 1) + a(2, 2) * b(2, 1);
  m(2, 2) = a(2, 0) * b(0, 2) + a(2, 1) * b(1, 2) + a(2, 2) * b(2, 2);

  return m;
}

template <typename T>
Vector3<T> operator*(const Matrix3<T> &a, const Vector3<T> &v)
{
  Vector3<T> r;

  r.x() = a(0, 0) * v[0] + a(0, 1) * v[1] + a(0, 2) * v[2];
  r.y() = a(1, 0) * v[0] + a(1, 1) * v[1] + a(1, 2) * v[2];
  r.z() = a(2, 0) * v[0] + a(2, 1) * v[1] + a(2, 2) * v[2];

  return r;
}


template <typename T>
const Matrix3<T> Matrix3<T>::Zero(0, 0, 0, 0, 0, 0, 0, 0, 0);
template <typename T>
const Matrix3<T> Matrix3<T>::Identity(1, 0, 0, 0, 1, 0, 0, 0, 1);

template <typename T>
Matrix3<T>::Matrix3(const T array9[9])  // NOLINT(modernize-avoid-c-arrays)
{
  for (int i = 0; i < 9; ++i)
  {
    _storage[i] = array9[i];
  }
}

template <typename T>
Matrix3<T>::Matrix3(const StorageType &array) noexcept
  : _storage(array)
{}

template <typename T>
template <typename U>
Matrix3<T>::Matrix3(const std::array<U, 9> &array) noexcept
{
  for (size_t i = 0; i < array.size(); ++i)
  {
    _storage[i] = static_cast<T>(array[i]);
  }
}

template <typename T>
template <typename U>
Matrix3<T>::Matrix3(const Matrix3<U> &other) noexcept
{
  for (int i = 0; i < 9; ++i)
  {
    _storage[i] = static_cast<T>(other._storage[i]);
  }
}

template <typename T>
Matrix3<T>::Matrix3(const T &rc00, const T &rc01, const T &rc02, const T &rc10, const T &rc11,
                    const T &rc12, const T &rc20, const T &rc21, const T &rc22) noexcept
{
  (*this)(0, 0) = rc00;
  (*this)(0, 1) = rc01;
  (*this)(0, 2) = rc02;
  (*this)(1, 0) = rc10;
  (*this)(1, 1) = rc11;
  (*this)(1, 2) = rc12;
  (*this)(2, 0) = rc20;
  (*this)(2, 1) = rc21;
  (*this)(2, 2) = rc22;
}

template <typename T>
Matrix3<T> Matrix3<T>::rotationX(const T &angle)
{
  Matrix3<T> m = Identity;
  T s = std::sin(angle);
  T c = std::cos(angle);
  m[4] = m[8] = c;
  m[5] = -s;
  m[7] = s;
  return m;
}

template <typename T>
inline Matrix3<T> Matrix3<T>::rotationY(const T &angle)
{
  Matrix3<T> m = Identity;
  T s = std::sin(angle);
  T c = std::cos(angle);
  m[0] = m[8] = c;
  m[6] = -s;
  m[2] = s;
  return m;
}

template <typename T>
inline Matrix3<T> Matrix3<T>::rotationZ(const T &angle)
{
  Matrix3<T> m = Identity;
  T s = std::sin(angle);
  T c = std::cos(angle);
  m[0] = m[4] = c;
  m[1] = -s;
  m[3] = s;
  return m;
}

template <typename T>
inline Matrix3<T> Matrix3<T>::rotation(const T &x, const T &y, const T &z)
{
  Matrix3<T> m = rotationZ(x);
  m = rotationX(y) * m;
  m = rotationZ(z) * m;
  return m;
}

template <typename T>
inline Matrix3<T> Matrix3<T>::scaling(const Vector3<T> &scale)
{
  Matrix3<T> m = Identity;
  m(0, 0) = scale.x();
  m(1, 1) = scale.y();
  m(2, 2) = scale.z();
  return m;
}

template <typename T>
Matrix3<T> Matrix3<T>::lookAt(const Vector3<T> &eye, const Vector3<T> &target,
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

  Matrix3<T> m = Identity;
  m.setAxis(side_axis_index, axes[side_axis_index]);
  m.setAxis(forward_axis_index, axes[forward_axis_index]);
  m.setAxis(up_axis_index, axes[up_axis_index]);

  return m;
}

template <typename T>
inline Matrix3<T> &Matrix3<T>::transpose()
{
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
  return *this;
}

template <typename T>
inline Matrix3<T> Matrix3<T>::transposed() const
{
  const Matrix3<T> m((*this)(0, 0), (*this)(1, 0), (*this)(2, 0), (*this)(0, 1), (*this)(1, 1),
                     (*this)(2, 1), (*this)(0, 2), (*this)(1, 2), (*this)(2, 2));
  return m;
}

template <typename T>
Matrix3<T> &Matrix3<T>::invert()
{
  Matrix3<T> adj;
  const T det = getAdjoint(adj);
  const T det_inv = static_cast<T>(1) / det;

  for (int i = 0; i < 9; ++i)
  {
    _storage[i] = adj[i] * det_inv;
  }

  return *this;
}

template <typename T>
Matrix3<T> Matrix3<T>::inverse() const
{
  Matrix3<T> inv;
  const T det = getAdjoint(inv);
  const T det_inv = static_cast<T>(1) / det;

  for (int i = 0; i < 9; ++i)
  {
    inv[i] *= det_inv;
  }

  return inv;
}

template <typename T>
T Matrix3<T>::getAdjoint(Matrix3<T> &adj) const
{
  adj._storage[0] = _storage[4] * _storage[8] - _storage[7] * _storage[5];
  adj._storage[1] = _storage[7] * _storage[2] - _storage[1] * _storage[8];
  adj._storage[2] = _storage[1] * _storage[5] - _storage[4] * _storage[2];
  adj._storage[3] = _storage[6] * _storage[5] - _storage[3] * _storage[8];
  adj._storage[4] = _storage[0] * _storage[8] - _storage[6] * _storage[2];
  adj._storage[5] = _storage[3] * _storage[2] - _storage[0] * _storage[5];
  adj._storage[6] = _storage[3] * _storage[7] - _storage[6] * _storage[4];
  adj._storage[7] = _storage[6] * _storage[1] - _storage[0] * _storage[7];
  adj._storage[8] = _storage[0] * _storage[4] - _storage[3] * _storage[1];

  return _storage[0] * adj._storage[0] + _storage[1] * adj._storage[3] +
         _storage[2] * adj._storage[6];
}

template <typename T>
inline Matrix3<T> &Matrix3<T>::rigidBodyInvert()
{
  return transpose();
}

template <typename T>
inline Matrix3<T> Matrix3<T>::rigidBodyInverse() const
{
  return transposed();
}

template <typename T>
inline T Matrix3<T>::determinant() const
{
  const T det = _storage[0] * _storage[4] * _storage[8] + _storage[1] * _storage[5] * _storage[6] +
                _storage[2] * _storage[3] * _storage[7] - _storage[2] * _storage[4] * _storage[6] -
                _storage[1] * _storage[3] * _storage[8] - _storage[0] * _storage[5] * _storage[7];
  return det;
}

template <typename T>
inline Vector3<T> Matrix3<T>::axisX() const
{
  return axis(0);
}

template <typename T>
inline Vector3<T> Matrix3<T>::axisY() const
{
  return axis(1);
}

template <typename T>
inline Vector3<T> Matrix3<T>::axisZ() const
{
  return axis(2);
}

template <typename T>
inline Vector3<T> Matrix3<T>::axis(int index) const
{
  const Vector3<T> v((*this)(0, index), (*this)(1, index), (*this)(2, index));
  return v;
}

template <typename T>
inline Matrix3<T> &Matrix3<T>::setAxisX(const Vector3<T> &axis)
{
  return setAxis(0, axis);
}

template <typename T>
inline Matrix3<T> &Matrix3<T>::setAxisY(const Vector3<T> &axis)
{
  return setAxis(1, axis);
}

template <typename T>
inline Matrix3<T> &Matrix3<T>::setAxisZ(const Vector3<T> &axis)
{
  return setAxis(2, axis);
}

template <typename T>
inline Matrix3<T> &Matrix3<T>::setAxis(int index, const Vector3<T> &axis)
{
  (*this)(0, index) = axis.x();
  (*this)(1, index) = axis.y();
  (*this)(2, index) = axis.z();
  return *this;
}

template <typename T>
inline Vector3<T> Matrix3<T>::scale() const
{
  const Vector3<T> v(axisX().magnitude(), axisY().magnitude(), axisZ().magnitude());
  return v;
}

template <typename T>
inline Matrix3<T> &Matrix3<T>::scale(const Vector3<T> &scaling)
{
  (*this)(0, 0) *= scaling.x();
  (*this)(1, 0) *= scaling.x();
  (*this)(2, 0) *= scaling.x();

  (*this)(0, 1) *= scaling.y();
  (*this)(1, 1) *= scaling.y();
  (*this)(2, 1) *= scaling.y();

  (*this)(0, 2) *= scaling.z();
  (*this)(1, 2) *= scaling.z();
  (*this)(2, 2) *= scaling.z();

  return *this;
}

template <typename T>
inline Vector3<T> Matrix3<T>::transform(const Vector3<T> &v) const
{
  return *this * v;
}

template <typename T>
inline Vector3<T> Matrix3<T>::rotate(const Vector3<T> &v) const
{
  Vector3<T> r;

  r.x() = (*this)(0, 0) * v[0] + (*this)(0, 1) * v[1] + (*this)(0, 2) * v[2];
  r.y() = (*this)(1, 0) * v[0] + (*this)(1, 1) * v[1] + (*this)(1, 2) * v[2];
  r.z() = (*this)(2, 0) * v[0] + (*this)(2, 1) * v[1] + (*this)(2, 2) * v[2];

  return r;
}


template <typename T>
inline bool Matrix3<T>::isEqual(const Matrix3<T> &a, const T epsilon) const
{
  return std::abs(_storage[0] - a._storage[0]) <= epsilon &&
         std::abs(_storage[1] - a._storage[1]) <= epsilon &&
         std::abs(_storage[2] - a._storage[2]) <= epsilon &&
         std::abs(_storage[3] - a._storage[3]) <= epsilon &&
         std::abs(_storage[4] - a._storage[4]) <= epsilon &&
         std::abs(_storage[5] - a._storage[5]) <= epsilon &&
         std::abs(_storage[6] - a._storage[6]) <= epsilon &&
         std::abs(_storage[7] - a._storage[7]) <= epsilon &&
         std::abs(_storage[8] - a._storage[8]) <= epsilon;
}

// NOLINTEND(readability-identifier-length)
}  // namespace tes
