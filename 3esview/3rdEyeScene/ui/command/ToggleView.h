#ifndef TRD_EYE_SCENE_UI_COMMAND_TOGGLE_VIEW_H
#define TRD_EYE_SCENE_UI_COMMAND_TOGGLE_VIEW_H

#include <3rdEyeScene/ClientConfig.h>

#include <3esview/command/Command.h>

#include <3rdEyeScene/ui/IconBar.h>

namespace tes::view::ui::command
{
class TES_VIEWER_API ToggleView : public tes::view::command::Command
{
public:
  using Args = tes::view::command::Args;
  using CommandResult = tes::view::command::CommandResult;

  ToggleView(const std::string &name, ui::IconBar &icon_bar, ui::IconBar::View view);

protected:
  bool checkAdmissible(Viewer &viewer) const override;
  CommandResult invoke(Viewer &viewer, const ExecInfo &info, const Args &args) override;

  ui::IconBar &_icon_bar;
  ui::IconBar::View _view = ui::IconBar::View::Count;
};
}  // namespace tes::view::ui::command

#endif  // TRD_EYE_SCENE_UI_COMMAND_TOGGLE_VIEW_H
