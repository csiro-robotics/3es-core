#ifndef TES_VIEWER_VIEWER_H
#define TES_VIEWER_VIEWER_H

#include "3es-viewer.h"

#include "camera/3esfly.h"

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
#include <memory>

// TODO(KS): abstract away Magnum so it's not in any public headers.
namespace tes
{
namespace shaders
{
class Edl;
}  // namespace shaders

class Viewer : public Magnum::Platform::Application
{
public:
  using Clock = std::chrono::steady_clock;

  explicit Viewer(const Arguments &arguments);

  void setContinuousSim(bool continuous);
  void checkContinuousSim();

private:
  void drawEvent() override;
  void viewportEvent(ViewportEvent &event) override;
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
    Magnum::Color3 colour;
  };

  struct Edl
  {
    struct Settings
    {
      float radius = 4.0f;
      float linear_scale = 4.0f;
      float exponential_scale = 4.0f;
      float near_clip = 1.0f;
      float far_clip = 100.0f;
      Magnum::Vector2i view_size{ 1 };
    };

    Magnum::GL::Texture2D colour_buffer;
    Magnum::GL::Texture2D depth_buffer;
    Magnum::GL::Framebuffer frame_buffer{ Magnum::NoCreate };
    std::unique_ptr<shaders::Edl> shader;
    Settings settings;
    Magnum::GL::Mesh mesh;
    bool enabled = false;

    void init(const Magnum::Range2Di &viewport);
    void resize(const Magnum::Vector2i &size);
    void blit(float near_clip, float far_clip);
  } _edl;

  Clock::time_point _last_sim_time = Clock::now();

  std::array<InstanceData, 6> _instances;
  Magnum::GL::Buffer _instance_buffer{ Magnum::NoCreate };
  Magnum::GL::Mesh _mesh;

  Magnum::Shaders::Flat3D _shader{ Magnum::NoCreate };

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
