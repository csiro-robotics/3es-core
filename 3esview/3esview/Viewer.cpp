#include "Viewer.h"

#include "EdlEffect.h"

#include "command/DefaultCommands.h"
#include "command/Set.h"

#include "data/NetworkThread.h"
#include "data/StreamThread.h"

#include "painter/Arrow.h"
#include "painter/Box.h"
#include "painter/Capsule.h"
#include "painter/Cylinder.h"
#include "painter/Plane.h"
#include "painter/Pose.h"
#include "painter/Sphere.h"
#include "painter/Star.h"

#include <3escore/Log.h>
#include <3escore/Server.h>

#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Version.h>

#include <fstream>
#include <iostream>

#include <cxxopts.hpp>

// Things to learn about:
// - UI

// Things to implement:
// - mesh renderer

namespace tes::view
{
namespace
{
void focusCallback(GLFWwindow *window, int focused)
{
  auto *app = static_cast<Magnum::Platform::GlfwApplication *>(glfwGetWindowUserPointer(window));
  auto *viewer = dynamic_cast<Viewer *>(app);
  viewer = viewer;
  if (viewer)
  {
    viewer->setContinuousSim(focused != 0);
  }
}
}  // namespace

uint16_t Viewer::defaultPort()
{
  return ServerSettings().listen_port;
}

Viewer::Viewer(const Arguments &arguments)
  : Magnum::Platform::Application{ arguments,
                                   Configuration{}
                                     .setTitle("3rd Eye Scene Viewer")
                                     .setWindowFlags(Configuration::WindowFlag::Resizable) }
  , _tes(std::make_shared<ThirdEyeScene>())
  , _commands(std::make_shared<command::Set>())
  , _move_keys({
      { KeyEvent::Key::A, 0, true },         //
      { KeyEvent::Key::Left, 0, true },      //
      { KeyEvent::Key::D, 0, false },        //
      { KeyEvent::Key::Right, 0, false },    //
      { KeyEvent::Key::W, 1, false },        //
      { KeyEvent::Key::Up, 1, false },       //
      { KeyEvent::Key::S, 1, true },         //
      { KeyEvent::Key::Down, 1, true },      //
      { KeyEvent::Key::R, 2, false },        //
      { KeyEvent::Key::PageUp, 2, false },   //
      { KeyEvent::Key::F, 2, true },         //
      { KeyEvent::Key::PageDown, 2, true },  //
    })
  , _rotate_keys({
      { KeyEvent::Key::T, 0, false },  //
      { KeyEvent::Key::G, 0, true },   //
      { KeyEvent::Key::Q, 1, false },  //
      { KeyEvent::Key::E, 1, true },   //
    })
{
  _edl_effect = std::make_shared<EdlEffect>(Magnum::GL::defaultFramebuffer.viewport());
  command::registerDefaultCommands(*_commands);

  if (!handleStartupArgs(arguments))
  {
    exit();
  }

  // GLFW specific.
  // Bind a callback to force continuous sim while focused.
  // glfwSetWindowUserPointer(window(), this);
  glfwSetWindowFocusCallback(window(), focusCallback);

  const auto config = _tes->settings().config();
  _camera.position = { 0, -5, 0 };
  onCameraSettingsChange(config);
  onRenderSettingsChange(config);
  onPlaybackSettingsChange(config);

  _tes->settings().addObserver(
    settings::Settings::Category::Camera,
    [this](const settings::Settings::Config &config) { onCameraSettingsChange(config); });
  _tes->settings().addObserver(
    settings::Settings::Category::Render,
    [this](const settings::Settings::Config &config) { onRenderSettingsChange(config); });
  _tes->settings().addObserver(
    settings::Settings::Category::Playback,
    [this](const settings::Settings::Config &config) { onPlaybackSettingsChange(config); });
}


Viewer::~Viewer()
{
  closeOrDisconnect();
}


bool Viewer::open(const std::filesystem::path &path)
{
  closeOrDisconnect();
  _tes->reset();
  std::ifstream file(path.string(), std::ios::binary);
  if (!file.is_open())
  {
    return false;
  }

  const auto config = _tes->settings().config();
  _data_thread =
    std::make_shared<StreamThread>(_tes, std::make_shared<std::ifstream>(std::move(file)));
  _data_thread->setLooping(config.playback.looping.value());
  return true;
}


bool Viewer::connect(const std::string &host, uint16_t port, bool allow_reconnect)
{
  closeOrDisconnect();
  _tes->reset();
  auto net_thread = std::make_shared<NetworkThread>(_tes, host, port, allow_reconnect);
  _data_thread = net_thread;
  if (!allow_reconnect)
  {
    // Reconnection not allowed. Wait until the network thread has tried to connect...
    const auto start_time = std::chrono::steady_clock::now();
    // ...but don't wait forever.
    const auto timeout = std::chrono::seconds(5);
    while (!net_thread->connectionAttempted() &&
           (std::chrono::steady_clock::now() - start_time) < timeout)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return net_thread->connected();
  }
  return true;
}


bool Viewer::closeOrDisconnect()
{
  if (_data_thread)
  {
    _data_thread->stop();
    _data_thread->join();
    _data_thread = nullptr;
    return true;
  }
  else
  {
    // Reset existing data on second close/reset request.
    _tes->reset();
  }
  return false;
}


void Viewer::setContinuousSim(bool continuous)
{
  if (_continuous_sim != continuous)
  {
    _continuous_sim = continuous;
    if (continuous)
    {
      _last_sim_time = Clock::now();
    }
  }
}

bool Viewer::continuousSim()
{
  // Check forcing continuous mode.
  if (_continuous_sim || _mouse_rotation_active || _data_thread)
  {
    return true;
  }

  // Check keys.
  bool continuous_sim = false;
  for (const auto &key : _move_keys)
  {
    continuous_sim = key.active || continuous_sim;
  }
  for (const auto &key : _rotate_keys)
  {
    continuous_sim = key.active || continuous_sim;
  }
  return continuous_sim;
}


void Viewer::drawEvent()
{
  using namespace Magnum::Math::Literals;

  const auto now = Clock::now();
  const auto delta_time = now - _last_sim_time;
  _last_sim_time = now;
  const float dt = std::chrono::duration_cast<std::chrono::duration<float>>(delta_time).count();

  const auto draw_mode = onDrawStart(dt);

  updateCamera(dt, draw_mode == DrawMode::Normal);

  _tes->render(dt, windowSize());

  onDrawComplete(dt);

  swapBuffers();
  if (continuousSim() || isTextInputActive())
  {
    redraw();
  }
}


void Viewer::viewportEvent(ViewportEvent &event)
{
  Magnum::GL::defaultFramebuffer.setViewport({ {}, event.framebufferSize() });
  _edl_effect->viewportChange(Magnum::GL::defaultFramebuffer.viewport());
}


void Viewer::keyPressEvent(KeyEvent &event)
{
  // Start with the shortcuts.
  checkShortcuts(event);
  if (event.isAccepted())
  {
    return;
  }

  bool dirty = false;
  for (auto &key : _move_keys)
  {
    if (event.key() == key.key)
    {
      key.active = true;
      event.setAccepted();
      dirty = true;
    }
  }

  for (auto &key : _rotate_keys)
  {
    if (event.key() == key.key)
    {
      key.active = true;
      event.setAccepted();
      dirty = true;
    }
  }

  if (event.key() == KeyEvent::Key::LeftShift)
  {
    const float fast_multiplier = 2.0f;
    _fly.setMoveMultiplier(fast_multiplier);
    // _fly.setRotationMultiplier(fast_multiplier);
    event.setAccepted();
  }

  if (event.key() == KeyEvent::Key::Space)
  {
    _tes->camera().position.y() -= 0.1f;
    dirty = true;
    event.setAccepted();
  }

  dirty = checkEdlKeys(event) || dirty;

  if (dirty)
  {
    redraw();
  }
}


void Viewer::keyReleaseEvent(KeyEvent &event)
{
  bool dirty = false;
  for (auto &key : _move_keys)
  {
    if (event.key() == key.key)
    {
      key.active = false;
      event.setAccepted();
      dirty = true;
    }
  }

  for (auto &key : _rotate_keys)
  {
    if (event.key() == key.key)
    {
      key.active = false;
      event.setAccepted();
      dirty = true;
    }
  }

  if (event.key() == KeyEvent::Key::LeftShift)
  {
    _fly.setMoveMultiplier(1.0f);
    _fly.setRotationMultiplier(1.0f);
    event.setAccepted();
  }

  if (dirty)
  {
    redraw();
  }
}


void Viewer::mousePressEvent(MouseEvent &event)
{
  if (event.button() != MouseEvent::Button::Left)
    return;

  _mouse_rotation_active = true;
  event.setAccepted();
}


void Viewer::mouseReleaseEvent(MouseEvent &event)
{
  _mouse_rotation_active = false;

  event.setAccepted();
  redraw();
}


void Viewer::mouseMoveEvent(MouseMoveEvent &event)
{
  using namespace Magnum::Math::Literals;
  if (!(event.buttons() & MouseMoveEvent::Button::Left))
  {
    return;
  }

  _fly.updateMouse(float(event.relativePosition().x()), float(event.relativePosition().y()),
                   _tes->camera());

  event.setAccepted();
  redraw();
}


void Viewer::onReset()
{
  _active_remote_camera = handler::Camera::kInvalidCameraId;
}


void Viewer::onCameraSettingsChange(const settings::Settings::Config &config)
{
  _camera.clip_far = config.camera.far_clip.value();
  _camera.clip_near = config.camera.near_clip.value();
  _camera.fov_horizontal_deg = config.camera.fov.value();
}


void Viewer::onRenderSettingsChange(const settings::Settings::Config &config)
{
  const auto &render = config.render;
  _edl_effect->setLinearScale(render.edl_linear_scale.value());
  _edl_effect->setExponentialScale(render.edl_exponential_scale.value());
  _edl_effect->setRadius(static_cast<float>(render.edl_radius.value()));
  if (render.use_edl_shader.value())
  {
    _tes->setActiveFboEffect(_edl_effect);
  }
  else
  {
    _tes->clearActiveFboEffect();
  }
}


void Viewer::onPlaybackSettingsChange(const settings::Settings::Config &config)
{
  if (_data_thread)
  {
    _data_thread->setLooping(config.playback.looping.value());
  }
}


bool Viewer::checkEdlKeys(KeyEvent &event)
{
  bool dirty = false;
  auto render_config = _tes->settings().config().render;
  if (event.key() == KeyEvent::Key::Tab)
  {
    render_config.use_edl_shader.setValue(!render_config.use_edl_shader.value());
    event.setAccepted(true);
    dirty = true;
    Magnum::Debug() << "EDL: " << (render_config.use_edl_shader.value() ? "on" : "off");
    _tes->settings().update(render_config);
  }
  else if (event.key() == KeyEvent::Key::Space)
  {
    _edl_tweak = EdlParam((int(_edl_tweak) + 1) % 3);
    switch (_edl_tweak)
    {
    case EdlParam::LinearScale:
      Magnum::Debug() << "EDL linear scale mode";
      break;
    case EdlParam::ExponentialScale:
      Magnum::Debug() << "EDL exponential scale mode";
      break;
    case EdlParam::Radius:
      Magnum::Debug() << "EDL radius scale mode";
      break;
    default:
      break;
    }
    event.setAccepted(true);
    dirty = true;
  }
  else if (event.key() == KeyEvent::Key::Equal || event.key() == KeyEvent::Key::Minus)
  {
    float delta = (event.key() == KeyEvent::Key::Equal) ? 0.5f : -0.5f;
    switch (_edl_tweak)
    {
    case EdlParam::LinearScale:
      render_config.edl_linear_scale.setValue(render_config.edl_linear_scale.value() + delta);
      Magnum::Debug() << "EDL linear scale: " << _edl_effect->linearScale();
      event.setAccepted(true);
      dirty = true;
      break;
    case EdlParam::ExponentialScale:
      render_config.edl_exponential_scale.setValue(render_config.edl_exponential_scale.value() +
                                                   delta);
      Magnum::Debug() << "EDL exponential scale: " << _edl_effect->exponentialScale();
      event.setAccepted(true);
      dirty = true;
      break;
    case EdlParam::Radius:
      render_config.edl_radius.setValue(render_config.edl_radius.value() +
                                        static_cast<unsigned>(2.0f * delta));
      Magnum::Debug() << "EDL radius scale: " << _edl_effect->radius();
      event.setAccepted(true);
      dirty = true;
      break;
    default:
      break;
    }

    if (dirty)
    {
      _tes->settings().update(render_config);
    }
  }

  return dirty;
}


void Viewer::updateCamera(float dt, bool allow_user_input)
{
  auto camera_handler = std::dynamic_pointer_cast<handler::Camera>(_tes->messageHandler(MtCamera));
  if (camera_handler)
  {
    if (_active_remote_camera != handler::Camera::kInvalidCameraId)
    {
      const auto config = _tes->settings().config().camera;
      // Check camera handler for an active camera.
      camera::Camera remote_camera = {};
      camera_handler->lookup(_active_remote_camera, remote_camera);
      if (!config.allow_remote_settings.value())
      {
        // Don't allow remote camera settings. Keep the user settings.
        remote_camera.clip_far = _camera.clip_far;
        remote_camera.clip_near = _camera.clip_near;
        remote_camera.fov_horizontal_deg = _camera.fov_horizontal_deg;
      }
      _tes->setCamera(remote_camera);
      allow_user_input = false;
    }
  }

  if (allow_user_input)
  {
    updateCameraInput(dt, _camera);
    _tes->setCamera(_camera);
  }
  else
  {
    _mouse_rotation_active = false;
  }
}

void Viewer::updateCameraInput(float dt, camera::Camera &camera)
{
  Magnum::Vector3i key_translation(0);
  Magnum::Vector3i key_rotation(0);
  for (const auto &key : _move_keys)
  {
    if (key.active)
    {
      key_translation[key.axis] += (!key.negate) ? 1 : -1;
    }
  }
  for (const auto &key : _rotate_keys)
  {
    if (key.active)
    {
      key_rotation[key.axis] += (!key.negate) ? 1 : -1;
    }
  }

  _fly.updateKeys(dt, key_translation, key_rotation, camera);
}


void Viewer::checkShortcuts(KeyEvent &event)
{
  if (!_commands || event.isRepeated())
  {
    return;
  }

  for (const auto &[name, shortcut] : _commands->commands())
  {
    if (checkShortcut(shortcut.shortcut, event) && shortcut.command->admissible(*this))
    {
      log::info("Invoke shortcut command '", shortcut.command->name(), "'");
      event.setAccepted();
      const auto result = shortcut.command->invoke(*this);
      switch (result.code())
      {
      case command::CommandResult::Code::Ok:
        log::info("Invoked shortcut command '", shortcut.command->name(), "'");
        break;
      case command::CommandResult::Code::Cancel:
        log::info("Cancelled shortcut command '", shortcut.command->name(), "'");
        break;
      default:
        log::error("Failed shortcut command '", shortcut.command->name(), "' : ", result.reason());
        break;
      }
      return;
    }
  }
}


bool Viewer::checkShortcut(const command::Shortcut &shortcut, const KeyEvent &event)
{
  if (event.key() != shortcut.key())
  {
    return false;
  }

  if ((static_cast<int>(event.modifiers()) & shortcut.modifierFlags()) != shortcut.modifierFlags())
  {
    return false;
  }

  return true;
}


Viewer::StartupMode Viewer::parseStartupArgs(const Arguments &arguments, CommandLineOptions &opt)
{
  const auto program_name = std::filesystem::path(arguments.argv[0]).filename().string();
  cxxopts::Options opt_parse(program_name, "3rd Eye Scene viewer.");

  try
  {
    // clang-format off
    opt_parse.add_options()
      ("help", "Show command line help.")
      ("file", "Start the UI and open this file for playback. Takes precedence over --host.", cxxopts::value(opt.filename))
      ("host", "Start the UI and open a connection to this host URL/IP. Use --port to select the port number.", cxxopts::value(opt.host))
      ("port", "The port number to use with --host", cxxopts::value(opt.port)->default_value(std::to_string(opt.port)))
      ;
    // clang-format on

    cxxopts::ParseResult parsed = opt_parse.parse(arguments.argc, arguments.argv);

    if (parsed.count("help"))
    {
      std::cout << opt_parse.help() << std::endl;
      // Help already shown.
      return StartupMode::Help;
    }
  }
  catch (const cxxopts::exceptions::parsing &e)
  {
    std::cerr << "Argument error\n" << e.what() << std::endl;
    return StartupMode::Error;
  }

  if (!opt.filename.empty())
  {
    return StartupMode::File;
  }

  if (!opt.host.empty())
  {
    return StartupMode::Host;
  }

  return StartupMode::Normal;
}


bool Viewer::handleStartupArgs(const Arguments &arguments)
{
  CommandLineOptions opt;
  const auto startup_mode = parseStartupArgs(arguments, opt);

  switch (startup_mode)
  {
  case StartupMode::Error:
  case StartupMode::Help:
    // Do not start UI.
    return false;
  case StartupMode::Normal:
    break;
  case StartupMode::File:
    open(opt.filename);
    break;
  case StartupMode::Host: {
    // Extract a port number if possible.
    connect(opt.host, opt.port, true);
    break;
  default:
    break;
  }
  }

  return true;
}
}  // namespace tes::view
