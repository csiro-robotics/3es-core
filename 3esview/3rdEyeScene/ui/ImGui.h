//
// Author: Kazys Stepanas
//
#ifndef TRD_EYE_SCENE_UI_IMGUI_H
#define TRD_EYE_SCENE_UI_IMGUI_H

// Fix including of ImGui headers with VCPKG integration.
// TODO(KS): This will need adjustment if using system built headers.
#define MAGNUM_IMGUIINTEGRATION_BUILD_STATIC

#include <Magnum/ImGuiIntegration/Context.hpp>

#endif  // TRD_EYE_SCENE_UI_IMGUI_H
