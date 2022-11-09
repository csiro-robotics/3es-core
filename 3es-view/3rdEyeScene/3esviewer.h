#ifndef TES_VIEWER_VIEWER_H
#define TES_VIEWER_VIEWER_H

#include "3es-viewer.h"

#include "camera/3esfly.h"

#include "3esboundsculler.h"
#include "painter/3esshapecache.h"
#include "painter/3esshapepainter.h"

#include <3esmessages.h>

#include <Magnum/GL/FrameBuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Platform/GlfwApplication.h>
#include <Magnum/Shaders/Flat.h>

#include <array>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <memory>

// TODO(KS): abstract away Magnum so it's not in any public headers.
namespace tes::viewer
{
namespace shaders
{
class Edl;
}  // namespace shaders

class EdlEffect;
class FboEffect;

class Viewer : public Magnum::Platform::Application
{
public:
  using Clock = std::chrono::steady_clock;

  explicit Viewer(const Arguments &arguments);

  void setContinuousSim(bool continuous);
  void checkContinuousSim();

private:
  enum EdlParam
  {
    LinearScale,
    ExponentialScale,
    Radius
  };

  void initialisePainters();

  void drawEvent() override;
  void viewportEvent(ViewportEvent &event) override;
  void mousePressEvent(MouseEvent &event) override;
  void mouseReleaseEvent(MouseEvent &event) override;
  void mouseMoveEvent(MouseMoveEvent &event) override;
  void keyPressEvent(KeyEvent &event) override;
  void keyReleaseEvent(KeyEvent &event) override;

  bool checkEdlKeys(KeyEvent &event);

  void updateCamera(float dt);
  void drawShapes(float dt, const Magnum::Matrix4 &projection_matrix);

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
    Magnum::Color3 colour;
  };

  std::shared_ptr<FboEffect> _active_fbo_effect;
  std::shared_ptr<EdlEffect> _edl_effect;
  EdlParam _edl_tweak = EdlParam::LinearScale;

  Clock::time_point _last_sim_time = Clock::now();

  Magnum::Matrix4 _projection;
  camera::Camera _camera;
  camera::Fly _fly;

  std::shared_ptr<BoundsCuller> _culler;

  std::unordered_map<ShapeHandlerIDs, std::shared_ptr<painter::ShapePainter>> _painters;

  unsigned _mark = 0;

  bool _mouse_rotation_active = false;
  bool _continuous_sim = false;

  std::vector<KeyAxis> _move_keys;
  std::vector<KeyAxis> _rotate_keys;
};
}  // namespace tes::viewer

#endif  // TES_VIEWER_VIEWER_H
