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


void IconBar::closeActiveView()
{
  _active_view = View::Invalid;
}


void IconBar::setActive(View view)
{
  if (_active_view != view)
  {
    closeActiveView();
    _active_view = view;
  }
}


IconBar::View IconBar::activeView() const
{
  return _active_view;
}


bool IconBar::isActive(View view) const
{
  return _active_view == view;
}


void IconBar::registerCommand(View view, std::shared_ptr<tes::view::command::Command> command)
{
  if (view != View::Invalid)
  {
    _commands[static_cast<unsigned>(view)] = command;
  }
}


void IconBar::registerView(View view, std::shared_ptr<Panel> panel)
{
  if (view != View::Invalid)
  {
    const auto view_idx = static_cast<unsigned>(view);
    _panels[view_idx] = panel;
  }
}


void IconBar::draw(Magnum::ImGuiIntegration::Context &ui)
{
  TES_UNUSED(ui);

  setNextWindowPos({ 0, 0 }, Anchor::TopLeft);
  setNextWindowSize({ kPanelSize, -Playback::kPanelSize }, Stretch::Vertical);

  ImGui::Begin("Icon Bar", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
  const ImVec2 icon_size = { kButtonSize, kButtonSize };
  button({ &_icons[static_cast<unsigned>(View::Settings)], "Settings",
           _commands[static_cast<unsigned>(View::Settings)].get(), icon_size });
  button({ &_icons[static_cast<unsigned>(View::Connect)], "Connect",
           _commands[static_cast<unsigned>(View::Connect)].get(), icon_size });
  button({ &_icons[static_cast<unsigned>(View::Categories)], "Categories",
           _commands[static_cast<unsigned>(View::Categories)].get(), icon_size });
  button({ &_icons[static_cast<unsigned>(View::Log)], "Log",
           _commands[static_cast<unsigned>(View::Log)].get(), icon_size });
  ImGui::End();

  if (_active_view != View::Invalid)
  {
    const auto view_idx = static_cast<unsigned>(_active_view);
    if (_panels[view_idx])
    {
      _panels[view_idx]->draw(ui);
    }
  }
}


void IconBar::initialiseIcons()
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

  const auto &icon_names = viewIconNames();
  for (size_t i = 0; i < _icons.size(); ++i)
  {
    const auto &icon_name = icon_names[i];
    if (!importer->openData(rs.getRaw(icon_name)))
    {
      log::error("Unable to resolve icon ", icon_name);
      continue;
    }

    auto image_data = importer->image2D(0);
    _icons[i]
      .setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge)
      .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
      .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
      .setStorage(1, Magnum::GL::textureFormat(image_data->format()), image_data->size())
      .setSubImage(0, {}, *image_data);
  }
}


const IconBar::ViewIconNames &IconBar::viewIconNames()
{
  static IconBar::ViewIconNames names = {
    "Settings.png",
    "Connect.png",
    "Categories.png",
    "Log.png",
  };
  return names;
}
}  // namespace tes::view::ui
