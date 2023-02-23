//
// Author: Kazys Stepanas
//
#ifndef TRD_EYE_SCENE_UI_PLAYBACK_H
#define TRD_EYE_SCENE_UI_PLAYBACK_H

#include <3rdEyeScene/ClientConfig.h>

#include "Panel.h"

#include <Magnum/GL/Texture.h>

#include <array>
#include <initializer_list>
#include <memory>

namespace tes::view::command
{
class Command;
}

namespace tes::view::ui
{
class Playback : public Panel
{
public:
  constexpr static int kButtonSize = 24;
  constexpr static int kPanelSize = 3 * kButtonSize;

  /// An enumeration of the actions which can be triggered by the playback bar.
  ///
  /// @c Command objects are to be registered with each action to effect those actions.
  enum Action : unsigned
  {
    /// Open a recording stream.
    Record,
    /// Stop the current recording or playback stream.
    Stop,
    /// Open a playback stream.
    Play,
    /// Toggle pause.
    Pause,
    /// Skip back - to the start of the stream.
    SkipBack,
    /// Step back a frame.
    StepBack,
    /// Step forward a frame.
    StepForward,
    /// Skip forward - to the end of the stream.
    SkipForward,

    /// Number of @c Actions - used for array sizes.
    Count
  };

  // Playback(Magnum::Range2D rect)
  Playback(Viewer &viewer);

  void registerAction(Action action, std::shared_ptr<command::Command> command);
  std::shared_ptr<command::Command> command(Action action) const
  {
    if (action != Action::Count)
    {
      return _actions[static_cast<unsigned>(action)];
    }
    return {};
  }

  void draw(Magnum::ImGuiIntegration::Context &ui) override;

private:
  enum ButtonResult
  {
    Inactive,
    Ok,
    Pressed,
  };

  void frameSlider();

  /// Draw a button associated with the given action.
  /// @param action The button action.
  /// @param label The button label.
  ButtonResult button(Action action, const char *label, bool allow_inactive = true);
  /// Select a button from the list of @p candidates, using the first admissible option.
  /// @param candidates The button candidates.
  ButtonResult button(std::initializer_list<std::pair<Action, const char *>> candidates);

  void initialiseIcons();

  using ActionSet =
    std::array<std::shared_ptr<command::Command>, static_cast<unsigned>(Action::Count)>;
  using ActionIconNames = std::array<std::string, static_cast<unsigned>(Action::Count)>;
  using ActionIcons = std::array<Magnum::GL::Texture2D, static_cast<unsigned>(Action::Count)>;

  ActionSet _actions;
  ActionIcons _action_icons;
  std::weak_ptr<command::Command> _set_frame_command;

  static const ActionIconNames &actionIconNames();
};
}  // namespace tes::view::ui

#endif  // TRD_EYE_SCENE_UI_PLAYBACK_H
