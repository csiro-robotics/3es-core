//
// author: Kazys Stepanas
//
#ifndef TES_CORE_TRANSFORM_H
#define TES_CORE_TRANSFORM_H

#include "CoreConfig.h"

#include "Matrix4.h"
#include "Quaternion.h"
#include "Rotation.h"
#include "Vector3.h"

namespace tes
{
/// Represents an affine transformation, split into position, rotation and scale components.
///
/// The @c Transform class is used in most @c Shape constructors to specify the transformation. It is setup to
/// handle different transform configurations, progressively specifying position, rotation and scale.
///
/// The @c Transform class stores double precision data, but stores a flag @c preferDoublePrecision() which records
/// how the @c Transform was constructed. This sets the preferred transfer mechanism when sending @c Shape transform
/// data. The flag value may be explicitly set.
///
/// Derivations of the @c Transform as used to configure shapes which do not require full scaling or rotation.
class TES_CORE_API Transform
{
public:
  /// Single precision constructor without rotation.
  /// @param pos Position coordinate.
  /// @param scale Scaling values.
  /// @param preferDouble Optional override for precision selection.
  Transform(const Vector3f &pos, const Vector3f &scale, bool preferDouble = false)
    : _rotation(0, 0, 0, 1)
    , _position(pos)
    , _scale(scale)
    , _preferDoublePrecision(preferDouble)
  {}

  /// Double precision constructor without rotation.
  /// @param pos Position coordinate.
  /// @param scale Scaling values.
  /// @param preferDouble Optional override for precision selection.
  Transform(const Vector3d &pos, const Vector3d &scale, bool preferDouble = true)
    : _rotation(0, 0, 0, 1)
    , _position(pos)
    , _scale(scale)
    , _preferDoublePrecision(preferDouble)
  {}

  /// Single precision constructor with rotation.
  /// @param pos Position coordinate.
  /// @param rot Rotation quaternion.
  /// @param scale Scaling values.
  /// @param preferDouble Optional override for precision selection.
  Transform(const Vector3f &pos = Vector3f(0, 0, 0), const Quaterniond &rot = Quaternionf(0, 0, 0, 1),
            const Vector3f &scale = Vector3f(1, 1, 1), bool preferDouble = false)
    : _rotation(rot)
    , _position(pos)
    , _scale(scale)
    , _preferDoublePrecision(preferDouble)
  {}

  /// Double precision constructor with rotation.
  /// @param pos Position coordinate.
  /// @param rot Rotation quaternion.
  /// @param scale Scaling values.
  /// @param preferDouble Optional override for precision selection.
  Transform(const Vector3d &pos, const Quaterniond &rot = Quaterniond(0, 0, 0, 1),
            const Vector3d &scale = Vector3d(1, 1, 1), bool preferDouble = true)
    : _rotation(rot)
    , _position(pos)
    , _scale(scale)
    , _preferDoublePrecision(preferDouble)
  {}

  /// Single precision constructor from @c Matrix4.
  /// @param matrix An affine transformation.
  /// @param preferDouble Optional override for precision selection.
  Transform(const Matrix4f &matrix, bool preferDouble = false)
    : _preferDoublePrecision(preferDouble)
  {
    transformToQuaternionTranslation(Matrix4d(matrix), _rotation, _position, _scale);
  }

  /// Double precision constructor from @c Matrix4.
  /// @param matrix An affine transformation.
  /// @param preferDouble Optional override for precision selection.
  Transform(const Matrix4d &matrix, bool preferDouble = true)
    : _preferDoublePrecision(preferDouble)
  {
    transformToQuaternionTranslation(matrix, _rotation, _position, _scale);
  }

  /// Get the position coordinate.
  /// @return Position coordinate.
  inline const Vector3d &position() const { return _position; }

  /// Set the position coordinate to @p pos.
  /// @param pos The new position coordinate.
  /// @return `*this` to allow operation chaining.
  inline Transform &setPosition(const Vector3d &pos)
  {
    _position = pos;
    return *this;
  }

  /// Get the rotation quaternion.
  /// @return Rotation quaternion.
  inline const Quaterniond &rotation() const { return _rotation; }

  /// Set the rotation quaternion coordinate to @p rot.
  /// @param rot The new rotation quaternion.
  /// @return `*this` to allow operation chaining.
  inline Transform &setRotation(const Quaterniond &rot)
  {
    _rotation = rot;
    return *this;
  }

  /// Get per axis scaling vetor.
  /// @return Scaling vector.
  inline const Vector3d &scale() const { return _scale; }

  /// Set per axis scaling to @p scale.
  /// @param scale The new scaling vector.
  /// @return `*this` to allow operation chaining.
  inline Transform &setScale(const Vector3d &scale)
  {
    _scale = scale;
    return *this;
  }

  /// Query the double or single precision preference.
  ///
  /// This value is used to determin the preferred transmission preision. It is either set implicitly on construction,
  /// selecting a value which matches the precision of the arguments given, or it may be explicitly set.
  /// @return @c true if double precision is preferred, @c false for single precision.
  inline bool preferDoublePrecision() const { return _preferDoublePrecision; }

  /// Set the preferred precision.
  /// @param preferDouble @c true to prefer double precision, @c false for single precision.
  /// @return `*this` to allow operation chaining.
  inline Transform &setPreferDoublePrecision(bool preferDouble)
  {
    _preferDoublePrecision = preferDouble;
    return *this;
  }

  /// Cast to @c Matrix4 operator.
  /// @return A @c Matrix4 equivalent to this @c Transform .
  template <typename real>
  operator Matrix4<real>() const;

  /// Create an identity @c Transform .
  /// @param preferDouble Sets the @c preferDoublePrecision() flag.
  /// @return An identity @c Transform
  inline static const Transform identity(bool preferDouble = true)
  {
    return Transform(Vector3d::zero, Quaterniond::identity, Vector3d::one, preferDouble);
  }

  /// Test if this @c Transform equals @c other to within @p epsilon error.
  ///
  /// The @p epsilon value is passed to the @c Vector3 and @c Quaternion class @c isEqual() functions.
  ///
  /// @param other The transform to compare to.
  /// @param epsilon Error tolerance.
  /// @return True if the transforms are equal to within the @p epsilon error.
  bool isEqual(const Transform &other, const double epsilon = Vector3d::Epsilon) const;

private:
  Quaterniond _rotation;
  Vector3d _position;
  Vector3d _scale;
  bool _preferDoublePrecision;
};

/// A specialisation of @c Transform designed to work with shapes which have a length, direction and radius.
/// This includes @c Arrow, @c Capsule, @c Cone, @c Cylinder, @c Plane (radius used to scale the plane rendering).
///
/// A @c Directional transform is constructed from a position and direction vector with optional radius and length.
/// The direction vector is converted into the @c rotation() value by calculating the @c Quaternion rotation from
/// @c DefaultDirection to the specified direction vector. The radius and length are stored in the @c scale() with
/// radius stored in X and Y components and length in Z.
class TES_CORE_API Directional : public Transform
{
public:
  /// Default direction used as a reference orientation for generating a quaternion rotation.
  ///
  /// The @c rotation() value is relative to this vector.
  ///
  /// The default is `(0, 0, 1)`
  static const Vector3d DefaultDirection;

  /// Default constructor, creating an identity transformation (single precision).
  inline Directional()
    : Transform()
  {}

  /// Single precision constructor.
  /// @param pos Position coordinate.
  /// @param dir The direction vector. Normal vector expected.
  /// @param radius The radius scaling, mapped to X and Y of @c scale()
  /// @param length The length scaling, mapped to Z of @c scale .
  /// @param preferDouble Optional override for precision selection.
  inline Directional(const Vector3f &pos, const Vector3f dir = Vector3f(DefaultDirection), float radius = 1.0f,
                     float length = 1.0f, bool preferDouble = false)
    : Transform(pos, directionToRotation(dir), Vector3f(radius, radius, length), preferDouble)
  {}

  /// Double precision constructor.
  /// @param pos Position coordinate.
  /// @param dir The direction vector. Normal vector expected.
  /// @param radius The radius scaling, mapped to X and Y of @c scale()
  /// @param length The length scaling, mapped to Z of @c scale .
  /// @param preferDouble Optional override for precision selection.
  inline Directional(const Vector3d &pos, const Vector3d dir = DefaultDirection, double radius = 1.0,
                     double length = 1.0, bool preferDouble = true)
    : Transform(pos, directionToRotation(dir), Vector3d(radius, radius, length), preferDouble)
  {}

  /// Single precision constructor aligned to @c DefaultDirection .
  /// @param pos Position coordinate.
  /// @param radius The radius scaling, mapped to X and Y of @c scale()
  /// @param length The length scaling, mapped to Z of @c scale .
  /// @param preferDouble Optional override for precision selection.
  inline Directional(const Vector3f &pos, float radius, float length = 1.0f, bool preferDouble = false)
    : Transform(pos, directionToRotation(DefaultDirection), Vector3f(radius, radius, length), preferDouble)
  {}

  /// Double precision constructor aligned to @c DefaultDirection .
  /// @param pos Position coordinate.
  /// @param radius The radius scaling, mapped to X and Y of @c scale()
  /// @param length The length scaling, mapped to Z of @c scale .
  /// @param preferDouble Optional override for precision selection.
  inline Directional(const Vector3d &pos, double radius, double length = 1.0, bool preferDouble = false)
    : Transform(pos, directionToRotation(DefaultDirection), Vector3d(radius, radius, length), preferDouble)
  {}

  /// Create an identity @c Directional transform .
  /// @param preferDouble Sets the @c preferDoublePrecision() flag.
  /// @return An identity @c Directional transform.
  inline static const Directional identity(bool preferDouble = true)
  {
    return Directional(Vector3d(0.0), DefaultDirection, 1, 1, preferDouble);
  }

  /// A utility function for calculating the @c Quaternion rotation from @c DefaultDirection to @c direction .
  /// @param direction The target direction vector. Normal vector expected.
  /// @return The rotation from @c DefaultDirection to @p direction .
  template <typename real>
  static Quaternion<real> directionToRotation(const Vector3<real> &direction)
  {
    Quaternion<real> rot;
    const Vector3<real> defaultDir(DefaultDirection);
    if (direction.dot(defaultDir) > -real(0.9998))
    {
      rot = Quaternion<real>(defaultDir, direction);
    }
    else
    {
      rot.setAxisAngle(Vector3<real>::axisx, real(M_PI));
    }
    return rot;
  }
};

/// A specialisation of @c Transform designed to work with shapes which have a radius.
/// This includes @c Sphere, @c Star.
///
/// The @c Spherical transform requires no rotation specification as uniform scaling is expected. The radius is applied
/// as a uniform scale factor.
///
/// @note Shapes supporting @c Spherical transform may also support a direct @c Transform with rotation and non-uniform
/// scaling. For example, a @c Sphere with rotation and non-uniform scaling generates an ellipsoid.
class TES_CORE_API Spherical : public Transform
{
public:
  /// Default constructor, creating an identity transformation (single precision).
  inline Spherical()
    : Transform()
  {}

  /// Single precision constructor.
  /// @param pos Position coordinate.
  /// @param radius The radius scaling, mapped to all axes of @c scale()
  /// @param preferDouble Optional override for precision selection.
  inline Spherical(const Vector3f &pos, float radius = 1.0f, bool preferDouble = false)
    : Transform(pos, Vector3f(radius), preferDouble)
  {}

  /// Double precision constructor.
  /// @param pos Position coordinate.
  /// @param radius The radius scaling, mapped to all axes of @c scale()
  /// @param preferDouble Optional override for precision selection.
  inline Spherical(const Vector3d &pos, double radius = 1.0, bool preferDouble = true)
    : Transform(pos, Vector3d(radius), preferDouble)
  {}

  /// Create an identity @c Spherical transform .
  /// @param preferDouble Sets the @c preferDoublePrecision() flag.
  /// @return An identity @c Spherical transform.
  inline static const Spherical identity(bool preferDouble = true) { return Spherical(Vector3d(0.0), 1, preferDouble); }
};


template <typename real>
inline Transform::operator Matrix4<real>() const
{
  return prsTransform<real>(Vector3<real>(_position), Quaternion<real>(_rotation), Vector3<real>(_scale));
}


inline bool Transform::isEqual(const Transform &other, const double epsilon) const
{
  return _rotation.isEqual(other._rotation, epsilon),
         _position.isEqual(other._position) && _scale.isEqual(other._scale, epsilon);
}
}  // namespace tes

#endif  // TES_CORE_TRANSFORM_H
