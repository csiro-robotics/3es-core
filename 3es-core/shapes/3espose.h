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
  /// @overload
  Pose(uint32_t id = 0u, const V3Arg &pos = V3Arg(0, 0, 0), const V3Arg &scale = V3Arg(1.0f, 1.0f, 1.0f),
      const QuaternionArg &rot = QuaternionArg(0, 0, 0, 1));
  /// Construct a pose object.
  /// @param id The shape ID, unique among @c Pose objects, or zero for a transient shape.
  /// @param category The category grouping for the shape used for filtering.
  /// @param pos Marks the centre position of the pose.
  /// @param scale Defines the size of the pose, were (1, 1, 1) denotes a pose with unit arms.
  /// @param rot Quaternion rotation to apply to the pose.
  Pose(uint32_t id, uint16_t category, const V3Arg &pos = V3Arg(0, 0, 0), const V3Arg &scale = V3Arg(1, 1, 1),
      const QuaternionArg &rot = QuaternionArg(0, 0, 0, 1));

  inline const char *type() const override { return "pose"; }
};


inline Pose::Pose(uint32_t id, const V3Arg &pos, const V3Arg &scale, const QuaternionArg &rot)
  : Shape(SIdPose, id)
{
  setPosition(pos);
  setRotation(rot);
  setScale(scale);
}


inline Pose::Pose(uint32_t id, uint16_t category, const V3Arg &pos, const V3Arg &scale, const QuaternionArg &rot)
  : Shape(SIdPose, id, category)
{
  setPosition(pos);
  setRotation(rot);
  setScale(scale);
}
}  // namespace tes

#endif  // _3ESPOSE_H_
