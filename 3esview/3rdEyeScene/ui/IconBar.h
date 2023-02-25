//
// Author: Kazys Stepanas
//
#ifndef TRD_EYE_SCENE_UI_ICON_BAR_H
#define TRD_EYE_SCENE_UI_ICON_BAR_H

#include <3rdEyeScene/ClientConfig.h>

#include "Panel.h"

#include <Magnum/GL/Texture.h>

#include <array>
#include <memory>

namespace tes::view
{
class DataThread;
}

namespace tes::view::command
{
class Command;
}

namespace tes::view::ui
{
class IconBar : public Panel
{
public:
  constexpr static int kButtonSize = 24;
  constexpr static int kPanelSize = 3 * kButtonSize;

  /// An enumeration of the actions which can be triggered by the playback bar.
  ///
  /// @c Command objects are to be registered with each action to effect those actions.
  enum Action : unsigned
  {
    /// Open settings dialog
    Settings,
    /// Open connection dialog
    Connect,
    /// Open categories display
    Categories,
    /// Show log
    Log,

    /// Number of @c Actions - used for array sizes.
    Count
  };

  IconBar(Viewer &viewer);

  void draw(Magnum::ImGuiIntegration::Context &ui) override;

private:
  void initialiseIcons();

  using ActionIcons = std::array<Magnum::GL::Texture2D, static_cast<unsigned>(Action::Count)>;

  ActionIcons _icons;
};
}  // namespace tes::view::ui

#endif  // TRD_EYE_SCENE_UI_ICON_BAR_H
