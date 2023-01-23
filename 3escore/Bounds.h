//
// author: Kazys Stepanas
//
#ifndef _3ESBOUNDS_H
#define _3ESBOUNDS_H

#include "CoreConfig.h"

#include "Vector3.h"

#include <algorithm>
#include <limits>

namespace tes
{
/// A simple bounding box structure.
template <typename T>
class Bounds
{
public:
  /// Initialises bounds where max < min at using the limits of the type @c T.
  Bounds();

  /// Copy constructor.
  /// @param other The bounds to copy.
  Bounds(const Bounds<T> &other);

  /// Copy constructor from a different numeric type.
  /// The type @c Q must be compatible with @c T. Generally used to convert between
  /// single and double precision.
  /// @param other The bounds to copy.
  template <typename Q>
  Bounds(const Bounds<Q> &other);

  /// Initialise a bounding box with the given extents.
  /// @param minExt The bounding box minimum. All components must be less than or equal to
  ///     @p maxExtents.
  /// @param maxExt The bounding box maximum. All components must be greater than or equal to
  ///     @p minExtents.
  Bounds(const Vector3<T> &minExt, const Vector3<T> maxExt);

  /// Initialise the boudns to the given point.
  /// @param point The point to set as both min and max extents.
  explicit Bounds(const Vector3<T> &point);

  /// Create a bounds structure from centre and half extents values.
  /// @param centre The bounds centre.
  /// @param half_extents Bounds half extents.
  /// @return The bounds AABB.
  static Bounds fromCentreHalfExtents(const Vector3<T> &centre, const Vector3<T> &half_extents)
  {
    return { centre - half_extents, centre + half_extents };
  }

  /// Access the minimum extents.
  /// @return The minimal corder of the bounding box.
  const Vector3<T> &minimum() const;
  /// Access the maximum extents.
  /// @return The maximal corder of the bounding box.
  const Vector3<T> &maximum() const;

  /// Get the bounds centre point.
  /// @return The bounds centre.
  Vector3<T> centre() const;
  /// Get the bounds half extents, from centre to max.
  /// @return The half extents, centre to max.
  Vector3<T> halfExtents() const;

  /// Converts the bounds from defining an AABB to being more spherical in nature.
  ///
  /// This adjusts the bounds such that all axes of the half extents are set to the maximum axis value, while
  /// maintaining the same centre.
  ///
  /// Note, there is no way to explicitly identify that this adjustment has been made to a @c Bounds object.
  void convertToSpherical();

  /// Expand the bounding box to include @p point.
  /// @param point The point to include.
  void expand(const Vector3<T> &point);

  /// Expand the bounding box to include @p other.
  /// @param other The bounds to include.
  void expand(const Bounds<T> &other);

  /// Returns true if the bounds are valid, with minimum extents less than or equal to the
  /// maximum.
  /// @return True when valid.
  bool isValid() const;

  /// Precise equality operator.
  /// @param other The object to compare to.
  /// @return True if this is precisely equal to @p other.
  bool operator==(const Bounds<T> &other) const;

  /// Precise inequality operator.
  /// @param other The object to compare to.
  /// @return True if this is no precisely equal to @p other.
  bool operator!=(const Bounds<T> &other) const;

  /// Assignment operator.
  /// @param other The bounds to copy.
  /// @return @c this.
  Bounds<T> &operator=(const Bounds<T> &other);

private:
  Vector3<T> _minimum;  ///< Minimum extents.
  Vector3<T> _maximum;  ///< Maximum extents.
};

_3es_extern template class TES_CORE_API Bounds<float>;
_3es_extern template class TES_CORE_API Bounds<double>;

/// Single precision bounds.
typedef Bounds<float> Boundsf;
/// Double precision bounds.
typedef Bounds<double> Boundsd;

template <typename T>
inline Bounds<T>::Bounds()
  : _minimum(std::numeric_limits<T>::max())
  , _maximum(-std::numeric_limits<T>::max())
{}


template <typename T>
inline Bounds<T>::Bounds(const Bounds<T> &other)
{
  *this = other;
}


template <typename T>
template <typename Q>
inline Bounds<T>::Bounds(const Bounds<Q> &other)
{
  _minimum = Vector3<T>(other._minimum);
  _maximum = Vector3<T>(other._maximum);
}


template <typename T>
inline Bounds<T>::Bounds(const Vector3<T> &minExt, const Vector3<T> maxExt)
  : _minimum(minExt)
  , _maximum(maxExt)
{}


template <typename T>
inline Bounds<T>::Bounds(const Vector3<T> &point)
  : Bounds(point, point)
{}


template <typename T>
inline const Vector3<T> &Bounds<T>::minimum() const
{
  return _minimum;
}


template <typename T>
inline const Vector3<T> &Bounds<T>::maximum() const
{
  return _maximum;
}


template <typename T>
Vector3<T> Bounds<T>::centre() const
{
  return T(0.5) * (_minimum + _maximum);
}


template <typename T>
Vector3<T> Bounds<T>::halfExtents() const
{
  return _maximum - centre();
}


template <typename T>
inline void Bounds<T>::convertToSpherical()
{
  const auto centre = this->centre();
  auto ext = halfExtents();
  ext.x = std::max(ext.x, ext.y);
  ext.x = ext.y = ext.z = std::max(ext.x, ext.z);
  *this = fromCentreHalfExtents(centre, ext);
}


template <typename T>
inline void Bounds<T>::expand(const Vector3<T> &point)
{
  _minimum.x = (point.x < _minimum.x) ? point.x : _minimum.x;
  _minimum.y = (point.y < _minimum.y) ? point.y : _minimum.y;
  _minimum.z = (point.z < _minimum.z) ? point.z : _minimum.z;
  _maximum.x = (point.x > _maximum.x) ? point.x : _maximum.x;
  _maximum.y = (point.y > _maximum.y) ? point.y : _maximum.y;
  _maximum.z = (point.z > _maximum.z) ? point.z : _maximum.z;
}


template <typename T>
inline void Bounds<T>::expand(const Bounds<T> &other)
{
  expand(other.minimum());
  expand(other.maximum());
}


template <typename T>
inline bool Bounds<T>::isValid() const
{
  return _minimum.x <= _maximum.x && _minimum.y <= _maximum.y && _minimum.z <= _maximum.z;
}


template <typename T>
inline bool Bounds<T>::operator==(const Bounds<T> &other) const
{
  return _minimum.x == other._minimum.x && _minimum.y == other._minimum.y && _minimum.z == other._minimum.z &&
         _maximum.x == other._maximum.x && _maximum.y == other._maximum.y && _maximum.z == other._maximum.z;
}


template <typename T>
inline bool Bounds<T>::operator!=(const Bounds<T> &other) const
{
  return !operator==(other);
}


template <typename T>
inline Bounds<T> &Bounds<T>::operator=(const Bounds<T> &other)
{
  _minimum = other._minimum;
  _maximum = other._maximum;
  return *this;
}
}  // namespace tes


#endif  // _3ESBOUNDS_H
