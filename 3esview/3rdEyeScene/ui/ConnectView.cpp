//
// Author: Kazys Stepanas
//
#include "ConnectView.h"

#include <3escore/CoreUtil.h>
#include <3escore/Log.h>

#include <Magnum/ImGuiIntegration/Context.hpp>

namespace tes::view::ui
{
ConnectView::ConnectView(Viewer &viewer)
  : Panel(viewer)
{}


void ConnectView::draw(Magnum::ImGuiIntegration::Context &ui)
{
  TES_UNUSED(ui);
}
}  // namespace tes::view::ui
