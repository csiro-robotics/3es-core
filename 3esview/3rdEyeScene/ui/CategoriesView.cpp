//
// Author: Kazys Stepanas
//
#include "CategoriesView.h"

#include <3escore/CoreUtil.h>
#include <3escore/Log.h>

#include <Magnum/ImGuiIntegration/Context.hpp>

namespace tes::view::ui
{
CategoriesView::CategoriesView(Viewer &viewer)
  : Panel(viewer)
{}


void CategoriesView::draw(Magnum::ImGuiIntegration::Context &ui)
{
  TES_UNUSED(ui);
}
}  // namespace tes::view::ui
