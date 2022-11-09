#ifndef TES_VIEWER_VIEWER_H
#define TES_VIEWER_VIEWER_H

#include "3es-viewer.h"

#include "camera/3esfly.h"

#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Platform/GlfwApplication.h>
#include <Magnum/Shaders/Phong.h>

#include <array>
#include <chrono>
#include <vector>

// TODO(KS): abstract away Magnum so it's not in any public headers.
namespace tes
{
class Viewer : public Magnum::Platform::Application
{
public:
  using Clock = std::chrono::steady_clock;

  explicit Viewer(const Arguments &arguments);

  void setContinuousSim(bool continuous);
  void checkContinuousSim();

private:
  void drawEvent() override;
  void mousePressEvent(MouseEvent &event) override;
  void mouseReleaseEvent(MouseEvent &event) override;
  void mouseMoveEvent(MouseMoveEvent &event) override;
  void keyPressEvent(KeyEvent &event) override;
  void keyReleaseEvent(KeyEvent &event) override;

  struct KeyAxis
  {
    KeyEvent::Key key;
    int axis = 0;
    bool negate = false;
    bool active = false;
  };

  struct InstanceData
  {
    Magnum::Matrix4 transform;
    Magnum::Matrix3x3 normal_matrix;
    Magnum::Color3 colour;
  };

  Clock::time_point _last_sim_time = Clock::now();

  std::array<InstanceData, 6> _instances;
  Magnum::GL::Buffer _instance_buffer{ Magnum::NoCreate };
  Magnum::GL::Mesh _mesh;

  Magnum::Shaders::Phong _shader{ Magnum::NoCreate };

  Magnum::Matrix4 _projection;
  camera::Camera _camera;
  camera::Fly _fly;

  bool _mouse_rotation_active = false;
  bool _continuous_sim = false;

  std::vector<KeyAxis> _move_keys;
  std::vector<KeyAxis> _rotate_keys;
};
}  // namespace tes

#endif  // TES_VIEWER_VIEWER_H
