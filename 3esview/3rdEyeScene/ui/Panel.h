//
// Author: Kazys Stepanas
//
#ifndef TRD_EYE_SCENE_UI_PANEL_H
#define TRD_EYE_SCENE_UI_PANEL_H

#include <3rdEyeScene/ClientConfig.h>

#include "ImGui.h"

#include <memory>

namespace Magnum::ImGuiIntegration
{
class Context;
}  // namespace Magnum::ImGuiIntegration

namespace tes::view
{
class Viewer;
}  // namespace tes::view

namespace tes::view::ui
{
/// Base class for a UI panel - anything which draws using the immediate mode UI.
class Panel
{
public:
  Panel(Viewer &viewer)
    : _viewer(viewer)
  {}

  virtual void draw(Magnum::ImGuiIntegration::Context &ui) = 0;

  Viewer &viewer() { return _viewer; }
  const Viewer &viewer() const { return _viewer; }

protected:
  Viewer &_viewer;
};
}  // namespace tes::view::ui

#endif  // TRD_EYE_SCENE_UI_PANEL_H
