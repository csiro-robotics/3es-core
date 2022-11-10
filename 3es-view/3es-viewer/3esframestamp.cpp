#include "3esframestamp.h"

namespace tes::viewer
{
namespace
{
FrameNumber g_frameWindow = 200u;
}

FrameNumber frameWindow()
{
  return g_frameWindow;
}
void setFrameWindow(FrameNumber window_size)
{
  g_frameWindow = window_size;
}
}  // namespace tes::viewer
