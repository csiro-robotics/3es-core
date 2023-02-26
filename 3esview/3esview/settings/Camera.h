//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SETTINGS_CAMERA_H
#define TES_VIEW_SETTINGS_CAMERA_H

#include <3esview/ViewConfig.h>

#include "Values.h"

namespace tes::view::settings
{
struct Camera
{
  Bool invert_y = { "Invert Y", false, "Invert mouse Y axis?" };
  Bool allow_remote_settings = { "Allow remote", true,
                                 "Use remote clip plane and field of view settings?" };
  Float near_clip = { "Near clip", 0.1f, 0.0f, 100.0f,
                      "The default near clip plane when not using remote settings." };
  Float far_clip = { "Far clip", 2000.0f, 0.0f, 3000.0f,
                     "The default far clip plane when not using remote settings." };
  Float fov = { "Field of view", 60.0f, 0.0f, 180.0f,
                "The default horizontal field of view (degrees)." };
};
}  // namespace tes::view::settings

#endif  // TES_VIEW_SETTINGS_CAMERA_H
