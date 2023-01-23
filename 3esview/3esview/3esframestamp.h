#ifndef TES_VIEWER_FRAME_STAMP_H
#define TES_VIEWER_FRAME_STAMP_H

#include "3esview/Viewer.h"

#include <cinttypes>
#include <atomic>

namespace tes::viewer
{
using FrameNumber = uint32_t;
using RenderStamp = uint32_t;
using FrameNumberAtomic = std::atomic_uint32_t;

/// The @c FrameStamp represents a rendered moment in time.
///
/// The @c frame_number identifies the logical frame being rendered as dictated by the data stream. This value is only
/// monotonic increasing during normal playback, but can jump around during step back and skip operations.
///
/// The @c render_mark is a monotonic increasing value which changes with every rendering of the scene to the current
/// camera, even when the logical frame stays fixed.
struct TES_VIEWER_API FrameStamp
{
  /// The logical frame number as dictated by the data stream.
  FrameNumber frame_number = 0;
  /// A monotonic increasing value, changing with every rendering of the scene.
  RenderStamp render_mark = 0;
};
}  // namespace tes::viewer

#endif  // TES_VIEWER_FRAME_STAMP_H
