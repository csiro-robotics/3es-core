//
// Author: Kazys Stepanas
//
#include "Playback.h"

#include <3escore/CoreUtil.h>
#include <3escore/Log.h>

#include <3esview/command/Set.h>
#include <3esview/data/StreamThread.h>
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
  using namespace Magnum::ImGuiIntegration;

  // ImGui::SetNextWindowPos();
  // ImGui::SetNextWindowSize();
  setNextWindowPos({ 0, -kPanelSize }, Anchor::BottomLeft);
  setNextWindowSize({ 0, kPanelSize }, Stretch::Horizontal);
  ImGui::Begin("Playback", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);


  drawButtons();
  drawFrameSlider();

  auto pos = ImGui::GetWindowPos();

  ImGui::End();
}


void Playback::drawButtons()
{
  // Layout playback buttons horizontally grouped.
  const auto button_row_size = kButtonSize + 8;
  ImGui::BeginChild("Playback buttons", ImVec2(uiViewportSize().x() * 0.75f, button_row_size));
  button({ { Action::Stop, "S" }, { Action::Record, "R" } });
  ImGui::SameLine();
  button({ { Action::Play, "P" }, { Action::Pause, "||" } });
  ImGui::SameLine();
  button(Action::SkipBack, "<<");
  ImGui::SameLine();
  button(Action::StepBack, "<");
  ImGui::SameLine();
  button(Action::StepForward, ">");
  ImGui::SameLine();
  button(Action::SkipForward, ">>");
  ImGui::EndChild();

  ImGui::SameLine();  // Payback speed is on the same line.

  // Playback speed UI.
  float playback_speed = 1.0f;
  auto stream_thread = std::dynamic_pointer_cast<tes::view::StreamThread>(_viewer.dataThread());
  if (stream_thread)
  {
    playback_speed = stream_thread->playbackSpeed();
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


void Playback::drawFrameSlider()
{
  int total_frames = 0;
  int current_frame = 0;

  auto data_thread = _viewer.dataThread();
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
    _pending_frame = std::max(-1, std::max(current_frame, total_frames));
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


Playback::ButtonResult Playback::button(Action action, const char *label, bool allow_inactive)
{
  const auto action_idx = static_cast<unsigned>(action);
  if (command(action) && command(action)->admissible(_viewer))
  {
    // Try for an image first.
    bool pressed = false;
    if (_action_icons[action_idx].id())
    {
      pressed =
        ImGui::ImageButton(label, &_action_icons[action_idx], ImVec2{ kButtonSize, kButtonSize });
    }
    else
    {
      pressed = ImGui::Button(label, ImVec2{ kButtonSize, kButtonSize });
    }

    if (pressed)
    {
      command(action)->invoke(_viewer);
      return ButtonResult::Pressed;
    }
    return ButtonResult::Ok;
  }

  // Draw inactive.
  if (allow_inactive)
  {
    if (_action_icons[action_idx].id())
    {
      // Padding to make sure the images rendering the same size as the ImageButton equivalents.
      // Determined empirically.
      constexpr static int kDisableButtonPaddingX = 6;
      constexpr static int kDisableButtonPaddingY = 4;
      ImGui::Image(&_action_icons[action_idx], ImVec2{ kButtonSize + kDisableButtonPaddingX,
                                                       kButtonSize + kDisableButtonPaddingY });
    }
    else
    {
      ImGui::Text(label);
    }
  }
  return ButtonResult::Inactive;
}


Playback::ButtonResult Playback::button(
  std::initializer_list<std::pair<Action, const char *>> candidates)
{
  Action first_action = Action::Count;
  const char *first_label = nullptr;

  for (const auto &[action, label] : candidates)
  {
    if (!first_label)
    {
      first_action = action;
      first_label = label;
    }
    // Try draw the button, but don't allow inactive.
    const auto result = button(action, label, false);
    if (result != ButtonResult::Inactive)
    {
      return result;
    }
  }

  // Nothing admissible. Draw the first item inactive.
  if (first_label)
  {
    return button(first_action, first_label);
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
