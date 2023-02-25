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
#include <optional>

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

  struct ButtonParams
  {
    /// The button action to represent. Determines the icon unless using @c icon_alias .
    Action action = Action::Count;
    /// When valid, use this action's icon instead of that belonging to @c action .
    Action icon_alias = Action::Count;
    /// Button label - fallback for no icon.
    const char *label;

    ButtonParams() = default;
    ButtonParams(Action action, const char *label)
      : action(action)
      , label(label)
    {}
    ButtonParams(Action action, Action icon_alias, const char *label)
      : action(action)
      , icon_alias(icon_alias)
      , label(label)
    {}
    ButtonParams(const ButtonParams &other) = default;
    ButtonParams(ButtonParams &&other) = default;

    ButtonParams &operator=(const ButtonParams &other) = default;
    ButtonParams &operator=(ButtonParams &&other) = default;
  };

  void drawButtons(DataThread *data_thread);
  void drawFrameSlider(DataThread *data_thread);

  /// Draw a button associated with the given action.
  /// @param params Details of the button.
  /// @param allow_inactive When true, darws the action icon as inactive, otherwise draws nothing.
  ButtonResult button(const ButtonParams &params, bool allow_inactive = true);
  /// Select a button from the list of @p candidates, using the first admissible option.
  /// @param candidates The button candidates.
  ButtonResult button(std::initializer_list<ButtonParams> candidates);

  void initialiseIcons();

  using ActionSet =
    std::array<std::shared_ptr<command::Command>, static_cast<unsigned>(Action::Count)>;
  using ActionIconNames = std::array<std::string, static_cast<unsigned>(Action::Count)>;
  using ActionIcons = std::array<Magnum::GL::Texture2D, static_cast<unsigned>(Action::Count)>;

  ActionSet _actions;
  ActionIcons _action_icons;
  std::weak_ptr<command::Command> _set_speed_command;
  std::weak_ptr<command::Command> _set_frame_command;
  /// Frame number while being edited.
  std::optional<int> _pending_frame;
  std::optional<float> _pending_speed;

  static const ActionIconNames &actionIconNames();
};
}  // namespace tes::view::ui

#endif  // TRD_EYE_SCENE_UI_PLAYBACK_H
