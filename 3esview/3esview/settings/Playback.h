//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SETTINGS_PLAYBACK_H
#define TES_VIEW_SETTINGS_PLAYBACK_H

#include <3esview/ViewConfig.h>

#include "Values.h"

namespace tes::view::settings
{
struct Playback
{
  Bool allow_key_frames = {
    "Allow key frames", true, "Enable scene keyframes to cache frames during playback and stepping?"
  };
  UInt keyframe_every_mib = {
    "Keyframe every MiB", 20, 1, 1024 * 1024,
    "Create a keyframe after reading this many mibibytes from the playback stream."
  };
  UInt keyframe_every_frames = { "Key frame separation", 5000, 1, 100000,
                                 "Create a keyframe after this number of frames elapses." };
  UInt keyframe_skip_forward_frames = {
    "Key frame minimum separation", 5, 1, 1000,
    "Minimum number of frames to have elapsed between keyframes."
  };
  Bool keyframe_compression = { "Keyframe compression", true, "Compress key frames?" };
  Bool looping = { "Looping", false,
                   "Automatically restart playback at the end of a file stream?" };
  Bool pause_on_error = { "Pause on error", true,
                          "Pause if an error occurs during playback? Only affects file playback." };
};
}  // namespace tes::view::settings

#endif  // TES_VIEW_SETTINGS_PLAYBACK_H
