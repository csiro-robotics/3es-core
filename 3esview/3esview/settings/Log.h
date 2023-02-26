//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SETTINGS_LOG_H
#define TES_VIEW_SETTINGS_LOG_H

#include <3esview/ViewConfig.h>

#include "Values.h"

namespace tes::view::settings
{
struct Log
{
  UInt log_window_size = { "Window size", 300, 0, 1000, "Size of the log window." };
};
}  // namespace tes::view::settings

#endif  // TES_VIEW_SETTINGS_LOG_H
