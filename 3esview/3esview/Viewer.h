#ifndef TES_VIEW_VIEWER_H
#define TES_VIEW_VIEWER_H

#include "3esview/ViewConfig.h"

#include "camera/Fly.h"
#include "handler/Camera.h"
#include "settings/Settings.h"
#include "ThirdEyeScene.h"

#include <filesystem>

namespace tes::view
{
namespace command
{
class Set;
class Shortcut;
}  // namespace command

namespace shaders
{
class Edl;
}  // namespace shaders

class EdlEffect;
class FboEffect;
class DataThread;

class TES_VIEWER_API Viewer : public Magnum::Platform::Application
{
public:
  using Clock = std::chrono::steady_clock;

  /// Get the default 3es server port.
  /// @return The default server port for 3es.
  [[nodiscard]] static uint16_t defaultPort();

  explicit Viewer(const Arguments &arguments);
  ~Viewer();

  [[nodiscard]] std::shared_ptr<ThirdEyeScene> tes() const { return _tes; }

  [[nodiscard]] const std::shared_ptr<DataThread> &dataThread() const { return _data_thread; }
  [[nodiscard]] std::shared_ptr<command::Set> commands() { return _commands; }
  [[nodiscard]] const std::shared_ptr<command::Set> &commands() const { return _commands; }


  bool open(const std::filesystem::path &path);
  bool connect(const std::string &host, uint16_t port, bool allow_reconnect = true);
  bool closeOrDisconnect();

  void setContinuousSim(bool continuous);
  [[nodiscard]] bool continuousSim();

protected:
  /// Return value for @c onDrawStart()
  enum class DrawMode
  {
    /// Normal drawing.
    Normal,
    /// Modal drawing - disable normal input mode and key responses.
    /// Useful for when a UI has focus.
    Modal
  };

  /// Hook function called at the start of @c drawEvent(). Allows extensions such as UI.
  /// @param dt Time elapsed since the last draw (seconds).
  /// @return The @c DrawMode to proceed with.
  virtual DrawMode onDrawStart(float dt)
  {
    TES_UNUSED(dt);
    return DrawMode::Normal;
  }

  /// Hook function called at the start of @c drawEvent() before @c swapBuffers(). Allows extensions
  /// such as UI.
  /// @param dt Time elapsed since the last draw (seconds).
  virtual void onDrawComplete(float dt) { TES_UNUSED(dt); }

  void drawEvent() override;
  void viewportEvent(ViewportEvent &event) override;
  void keyPressEvent(KeyEvent &event) override;
  void keyReleaseEvent(KeyEvent &event) override;
  void mousePressEvent(MouseEvent &event) override;
  void mouseReleaseEvent(MouseEvent &event) override;
  void mouseMoveEvent(MouseMoveEvent &event) override;

  virtual void onReset();
  virtual void onCameraSettingsChange(const settings::Settings::Config &config);
  virtual void onRenderSettingsChange(const settings::Settings::Config &config);
  virtual void onPlaybackSettingsChange(const settings::Settings::Config &config);

private:
  enum class EdlParam
  {
    LinearScale,
    ExponentialScale,
    Radius
  };

  struct CommandLineOptions
  {
    std::string filename;
    std::string host;
    uint16_t port = Viewer::defaultPort();
  };

  /// Return values from @c handleStartupArgs() which indicate what how to start.
  enum class StartupMode
  {
    /// An error has occured parsing the command line options. Best to show help and quit.
    Error,
    /// Normal UI startup mode.
    Normal,
    /// Show help and exit.
    Help,
    /// Start the UI and open a file.
    File,
    /// Start the UI and open a network connection.
    Host
  };

  bool checkEdlKeys(KeyEvent &event);

  void updateCamera(float dt, bool allow_user_input);
  void updateCameraInput(float dt, camera::Camera &camera);

  void checkShortcuts(KeyEvent &event);
  static bool checkShortcut(const command::Shortcut &shortcut, const KeyEvent &event);

  StartupMode parseStartupArgs(const Arguments &arguments, CommandLineOptions &opt);
  bool handleStartupArgs(const Arguments &arguments);

  struct KeyAxis
  {
    KeyEvent::Key key;
    int axis = 0;
    bool negate = false;
    bool active = false;
  };

  std::shared_ptr<EdlEffect> _edl_effect;
  EdlParam _edl_tweak = EdlParam::LinearScale;

  std::shared_ptr<ThirdEyeScene> _tes;
  std::shared_ptr<DataThread> _data_thread;
  std::shared_ptr<command::Set> _commands;

  Clock::time_point _last_sim_time = Clock::now();

  camera::Camera _camera = {};
  handler::Camera::CameraId _active_remote_camera = handler::Camera::kInvalidCameraId;
  camera::Fly _fly;

  bool _mouse_rotation_active = false;
  bool _continuous_sim = true;

  std::vector<KeyAxis> _move_keys;
  std::vector<KeyAxis> _rotate_keys;
};
}  // namespace tes::view

#endif  // TES_VIEW_VIEWER_H
