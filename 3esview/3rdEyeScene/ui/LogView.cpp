//
// Author: Kazys Stepanas
//
#include "LogView.h"

#include <3escore/CoreUtil.h>
#include <3escore/Log.h>

#include <Magnum/ImGuiIntegration/Context.hpp>

namespace tes::view::ui
{
LogView::LogView(Viewer &viewer)
  : Panel(viewer)
{}


void LogView::draw(Magnum::ImGuiIntegration::Context &ui)
{
  TES_UNUSED(ui);
}
}  // namespace tes::view::ui
