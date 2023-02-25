#ifndef TRD_EYE_SCENE_UI_COMMAND_TOGGLE_LOG_H
#define TRD_EYE_SCENE_UI_COMMAND_TOGGLE_LOG_H

#include <3rdEyeScene/ClientConfig.h>

#include "ToggleView.h"

namespace tes::view::ui::command
{
class TES_VIEWER_API ToggleLog : public ToggleView
{
public:
  ToggleLog(ui::IconBar &icon_bar);
};
}  // namespace tes::view::ui::command

#endif  // TRD_EYE_SCENE_UI_COMMAND_TOGGLE_LOG_H
