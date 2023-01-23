//
// author: Kazys Stepanas
//
#ifndef TES_CORE_COORDINATE_FRAME_H
#define TES_CORE_COORDINATE_FRAME_H

#include "CoreConfig.h"

namespace tes
{
/// Enumerates various coordinate frames. Each frame specifies the global axes in
/// the order right, forward, up. The up axis may be negated, that is a positive
/// value is down, in which case the 'Neg' suffix is added.
///
/// Right handed coordinate frames come first with left handed frames being those
/// greater than or equal to @c CFLeft.
///
/// Examples:
/// Label | Right | Forward | Up  | Notes
/// ----- | ----- | ------- | --- | ----------------------------------------------
/// XYZ   | X     | Y       | Z   | A common extension of 2D Catesian coordinates.
/// XZY   | X     | Z       | Y   | The default in Unity 3D.
/// XZYNeg| X     | Z       | -Y  | A common camera space system.
enum CoordinateFrame
{
  // Right handled frames.
  XYZ,
  XZYNeg,
  YXZNeg,
  YZX,
  ZXY,
  ZYXNeg,
  // Left handed frames
  XYZNeg,
  XZY,
  YXZ,
  YZXNeg,
  ZXYNeg,
  ZYX,

  CFCount,
  CFLeft = XYZNeg
};
}  // namespace tes

#endif  // TES_CORE_COORDINATE_FRAME_H
