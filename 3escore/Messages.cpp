//
// author: Kazys Stepanas
//
#include "Messages.h"

#include <cstring>

void tes::initDefaultServerInfo(ServerInfoMessage *info)
{
  const uint32_t default_frame_step_ms = 33u;
  memset(info, 0, sizeof(*info));
  info->time_unit = 1000ull;
  info->default_frame_time = default_frame_step_ms;
  info->coordinate_frame = 0;
}
