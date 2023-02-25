//
// Author: Kazys Stepanas
//
#ifndef TRD_EYE_SCENE_UI_CATEGORIES_VIEW_H
#define TRD_EYE_SCENE_UI_CATEGORIES_VIEW_H

#include <3rdEyeScene/ClientConfig.h>

#include "Panel.h"

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
class CategoriesView : public Panel
{
public:
  CategoriesView(Viewer &viewer);

  void draw(Magnum::ImGuiIntegration::Context &ui) override;

private:
};
}  // namespace tes::view::ui

#endif  // TRD_EYE_SCENE_UI_CATEGORIES_VIEW_H
