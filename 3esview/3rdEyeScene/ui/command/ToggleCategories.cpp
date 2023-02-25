#include "ToggleCategories.h"

namespace tes::view::ui::command
{
ToggleCategories::ToggleCategories(ui::IconBar &icon_bar)
  : ToggleView("toggleCategories", icon_bar, ui::IconBar::View::Categories)
{}
}  // namespace tes::view::ui::command
