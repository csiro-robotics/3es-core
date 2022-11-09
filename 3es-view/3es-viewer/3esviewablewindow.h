#ifndef TES_VIEWER_VIEWABLE_WINDOW_H
#define TES_VIEWER_VIEWABLE_WINDOW_H

#include "3es-viewer.h"

#include <limits>

namespace tes::viewer
{
/// Defines a viewable window in terms of a frame number and frame count or number of frames to stay visible for.
///
/// Viewable windows are used to specify both the frames a shape may be visible for and the window a shape drawer should
/// display.
class ViewableWindow
{
public:
  inline ViewableWindow() = default;
  inline ViewableWindow(unsigned start_frame, unsigned frame_count)
    : _start_frame(start_frame)
    , _frame_count(frame_count)
  {}
  /// Define an open window.
  /// @param start_frame
  inline explicit ViewableWindow(unsigned start_frame)
    : ViewableWindow(start_frame, 0)
  {}
  inline ViewableWindow(const ViewableWindow &other) = default;
  inline ViewableWindow(ViewableWindow &&other) = default;

  inline ViewableWindow &operator=(const ViewableWindow &other) = default;
  inline ViewableWindow &operator=(ViewableWindow &&other) = default;

  inline bool operator==(const ViewableWindow &other) const
  {
    return _start_frame == other._start_frame && _frame_count == other._frame_count;
  }

  inline bool operator!=(const ViewableWindow &other) const { return !operator==(other); }

  /// Get the first frame number of this window.
  /// @return The first viewable frame.
  inline unsigned startFrame() const { return _start_frame; }
  /// Get the last viewable frame number of this window.
  ///
  /// @note For an open window, this is always @c std::numeric_limits<decltype(_frame_count)>::max() .
  /// For a single frame window, this is the same as @c startFrame() .
  ///
  /// @return The last viewable frame.
  inline unsigned lastFrame() const
  {
    return (!isOpen()) ? _start_frame + _frame_count - 1 : std::numeric_limits<decltype(_frame_count)>::max();
  }
  /// Get the number of frames covered by the window.
  ///
  /// @note For an open window, this is always @c std::numeric_limits<decltype(_frame_count)>::max() .
  /// @return The number of frames for which the window is viewable.
  inline unsigned frameCount() const
  {
    return (!isOpen()) ? _frame_count : std::numeric_limits<decltype(_frame_count)>::max();
  }

  /// Check if this defines an open window, which which starts at the @c frameNumber() and stays viewable.
  /// @return True if this defines an open window.
  inline bool isOpen() const { return _frame_count == 0; }

  /// Check if this window overlaps with @p other.
  /// @param other The window to check for overlap with.
  /// @return True if the windows overlap.
  inline bool overlaps(const ViewableWindow &other) const
  {
    const unsigned relative_start = other._start_frame - _start_frame;
    const unsigned relative_end = other.lastFrame() - other._start_frame;
    return (isOpen() && relative_start <= 0 && (relative_end >= 0 || other.isOpen())) ||
           (!isOpen() && relative_start <= 0 && relative_end <= lastFrame() || other.isOpen());
  }

  /// Check if the given frame number overlaps this window.
  /// @param frame_number THe frame number to check.
  /// @return True if the @p frame_number overlap this window.
  inline bool overlaps(unsigned frame_number) const
  {
    return _start_frame <= frame_number && frame_number <= lastFrame();
  }

private:
  unsigned _start_frame = 0;
  unsigned _frame_count = 0;
};

}  // namespace tes::viewer

#endif  // TES_VIEWER_VIEWABLE_WINDOW_H
