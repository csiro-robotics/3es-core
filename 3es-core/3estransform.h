//
// author: Kazys Stepanas
//
#ifndef _3ESTRANSFORM_H
#define _3ESTRANSFORM_H

#include "3es-core.h"

#include "3esquaternion.h"
#include "3esvector3.h"
#include "3esmatrix4.h"
#include "3esrotation.h"

namespace tes
{
/// A helper argument used with shape construction to encapsulate various shape transformation argument combinations.
class Transform
{
public:
  Transform(const Vector3d &pos, const Vector3d &scale)
    : _rotation(0, 0, 0, 1)
    , _position(pos)
    , _scale(scale)
  {}

  Transform(const Vector3d &pos = Vector3d(0, 0, 0), const Quaterniond &rot = Quaterniond(0, 0, 0, 1),
            const Vector3d &scale = Vector3d(1, 1, 1))
    : _rotation(rot)
    , _position(pos)
    , _scale(scale)
  {}

  Transform(const Matrix4d &matrix)
  {
    transformToQuaternionTranslation(matrix, _rotation, _position, _scale);
  }

  inline const Vector3d &position() const { return _position; }
  inline void setPosition(const Vector3d &pos) { _position = pos; }
  inline const Quaterniond &rotation() const { return _rotation; }
  inline void setRotation(const Quaterniond &rot) { _rotation = rot; }
  inline const Vector3d &scale() const { return _scale; }
  inline void setScale(const Vector3d &scale) { _scale = scale; }

  inline static Transform identity() { return Transform(); }

private:
  Quaterniond _rotation;
  Vector3d _position;
  Vector3d _scale;
};
}  // namespace tes

#endif  // _3ESTRANSFORM_H
