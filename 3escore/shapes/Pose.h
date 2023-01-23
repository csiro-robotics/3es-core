//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SHAPES_POSE_H
#define TES_CORE_SHAPES_POSE_H

#include <3escore/CoreConfig.h>

#include "Shape.h"

namespace tes
{
/// Defines a pose (set of axes) shape for remote rendering.
///
/// A pose is represented by a set of axis arrows or lines, coloured RBG corresponding to XYZ.
///
/// Setting the shape colour tints the axis colours.
class TES_CORE_API Pose : public Shape
{
public:
  /// Construct a box object.
  /// @param id The shape id and category, with unique id among @c Pose objects, or zero for a transient shape.
  /// @param transform The pose transformation matrix.
  Pose(const Id &id = Id(), const Transform &transform = Transform());

  /// Copy constructor
  /// @param other Object to copy.
  Pose(const Pose &other);

  inline const char *type() const override { return "pose"; }
};


inline Pose::Pose(const Id &id, const Transform &transform)
  : Shape(SIdPose, id, transform)
{}


inline Pose::Pose(const Pose &other)
  : Shape(other)
{}
}  // namespace tes

#endif  // TES_CORE_SHAPES_POSE_H
