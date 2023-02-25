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
  constexpr static int kPanelSize = 2 * kButtonSize;

  /// An enumeration of the actions which can be triggered by the playback bar.
  ///
  /// @c Command objects are to be registered with each action to effect those actions.
  enum View : unsigned
  {
    /// Open settings dialog
    Settings,
    /// Open connection dialog
    Connect,
    /// Open categories display
    Categories,
    /// Show log
    Log,

    /// Number of @c Views - used for array sizes.
    Count,
    /// Invalid value
    Invalid = Count
  };

  IconBar(Viewer &viewer);

  void closeActiveView();
  void setActive(View view);
  [[nodiscard]] View activeView() const;
  [[nodiscard]] bool isActive(View view) const;

  void registerCommand(View view, std::shared_ptr<tes::view::command::Command> command);

  void draw(Magnum::ImGuiIntegration::Context &ui) override;

private:
  void initialiseIcons();

  using ViewIcons = std::array<Magnum::GL::Texture2D, static_cast<unsigned>(View::Count)>;
  using ViewCommands =
    std::array<std::shared_ptr<tes::view::command::Command>, static_cast<unsigned>(View::Count)>;
  using ViewIconNames = std::array<std::string, static_cast<unsigned>(View::Count)>;

  static const IconBar::ViewIconNames &viewIconNames();

  ViewIcons _icons;
  ViewCommands _commands;
  View _active_view = View::Invalid;
};
}  // namespace tes::view::ui

#endif  // TRD_EYE_SCENE_UI_ICON_BAR_H
