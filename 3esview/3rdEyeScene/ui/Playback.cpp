//
// Author: Kazys Stepanas
//
#include "Playback.h"

#include <3escore/CoreUtil.h>
#include <3escore/Log.h>

#include <3esview/command/Set.h>
#include <3esview/data/DataThread.h>
#include <3esview/Viewer.h>

#include <Magnum/GL/TextureFormat.h>
#include <Magnum/ImageView.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>

#include <Corrade/Containers/Optional.h>
#include <Corrade/PluginManager/PluginManager.h>
#include <Corrade/Utility/Resource.h>

namespace tes::view::ui
{

Playback::Playback(Viewer &viewer)
  : Panel(viewer)
{
  initialiseIcons();
  registerAction(ui::Playback::Stop, viewer.commands()->lookupName("stop").command);
  registerAction(ui::Playback::Record, viewer.commands()->lookupName("record").command);
  registerAction(ui::Playback::Play, viewer.commands()->lookupName("openFile").command);
  registerAction(ui::Playback::Pause, viewer.commands()->lookupName("pause").command);
  registerAction(ui::Playback::SkipBack, viewer.commands()->lookupName("skipBackward").command);
  registerAction(ui::Playback::StepBack, viewer.commands()->lookupName("stepBackward").command);
  registerAction(ui::Playback::StepForward, viewer.commands()->lookupName("stepForward").command);
  registerAction(ui::Playback::SkipForward, viewer.commands()->lookupName("skipForward").command);
  _set_speed_command = viewer.commands()->lookupName("playbackSpeed").command;
  _set_frame_command = viewer.commands()->lookupName("skipToFrame").command;
}


void Playback::registerAction(Action action, std::shared_ptr<command::Command> command)
{
  if (action != Action::Count)
  {
    _actions[static_cast<unsigned>(action)] = std::move(command);
  }
}

void Playback::draw(Magnum::ImGuiIntegration::Context &ui)
{
  TES_UNUSED(ui);

  setNextWindowPos({ 0, -kPanelSize }, Anchor::BottomLeft);
  setNextWindowSize({ 0, kPanelSize }, Stretch::Horizontal);
  ImGui::Begin("Playback", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);


  auto data_thread = _viewer.dataThread();

  drawButtons(data_thread.get());
  drawFrameSlider(data_thread.get());

  auto pos = ImGui::GetWindowPos();

  ImGui::End();
}


void Playback::drawButtons(DataThread *data_thread)
{
  // Layout playback buttons horizontally grouped.
  const auto button_row_size = kButtonSize + 8;

  // Select which icon to use for the pause button based on paused state.
  const auto play_pause_icon =
    (data_thread && !data_thread->paused()) ? Action::Pause : Action::Play;

  ImGui::BeginChild("Playback buttons", ImVec2(uiViewportSize().x() * 0.75f, button_row_size));
  button({ { Action::Stop, "S" }, { Action::Record, "R" } });
  ImGui::SameLine();
  button({ { Action::Play, "P" }, { Action::Pause, play_pause_icon, "||" } });
  ImGui::SameLine();
  button({ Action::SkipBack, "<<" });
  ImGui::SameLine();
  button({ Action::StepBack, "<" });
  ImGui::SameLine();
  button({ Action::StepForward, ">" });
  ImGui::SameLine();
  button({ Action::SkipForward, ">>" });
  ImGui::EndChild();

  ImGui::SameLine();  // Payback speed is on the same line.

  // Playback speed UI.
  float playback_speed = 1.0f;
  if (data_thread)
  {
    playback_speed = data_thread->playbackSpeed();
  }

  if (_pending_speed.has_value())
  {
    playback_speed = *_pending_speed;
  }

  ImGui::BeginChild("Playback speed", ImVec2{ 0, button_row_size });
  if (ImGui::InputFloat("Speed", &playback_speed, 0.1f, 1.0f, "%.2f"))
  {
    _pending_speed = std::max(0.01f, std::min(playback_speed, 20.0f));
  }
  const bool edit_active = ImGui::IsItemActive();
  ImGui::EndChild();

  // Commit pending frame when neither input control is active.
  if (_pending_speed.has_value() && !edit_active)
  {
    auto set_speed_command = _set_speed_command.lock();
    if (set_speed_command)
    {
      if (set_speed_command->admissible(_viewer))
      {
        set_speed_command->invoke(_viewer, command::Args(*_pending_speed));
      }
    }
    _pending_speed.reset();
  }
}


void Playback::drawFrameSlider(DataThread *data_thread)
{
  int total_frames = 0;
  int current_frame = 0;

  if (data_thread)
  {
    current_frame = int_cast<int>(data_thread->currentFrame());
    total_frames = int_cast<int>(data_thread->totalFrames());
  }

  // Pending frame number takes precedence over the actual current frame number.
  if (_pending_frame.has_value())
  {
    current_frame = *_pending_frame;
  }

  const auto frames_str = std::to_string(total_frames);
  auto set_frame_command = _set_frame_command.lock();
  const bool writable = set_frame_command && set_frame_command->admissible(_viewer);

  int flags = 0;

  flags = (!writable) ? ImGuiSliderFlags_NoInput : 0;
  ImGui::BeginChild("Frame slider", ImVec2(uiViewportSize().x() * 0.75f, 0));
  if (ImGui::SliderInt(frames_str.c_str(), &current_frame, 0, total_frames, "%d", flags))
  {
    _pending_frame = current_frame;
  }
  const bool slider_active = ImGui::IsItemActive();
  ImGui::EndChild();
  ImGui::SameLine();
  flags = (!writable) ? ImGuiInputTextFlags_ReadOnly : 0;
  ImGui::BeginChild("Frame edit");
  if (ImGui::InputInt(frames_str.c_str(), &current_frame))
  {
    _pending_frame = std::max(-1, std::min(current_frame, total_frames));
  }
  const bool edit_active = ImGui::IsItemActive();
  ImGui::EndChild();

  // Commit pending frame when neither input control is active.
  if (_pending_frame.has_value() && !slider_active && !edit_active)
  {
    if (set_frame_command)
    {
      if (set_frame_command->admissible(_viewer))
      {
        // Wrap/clamp the pending frame number.
        auto target_frame = *_pending_frame;
        if (target_frame < 0)
        {
          target_frame += total_frames;
        }
        if (target_frame < total_frames)
        {
          set_frame_command->invoke(_viewer, command::Args(int_cast<unsigned>(target_frame)));
        }
      }
    }
    _pending_frame.reset();
  }
}


Playback::ButtonResult Playback::button(const PlaybackButtonParams &params, bool allow_inactive)
{
  const auto action_idx = static_cast<unsigned>(params.action);
  const auto icon_idx =
    (params.icon_alias != Action::Count) ? static_cast<unsigned>(params.icon_alias) : action_idx;

  ButtonParams super_params;
  super_params.icon = &_action_icons[icon_idx];
  super_params.label = params.label;
  super_params.command = command(params.action).get();
  super_params.size = { kButtonSize, kButtonSize };
  return Panel::button(super_params, allow_inactive);
}


Playback::ButtonResult Playback::button(std::initializer_list<PlaybackButtonParams> candidates)
{
  Action first_action = Action::Count;
  const PlaybackButtonParams *first_params = nullptr;

  for (const auto &params : candidates)
  {
    if (!first_params)
    {
      first_params = &params;
    }
    // Try draw the button, but don't allow inactive.
    const auto result = button(params, false);
    if (result != ButtonResult::Inactive)
    {
      return result;
    }
  }

  // Nothing admissible. Draw the first item inactive.
  if (first_params)
  {
    return button(*first_params, true);
  }

  return ButtonResult::Inactive;
}

void Playback::initialiseIcons()
{
  Corrade::PluginManager::Manager<Magnum::Trade::AbstractImporter> manager;
  Corrade::Containers::Pointer<Magnum::Trade::AbstractImporter> importer =
    manager.loadAndInstantiate("PngImporter");
  if (!importer)
  {
    log::error("Unable to resolve PngImporter plugin. Icons will be absent.");
    return;
  }

  Corrade::Utility::Resource rs("resources");

  const auto &icon_names = actionIconNames();
  for (size_t i = 0; i < _action_icons.size(); ++i)
  {
    const auto &icon_name = icon_names[i];
    if (!importer->openData(rs.getRaw(icon_name)))
    {
      log::error("Unable to resolve icon ", icon_name);
      continue;
    }

    auto image_data = importer->image2D(0);
    _action_icons[i]
      .setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge)
      .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
      .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
      .setStorage(1, Magnum::GL::textureFormat(image_data->format()), image_data->size())
      .setSubImage(0, {}, *image_data);
  }
}


const Playback::ActionIconNames &Playback::actionIconNames()
{
  static Playback::ActionIconNames names = { "Record.png",      "Stop.png",       "Play.png",
                                             "Pause.png",       "SkipBack.png",   "StepBack.png",
                                             "StepForward.png", "SkipForward.png" };
  return names;
}
}  // namespace tes::view::ui
