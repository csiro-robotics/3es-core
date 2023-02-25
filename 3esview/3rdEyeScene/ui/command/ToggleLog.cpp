#include "ToggleLog.h"

namespace tes::view::ui::command
{
ToggleLog::ToggleLog(ui::IconBar &icon_bar)
  : ToggleView("toggleLog", icon_bar, ui::IconBar::View::Log)
{}
}  // namespace tes::view::ui::command
