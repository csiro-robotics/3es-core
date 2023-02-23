//
// Author: Kazys Stepanas
//
#include "Playback.h"

#include <3esview/command/Command.h>

#include <Magnum/ImGuiIntegration/Context.hpp>

namespace tes::view::ui
{
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

  ImGui::Begin("Playback");

  // Layout playback buttons horizontally.
  // TODO(KS): use ImuGui::ImageButton()
  // Start with the record/stop button
  button({ { Action::Stop, "S" }, { Action::Record, "R" } });
  ImGui::SameLine();
  button(Action::PauseResume, "P");
  ImGui::SameLine();
  button(Action::SkipBack, "<<");
  ImGui::SameLine();
  button(Action::StepBack, "<");
  ImGui::SameLine();
  button(Action::StepForward, ">");
  ImGui::SameLine();
  button(Action::SkipForward, ">>");

  ImGui::End();
}


bool Playback::button(Action action, const char *label)
{
  if (command(action) && command(action)->admissible(_viewer))
  {
    if (ImGui::Button(label))
    {
      command(action)->invoke(_viewer);
      return true;
    }
  }
  else
  {
    // Draw inactive.
    ImGui::Text(label);
  }
  return false;
}


bool Playback::button(std::initializer_list<std::pair<Action, const char *>> candidates)
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
    if (command(action) && command(action)->admissible(_viewer))
    {
      if (ImGui::Button(label))
      {
        command(action)->invoke(_viewer);
        return true;
      }
      return false;
    }
  }

  // Nothing admissible. Draw the first item inactive.
  if (first_label)
  {
    return button(first_action, first_label);
  }

  return false;
}

}  // namespace tes::view::ui
