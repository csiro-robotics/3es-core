#include "ToggleSettings.h"

namespace tes::view::ui::command
{
ToggleSettings::ToggleSettings(ui::IconBar &icon_bar)
  : ToggleView("toggleSettings", icon_bar, ui::IconBar::View::Settings)
{}
}  // namespace tes::view::ui::command
