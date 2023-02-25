//
// Author: Kazys Stepanas
//
#ifndef TRD_EYE_SCENE_UI_PANEL_H
#define TRD_EYE_SCENE_UI_PANEL_H

#include <3rdEyeScene/ClientConfig.h>

#include "ImGui.h"

#include <Magnum/GL/Texture.h>

#include <memory>

namespace tes::view::command
{
class Command;
}

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
  enum class Anchor : unsigned
  {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Centre,
    TopCentre,
    BottomCentre,
    CentreLeft,
    CentreRight,
  };

  enum class Stretch : unsigned
  {
    None,
    Horizontal,
    Vertical
  };

  Panel(Viewer &viewer)
    : _viewer(viewer)
  {}

  virtual void draw(Magnum::ImGuiIntegration::Context &ui) = 0;

  void setNextWindowPos(Magnum::Vector2i pos, Anchor anchor = Anchor::TopLeft) const;
  void setNextWindowSize(Magnum::Vector2i size, Stretch stretch = Stretch::None) const;

  /// Get the size of the viewport used to draw the UI. This may differ from the window size.
  /// @return The UI viewport size.
  Magnum::Vector2i uiViewportSize() const;

  Viewer &viewer() { return _viewer; }
  const Viewer &viewer() const { return _viewer; }

protected:
  /// Result from @c button() function.
  enum ButtonResult
  {
    /// Button is inactive. Rendered if @c allow_inactive was passed true.
    ///
    /// A button can only be inactive when @c ButtonParam::command is not null and is inadmissible.
    Inactive,
    /// Button was drawn, but not pressed.
    Ok,
    /// Button was pressed. The @c ButtonParams::command will have been invoked if not null.
    Pressed,
  };

  /// Button parameterisation.
  struct ButtonParams
  {
    /// Button icon (if any).
    Magnum::GL::Texture2D *icon = nullptr;
    /// Button label (required).
    std::string label;
    /// Command to execute when pressed (if any).
    tes::view::command::Command *command = nullptr;
    /// Explicit drawing size
    ImVec2 size = { 0, 0 };

    ButtonParams() = default;
    ButtonParams(Magnum::GL::Texture2D *icon, std::string label,
                 tes::view::command::Command *command = nullptr)
      : icon(std::move(icon))
      , label(std::move(label))
      , command(command)
    {}
    ButtonParams(Magnum::GL::Texture2D *icon, std::string label,
                 tes::view::command::Command *command, const ImVec2 &size)
      : icon(std::move(icon))
      , label(std::move(label))
      , command(command)
      , size(size)
    {}
    ButtonParams(const ButtonParams &other) = default;
    ButtonParams(ButtonParams &&other) = default;

    ButtonParams &operator=(const ButtonParams &other) = default;
    ButtonParams &operator=(ButtonParams &&other) = default;
  };

  /// Draw a button associated with the given action.
  /// @param params Details of the button.
  /// @param allow_inactive When true, draws the action icon as inactive, otherwise draws nothing.
  ButtonResult button(const ButtonParams &params, bool allow_inactive = true);

  Viewer &_viewer;
};
}  // namespace tes::view::ui

#endif  // TRD_EYE_SCENE_UI_PANEL_H
