//
// Author: Kazys Stepanas
//
#include "IconBar.h"

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

IconBar::IconBar(Viewer &viewer)
  : Panel(viewer)
{
  initialiseIcons();
}


void IconBar::draw(Magnum::ImGuiIntegration::Context &ui)
{
  TES_UNUSED(ui);

  setNextWindowPos({ 0, 0 }, Anchor::TopLeft);
  setNextWindowSize({ kPanelSize, -Playback::kPanelSize }, Stretch::Vertical);

  button({ &_icons[static_cast<unsigned>(Action::Settings)], "Settings" });
  button({ &_icons[static_cast<unsigned>(Action::Connect)], "Connect" });
  button({ &_icons[static_cast<unsigned>(Action::Categories)], "Categories" });
  button({ &_icons[static_cast<unsigned>(Action::Log)], "Log" });
}


void IconBar::initialiseIcons()
{}
}  // namespace tes::view::ui
