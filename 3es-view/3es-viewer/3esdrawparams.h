//
// Author: Kazys Stepanas
//
#ifndef TES_VIEWER_DRAW_PARAMS_H
#define TES_VIEWER_DRAW_PARAMS_H

#include "3es-viewer.h"

#include "camera/3escamera.h"

#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector2.h>

namespace tes::viewer
{
/// Render related parameters passed to the @c Message::draw() function.
struct TES_VIEWER_API DrawParams
{
  /// Current view camera.
  camera::Camera camera;
  /// The current projection matrix. This does not include the view matrix.
  Magnum::Matrix4 projection_matrix;
  /// The inverse of @p camera_matrix.
  Magnum::Matrix4 view_matrix;
  /// Represents the @c camera transform in the world.
  Magnum::Matrix4 camera_matrix;
  /// Size of the viewport being drawn to (pixels).
  Magnum::Vector2 view_size;
};
}  // namespace tes::viewer

#endif  // TES_VIEWER_DRAW_PARAMS_H
