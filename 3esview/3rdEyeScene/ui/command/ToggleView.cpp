#include "ToggleView.h"

#include <3esview/Viewer.h>
#include <3esview/data/DataThread.h>

namespace tes::view::ui::command
{
ToggleView::ToggleView(const std::string &name, ui::IconBar &icon_bar, ui::IconBar::View view)
  : tes::view::command::Command(name, Args())
  , _icon_bar(icon_bar)
  , _view(view)
{}


bool ToggleView::checkAdmissible(Viewer &viewer) const
{
  TES_UNUSED(viewer);
  return true;
}


tes::view::command::CommandResult ToggleView::invoke(Viewer &viewer, const ExecInfo &info,
                                                     const Args &args)
{
  TES_UNUSED(viewer);
  TES_UNUSED(info);
  TES_UNUSED(args);

  if (_view == ui::IconBar::View::Count)
  {
    return { CommandResult::Code::Invalid };
  }

  if (!_icon_bar.isActive(_view))
  {
    _icon_bar.setActive(_view);
  }
  else
  {
    _icon_bar.closeActiveView();
  }

  return { CommandResult::Code::Ok };
}
}  // namespace tes::view::ui::command
