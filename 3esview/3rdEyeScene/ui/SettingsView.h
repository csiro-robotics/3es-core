//
// Author: Kazys Stepanas
//
#ifndef TRD_EYE_SCENE_UI_SETTINGS_VIEW_H
#define TRD_EYE_SCENE_UI_SETTINGS_VIEW_H

#include <3rdEyeScene/ClientConfig.h>

#include "Panel.h"

#include <3esview/settings/Settings.h>

#include <Magnum/GL/Texture.h>

#include <array>
#include <memory>

namespace tes::view
{
class DataThread;
}

namespace tes::view::command
{
class Command;
}

namespace tes::view::ui
{
class SettingsView : public Panel
{
public:
  SettingsView(Viewer &viewer);

  void draw(Magnum::ImGuiIntegration::Context &ui) override;

private:
  bool show(unsigned idx, settings::Camera &config);
  bool show(unsigned idx, settings::Log &config);
  bool show(unsigned idx, settings::Playback &config);
  bool show(unsigned idx, settings::Render &config);

  bool showProperty(unsigned idx, settings::Bool &prop);
  bool showProperty(unsigned idx, settings::Int &prop);
  bool showProperty(unsigned idx, settings::UInt &prop);
  bool showProperty(unsigned idx, settings::Float &prop);
  bool showProperty(unsigned idx, settings::Double &prop);
  bool showProperty(unsigned idx, settings::Colour &prop);
  template <typename E>
  bool showProperty(unsigned idx, settings::Enum<E> &prop);

  bool beginSection(unsigned idx, const std::string &label);
  void endSection(bool open);
  void beginProperty(unsigned idx, const std::string &label, const std::string &info);
  void endProperty();
};
}  // namespace tes::view::ui

#endif  // TRD_EYE_SCENE_UI_SETTINGS_VIEW_H
