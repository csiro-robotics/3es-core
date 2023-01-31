#include "FramesPerSecondWindow.h"

#include <numeric>

namespace tes::view
{
FramesPerSecondWindow::FramesPerSecondWindow(unsigned window_size)
  : _window(window_size, 0.0f)
{}


void FramesPerSecondWindow::push(float dt)
{
  const unsigned max_count = unsigned(_window.size());
  _window[_next] = dt;
  _next = (_next + 1) % max_count;
  _count = std::min(_count + 1, max_count);
}


float FramesPerSecondWindow::averageFrameTime() const
{
  const float elapsed = std::accumulate(_window.begin(), _window.end(), 0.0f);
  return elapsed / (_count > 0 ? float(_count) : 1.0f);
}


float FramesPerSecondWindow::fps() const
{
  const float average_frame_time = averageFrameTime();
  return (average_frame_time > 0) ? 1.0f / average_frame_time : average_frame_time;
}
}  // namespace tes::view
