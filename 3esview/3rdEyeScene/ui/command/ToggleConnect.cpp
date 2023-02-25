#include "ToggleConnect.h"

namespace tes::view::ui::command
{
ToggleConnect::ToggleConnect(ui::IconBar &icon_bar)
  : ToggleView("toggleConnect", icon_bar, ui::IconBar::View::Connect)
{}
}  // namespace tes::view::ui::command
