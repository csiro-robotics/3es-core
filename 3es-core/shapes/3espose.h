//
// author: Kazys Stepanas
//
#ifndef _3ESPOSE_H_
#define _3ESPOSE_H_

#include "3es-core.h"
#include "3esshape.h"

namespace tes
{
/// Defines a pose (set of axes) shape for remote rendering.
///
/// A pose is represented by a set of axis arrows or lines, coloured RBG corresponding to XYZ.
///
/// Setting the shape colour tints the axis colours.
class _3es_coreAPI Pose : public Shape
{
public:
  /// Construct a box object.
  /// @param id The shape id and category, with unique id among @c Pose objects, or zero for a transient shape.
  /// @param transform The pose transformation matrix.
  Pose(const IdCat &id = IdCat(), const Transform &transform = Transform());

  inline const char *type() const override { return "pose"; }
};


inline Pose::Pose(const IdCat &id, const Transform &transform)
  : Shape(SIdPose, id, transform)
{
}
}  // namespace tes

#endif  // _3ESPOSE_H_
