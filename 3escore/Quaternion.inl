//
// author: Kazys Stepanas
//

namespace tes
{
// NOLINTBEGIN(readability-identifier-length)
template <typename T>
inline Quaternion<T> operator*(const Quaternion<T> &a, const Quaternion<T> &b)
{
  Quaternion<T> q;
  q.x() = a.w() * b.x() + a.x() * b.w() + a.y() * b.z() - a.z() * b.y();
  q.y() = a.w() * b.y() - a.x() * b.z() + a.y() * b.w() + a.z() * b.x();
  q.z() = a.w() * b.z() + a.x() * b.y() - a.y() * b.x() + a.z() * b.w();
  q.w() = a.w() * b.w() - a.x() * b.x() - a.y() * b.y() - a.z() * b.z();
  return q;
}


template <typename T>
inline Vector3<T> operator*(const Quaternion<T> &a, const Vector3<T> &v)
{
  return a.transform(v);
}

template <typename T>
Quaternion<T>::Quaternion(const Vector3<T> &from, const Vector3<T> &to)
{
  Vector3<T> half = from + to;
  half.normalise();
  const Vector3<T> vec = from.cross(half);
  x() = vec.x();
  y() = vec.y();
  z() = vec.z();
  w() = from.dot(half);
}
// NOLINTEND(readability-identifier-length)


template <typename T>
bool Quaternion<T>::operator==(const Quaternion<T> &other) const
{
  return _storage == other._storage;
}


template <typename T>
bool Quaternion<T>::operator!=(const Quaternion<T> &other) const
{
  return _storage != other._storage;
}


template <typename T>
bool Quaternion<T>::isEqual(const Quaternion<T> &other, const T &epsilon) const
{
  return std::abs(x() - other.x()) <= epsilon && std::abs(y() - other.y()) <= epsilon &&
         std::abs(z() - other.z()) <= epsilon && std::abs(w() - other.w()) <= epsilon;
}


template <typename T>
bool Quaternion<T>::isIdentity()
{
  return *this == Identity;
}


template <typename T>
void Quaternion<T>::getAxisAngle(Vector3<T> &axis, T &angle) const
{
  T mag = x() * x() + y() * y() + z() * z();

  if (mag <= Vector3<T>::kEpsilon)
  {
    axis = Vector3<T>::AxisZ;
    angle = 0;
    return;
  }

  const T mag_inv = static_cast<T>(1) / mag;
  axis = Vector3<T>(x() * mag_inv, y() * mag_inv, z() * mag_inv);
  angle = static_cast<T>(2) * std::acos(w());
}


template <typename T>
Quaternion<T> &Quaternion<T>::setAxisAngle(const Vector3<T> &axis, const T &angle)
{
  T sin_half_angle = std::sin(static_cast<T>(0.5) * angle);
  w() = std::cos(static_cast<T>(0.5) * angle);
  x() = axis.x() * sin_half_angle;
  y() = axis.y() * sin_half_angle;
  z() = axis.z() * sin_half_angle;
  normalise();
  return *this;
}


template <typename T>
Quaternion<T> &Quaternion<T>::invert()
{
  T mag2 = magnitudeSquared();
  conjugate();
  *this *= static_cast<T>(1) / mag2;
  return *this;
}


template <typename T>
Quaternion<T> Quaternion<T>::inverse() const
{
  Quaternion<T> inv = *this;
  return inv.invert();
}


template <typename T>
Quaternion<T> &Quaternion<T>::conjugate()
{
  x() = -x();
  y() = -y();
  z() = -z();
  return *this;
}


template <typename T>
Quaternion<T> Quaternion<T>::conjugated() const
{
  Quaternion<T> conj = *this;
  return conj.conjugate();
}


template <typename T>
T Quaternion<T>::normalise(const T &epsilon)
{
  T mag = magnitude();
  if (mag <= epsilon)
  {
    *this = Identity;
    return 0;
  }

  const T mag_inv = static_cast<T>(1) / mag;
  x() *= mag_inv;
  y() *= mag_inv;
  z() *= mag_inv;
  w() *= mag_inv;

  return mag;
}


template <typename T>
Quaternion<T> Quaternion<T>::normalised(const T &epsilon) const
{
  Quaternion<T> norm = *this;
  norm.normalise(epsilon);
  return norm;
}


template <typename T>
T Quaternion<T>::magnitude() const
{
  return std::sqrt(magnitudeSquared());
}


template <typename T>
T Quaternion<T>::magnitudeSquared() const
{
  return x() * x() + y() * y() + z() * z() + w() * w();
}


template <typename T>
T Quaternion<T>::dot(const Quaternion<T> &other) const
{
  return x() * other.x() + y() * other.y() + z() * other.z() + w() * other.w();
}


template <typename T>
Vector3<T> Quaternion<T>::transform(const Vector3<T> &vec) const
{
  // NOLINTBEGIN(readability-identifier-length, readability-isolate-declaration)
  const T xx = x() * x(), xy = x() * y(), xz = x() * z(), xw = x() * w();
  const T yy = y() * y(), yz = y() * z(), yw = y() * w();
  const T zz = z() * z(), zw = z() * w();
  // NOLINTEND(readability-identifier-length, readability-isolate-declaration)

  Vector3<T> res;

  res.x() = (1 - 2 * (yy + zz)) * vec.x() + (2 * (xy - zw)) * vec.y() + (2 * (xz + yw)) * vec.z();
  res.y() = (2 * (xy + zw)) * vec.x() + (1 - 2 * (xx + zz)) * vec.y() + (2 * (yz - xw)) * vec.z();
  res.z() = (2 * (xz - yw)) * vec.x() + (2 * (yz + xw)) * vec.y() + (1 - 2 * (xx + yy)) * vec.z();

  return res;
}


template <typename T>
Quaternion<T> &Quaternion<T>::multiply(const T &scalar)
{
  x() *= scalar;
  y() *= scalar;
  z() *= scalar;
  w() *= scalar;
  return *this;
}


// NOLINTBEGIN(readability-identifier-length)
template <typename T>
Quaternion<T> Quaternion<T>::slerp(const Quaternion<T> &from, const Quaternion<T> &to, T t,
                                   T epsilon)
// NOLINTEND(readability-identifier-length)
{
  T coeff0 = 0;
  T coeff1 = 0;
  T angle = 0;
  T sin_val = 0;
  T cos_val = 0;
  T sin_inv = 0;

  if (from == to)
  {
    return from;
  }

  cos_val = from.dot(to);

  Quaternion<T> temp;

  // numerical round-off error could create problems in call to acos
  if (cos_val < static_cast<T>(0))
  {
    cos_val = -cos_val;
    temp.x() = -to.x();
    temp.y() = -to.y();
    temp.z() = -to.z();
    temp.w() = -to.w();
  }
  else
  {
    temp.x() = to.x();
    temp.y() = to.y();
    temp.z() = to.z();
    temp.w() = to.w();
  }

  if ((static_cast<T>(1) - cos_val) > epsilon)
  {
    angle = std::acos(cos_val);
    sin_val = std::sin(angle);  // fSin >= 0 since fCos >= 0

    sin_inv = static_cast<T>(1) / sin_val;
    coeff0 = std::sin((static_cast<T>(1) - t) * angle) * sin_inv;
    coeff1 = std::sin(t * angle) * sin_inv;
  }
  else
  {
    coeff0 = static_cast<T>(1) - t;
    coeff1 = t;
  }

  temp.x() = coeff0 * from.x() + coeff1 * temp.x();
  temp.y() = coeff0 * from.y() + coeff1 * temp.y();
  temp.z() = coeff0 * from.z() + coeff1 * temp.z();
  temp.w() = coeff0 * from.w() + coeff1 * temp.w();

  return temp;
}


// NOLINTBEGIN(readability-identifier-length)
template <typename T>
inline Quaternion<T> &Quaternion<T>::operator*=(const Quaternion<T> &other)
{
  const Quaternion<T> result = *this * other;
  *this = result;
  return *this;
}
// NOLINTEND(readability-identifier-length)


template <typename T>
const Quaternion<T> Quaternion<T>::Identity(0, 0, 0, 1);
}  // namespace tes
