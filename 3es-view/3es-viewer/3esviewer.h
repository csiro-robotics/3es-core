#ifndef TES_VIEWER_VIEWER_H
#define TES_VIEWER_VIEWER_H

#include "3es-viewer.h"

#include "camera/3esfly.h"
#include "3esthirdeyescene.h"

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

class Viewer : public Magnum::Platform::Application
{
public:
  using Clock = std::chrono::steady_clock;

  explicit Viewer(const Arguments &arguments);

  inline std::shared_ptr<ThirdEyeScene> tes() const { return _tes; }

  bool open(const std::filesystem::path &path);
  bool closeOrDisconnect();

  void setContinuousSim(bool continuous);
  void checkContinuousSim();

private:
  enum EdlParam
  {
    LinearScale,
    ExponentialScale,
    Radius
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
