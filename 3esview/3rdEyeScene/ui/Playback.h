//
// Author: Kazys Stepanas
//
#ifndef TRD_EYE_SCENE_UI_PLAYBACK_H
#define TRD_EYE_SCENE_UI_PLAYBACK_H

#include <3rdEyeScene/ClientConfig.h>

#include "Panel.h"

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
    PauseResume,
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
  Playback(Viewer &viewer)
    : Panel(viewer)
  {}

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
  /// Draw a button associated with the given action.
  /// @param action The button action.
  /// @param label The button label.
  bool button(Action action, const char *label);
  /// Select a button from the list of @p candidates, using the first admissible option.
  /// @param candidates The button candidates.
  bool button(std::initializer_list<std::pair<Action, const char *>> candidates);

  using ActionSet =
    std::array<std::shared_ptr<command::Command>, static_cast<unsigned>(Action::Count)>;

  ActionSet _actions;
};
}  // namespace tes::view::ui

#endif  // TRD_EYE_SCENE_UI_PLAYBACK_H
