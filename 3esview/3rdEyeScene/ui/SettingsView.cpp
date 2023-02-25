//
// Author: Kazys Stepanas
//
#include "SettingsView.h"

#include <3escore/CoreUtil.h>
#include <3escore/Log.h>

#include <Magnum/ImGuiIntegration/Context.hpp>

namespace tes::view::ui
{
SettingsView::SettingsView(Viewer &viewer)
  : Panel(viewer)
{}


void SettingsView::draw(Magnum::ImGuiIntegration::Context &ui)
{
  TES_UNUSED(ui);
}
}  // namespace tes::view::ui
