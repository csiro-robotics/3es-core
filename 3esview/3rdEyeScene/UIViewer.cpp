//
// Author: Kazys Stepanas
//
#include "UIViewer.h"

#include "ui/Playback.h"

#include <3esview/command/Set.h>

namespace tes::view
{
class ToggleUI : public command::Command
{
public:
  ToggleUI()
    : command::Command("toggleUI", command::Args())
  {}

protected:
  bool checkAdmissible(Viewer &viewer) const
  {
    auto *ui_viewer = dynamic_cast<UIViewer *>(&viewer);
    return ui_viewer != nullptr;
  }

  command::CommandResult invoke(Viewer &viewer, const ExecInfo &info,
                                const command::Args &args) override
  {
    TES_UNUSED(info);
    TES_UNUSED(args);
    auto *ui_viewer = dynamic_cast<UIViewer *>(&viewer);
    if (!ui_viewer)
    {
      return { command::CommandResult::Code::Invalid };
    }

    ui_viewer->setUiEnabled(!ui_viewer->uiEnabled());

    return { command::CommandResult::Code::Ok };
  }
};


UIViewer::GuiContext::GuiContext(ImGuiContext *context)
  : _current(context)
{
  _restore = ImGui::GetCurrentContext();
  ImGui::SetCurrentContext(context);
}


UIViewer::GuiContext::~GuiContext()
{
  ImGui::SetCurrentContext(_restore);
}


UIViewer::UIViewer(const Arguments &arguments)
  : Viewer(arguments)
{
  _imgui = Magnum::ImGuiIntegration::Context(Magnum::Vector2{ windowSize() } / dpiScaling(),
                                             windowSize(), framebufferSize());
  initialiseUi();
  commands()->registerCommand(std::make_shared<ToggleUI>(), command::Shortcut("F2"));
}


UIViewer::DrawMode UIViewer::onDrawStart(float dt)
{
  TES_UNUSED(dt);
  const GuiContext gui_context(_imgui.context());

  if (ImGui::GetIO().WantTextInput && !isTextInputActive())
  {
    startTextInput();
  }
  else if (!ImGui::GetIO().WantTextInput && isTextInputActive())
  {
    stopTextInput();
  }

  return isTextInputActive() ? DrawMode::Modal : DrawMode::Normal;
}


void UIViewer::onDrawComplete(float dt)
{
  TES_UNUSED(dt);

  const GuiContext gui_context(_imgui.context());

  _imgui.newFrame();

  if (_ui_enabled)
  {
    for (const auto &panel : _panels)
    {
      panel->draw(_imgui);
    }

    _imgui.updateApplicationCursor(*this);
  }

  // Set render state for the UI
  Magnum::GL::Renderer::setBlendEquation(Magnum::GL::Renderer::BlendEquation::Add,
                                         Magnum::GL::Renderer::BlendEquation::Add);
  Magnum::GL::Renderer::setBlendFunction(Magnum::GL::Renderer::BlendFunction::SourceAlpha,
                                         Magnum::GL::Renderer::BlendFunction::OneMinusSourceAlpha);

  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::Blending);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::ScissorTest);
  Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::FaceCulling);
  Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::DepthTest);

  _imgui.drawFrame();

  // Clear UI render states
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::DepthTest);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::FaceCulling);
  Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::ScissorTest);
  Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::Blending);

  Magnum::GL::Renderer::setBlendEquation(Magnum::GL::Renderer::BlendEquation::Add,
                                         Magnum::GL::Renderer::BlendEquation::Add);
  Magnum::GL::Renderer::setBlendFunction(Magnum::GL::Renderer::BlendFunction::One,
                                         Magnum::GL::Renderer::BlendFunction::Zero);
}


void UIViewer::viewportEvent(ViewportEvent &event)
{
  Viewer::viewportEvent(event);
  _imgui.relayout(Magnum::Vector2{ event.windowSize() } / event.dpiScaling(), event.windowSize(),
                  event.framebufferSize());
}


void UIViewer::initialiseUi()
{
  initialiseImGui();
  initialisePlaybackUi();
}


void UIViewer::initialiseImGui()
{
  // The ImGuiIntegration::Context initialises the KeyMap incorrectly. It is meant to index items
  // in KeysDown, and must be in range [-1, 511] with -1 used for unmapped keys.
  // TODO(KS): this may break some UI functionality, so I'll need to work out what the mappings
  // should be. The Magnum code actually flags the mappings as suspect.
  const GuiContext gui_context(_imgui.context());
  ImGuiIO &io = ImGui::GetIO();
  for (auto &key : io.KeyMap)
  {
    if (key != -1)
    {
      key = -1;
    }
  }
}


void UIViewer::initialisePlaybackUi()
{
  auto playback = std::make_shared<ui::Playback>(*this);
  playback->registerAction(ui::Playback::Stop, commands()->lookupName("stop").command);
  playback->registerAction(ui::Playback::Record, commands()->lookupName("record").command);
  playback->registerAction(ui::Playback::Play, commands()->lookupName("openFile").command);
  playback->registerAction(ui::Playback::PauseResume, commands()->lookupName("openFile").command);
  playback->registerAction(ui::Playback::SkipBack, commands()->lookupName("skipBackward").command);
  playback->registerAction(ui::Playback::StepBack, commands()->lookupName("stepBackward").command);
  playback->registerAction(ui::Playback::StepForward,
                           commands()->lookupName("stepForward").command);
  playback->registerAction(ui::Playback::SkipForward,
                           commands()->lookupName("skipForward").command);

  _panels.emplace_back(playback);
}


void UIViewer::mousePressEvent(MouseEvent &event)
{
  if (_imgui.handleMousePressEvent(event))
  {
    return;
  }
  Viewer::mousePressEvent(event);
}


void UIViewer::mouseReleaseEvent(MouseEvent &event)
{
  if (_imgui.handleMouseReleaseEvent(event))
  {
    return;
  }
  Viewer::mouseReleaseEvent(event);
}


void UIViewer::mouseMoveEvent(MouseMoveEvent &event)
{
  if (_imgui.handleMouseMoveEvent(event))
  {
    return;
  }
  Viewer::mouseMoveEvent(event);
}


void UIViewer::mouseScrollEvent(MouseScrollEvent &event)
{
  if (_imgui.handleMouseScrollEvent(event))
  {
    // Prevent scrolling the page
    event.setAccepted();
    return;
  }
  // Viewer::mouseScrollEvent(event);
}


void UIViewer::keyPressEvent(KeyEvent &event)
{
  if (_imgui.handleKeyPressEvent(event))
  {
    return;
  }
  Viewer::keyPressEvent(event);
}


void UIViewer::keyReleaseEvent(KeyEvent &event)
{
  if (_imgui.handleKeyReleaseEvent(event))
  {
    return;
  }
  Viewer::keyReleaseEvent(event);
}


void UIViewer::textInputEvent(TextInputEvent &event)
{
  if (_imgui.handleTextInputEvent(event))
  {
    return;
  }
  // Viewer::textInputEvent(event);
}
}  // namespace tes::view
