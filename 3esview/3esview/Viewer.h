#ifndef TES_VIEWER_VIEWER_H
#define TES_VIEWER_VIEWER_H

#include "3esview/ViewConfig.h"

#include "camera/Fly.h"
#include "ThirdEyeScene.h"

#include <filesystem>

namespace tes::viewer
{
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
  static uint16_t defaultPort();

  explicit Viewer(const Arguments &arguments);
  ~Viewer();

  inline std::shared_ptr<ThirdEyeScene> tes() const { return _tes; }

  bool open(const std::filesystem::path &path);
  bool connect(const std::string &host, uint16_t port, bool allow_reconnect = true);
  bool closeOrDisconnect();

  void setContinuousSim(bool continuous);
  bool continuousSim();

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

  void drawEvent() override;
  void viewportEvent(ViewportEvent &event) override;
  void mousePressEvent(MouseEvent &event) override;
  void mouseReleaseEvent(MouseEvent &event) override;
  void mouseMoveEvent(MouseMoveEvent &event) override;
  void keyPressEvent(KeyEvent &event) override;
  void keyReleaseEvent(KeyEvent &event) override;

  bool checkEdlKeys(KeyEvent &event);

  void updateCamera(float dt, camera::Camera &camera);

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

  Clock::time_point _last_sim_time = Clock::now();

  camera::Fly _fly;

  bool _mouse_rotation_active = false;
  bool _continuous_sim = false;

  std::vector<KeyAxis> _move_keys;
  std::vector<KeyAxis> _rotate_keys;
};
}  // namespace tes::viewer

#endif  // TES_VIEWER_VIEWER_H
