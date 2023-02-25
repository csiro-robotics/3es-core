#ifndef TRD_EYE_SCENE_UI_COMMAND_TOGGLE_SETTINGS_H
#define TRD_EYE_SCENE_UI_COMMAND_TOGGLE_SETTINGS_H

#include <3rdEyeScene/ClientConfig.h>

#include "ToggleView.h"

namespace tes::view::ui::command
{
class TES_VIEWER_API ToggleSettings : public ToggleView
{
public:
  ToggleSettings(ui::IconBar &icon_bar);
};
}  // namespace tes::view::ui::command

#endif  // TRD_EYE_SCENE_UI_COMMAND_TOGGLE_SETTINGS_H
