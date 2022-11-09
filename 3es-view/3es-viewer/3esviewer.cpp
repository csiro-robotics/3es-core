#include "3es-viewer.h"

#include <3escoordinateframe.h>

#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/GlfwApplication.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Shaders/VertexColor.h>
#include <Magnum/Trade/MeshData.h>

#include <array>
#include <chrono>

// Things to learn about:
// - primitives example
// - shaders example
// - instancing
// - mesh building
// - text rendering
// - UI

// Things to implement:
// - camera control
// - primitive instance rendering
// - shaders:
//  - simple primitives
//  - transparent
//  - wireframe
// - mesh renderer
// - point cloud rendering
//  - simple from vertex buffer
//  - with point shader
// - EDL shader
//  - render to texture
//  - blit with EDL shader

namespace tes
{
struct Camera
{
  Magnum::Vector3 position;
  float pitch = 0;
  float yaw = 0;
  float fov_horizontal = float(Magnum::Rad(Magnum::Deg(70.0f)));
  float clip_near = 0.1f;
  float clip_far = 1000.0f;
  // TODO: apply this frame. For now just use XYZ.
  tes::CoordinateFrame frame;
};

/// Calculate the camera world transform. This is in X right, Y forward, Z up.
inline Magnum::Matrix4 cameraTransform(const Camera &camera)
{
  Magnum::Matrix4 transform = Magnum::Matrix4::translation(camera.position) *        //
                              Magnum::Matrix4::rotationZ(Magnum::Rad(camera.yaw)) *  //
                              Magnum::Matrix4::rotationX(Magnum::Rad(camera.pitch));
  return transform;
}

/// Base class for camera update.
class CameraController
{
public:
  /// Control flags.
  enum class Flag : unsigned
  {
    Zero = 0,
    InvertKeyMoveX = (1 << 0u),
    InvertKeyMoveY = (1 << 1u),
    InvertKeyMoveZ = (1 << 2u),
    InvertKeyRotateX = (1 << 3u),
    InvertKeyRotateY = (1 << 4u),
    InvertKeyRotateZ = (1 << 5u),
    InvertMouseX = (1 << 6u),
    InvertMouseY = (1 << 7u),
    InvertKeyX = InvertKeyMoveX | InvertKeyRotateX,
    InvertKeyY = InvertKeyMoveY | InvertKeyRotateY,
    InvertKeyZ = InvertKeyMoveZ | InvertKeyRotateZ,
  };

  /// Virtual destructor.
  virtual ~CameraController() = default;

  /// Get the current control flags.
  inline Flag flags() const { return _flags; }
  /// Set the current control flags.
  inline void setFlags(Flag flags) { _flags = flags; }

  /// Set the given control flag(s). The @p flag value may contain multiple flags, but is generally expected to be a
  /// single flag.
  inline void set(Flag flag) { _flags = Flag(unsigned(_flags) | unsigned(flag)); }
  /// Clear the given control flag(s). The @p flag value may contain multiple flags, but is generally expected to be a
  /// single flag.
  inline void clear(Flag flag) { _flags = Flag(unsigned(_flags) & ~unsigned(flag)); }
  /// Check fi the given control flags are set. The @p flag value may contain multiple flags, but the return value is
  /// only true if *all* flag bits are set.
  inline bool isSet(Flag flag) const { return unsigned(_flags) & unsigned(flag) == unsigned(flag); }

  /// Perform mouse movement update logic.
  /// @param dx Mouse movement delta in x.
  /// @param dy Mouse movement delta in y.
  /// @param camera Camera to update.
  virtual void updateMouse(float dx, float dy, Camera &camera) = 0;

  /// Perform keyboard camera control update logic.
  /// @param dt Time delta (seconds).
  /// @param translate Current translation to apply per axis.
  /// @param rotate Current rotation to apply per axis.
  /// @param camera Camera to update. Only uses X and Y; X => pitch, Y => yaw.
  virtual void updateKeys(float dt, Magnum::Vector3i translate, Magnum::Vector3i rotate, Camera &camera) = 0;

  /// @overload
  void updateKeys(float dt, Magnum::Vector3i translate, Camera &camera)
  {
    updateKeys(dt, translate, Magnum::Vector3i(0.), camera);
  }

  inline static void clampRotation(Camera &camera)
  {
    camera.pitch = std::max(-float(0.5 * M_PI), std::min(camera.pitch, float(0.5 * M_PI)));
    while (camera.yaw >= float(2.0 * M_PI))
    {
      camera.yaw -= float(M_PI);
    }
    while (camera.yaw < 0.0f)
    {
      camera.yaw += float(M_PI);
    }
  }

protected:
  Flag _flags = Flag::Zero;
};

class FlyCameraController : public CameraController
{
public:
  /// Get the movement speed for key translation updates: m/s.
  inline float moveSpeed() const { return _move_speed; }
  /// Set the movement speed for key translation updates: m/s.
  inline void setMoveSpeed(float move_speed) { _move_speed = move_speed; }
  /// Get the rotation speed for key rotation updates: radians/s.
  inline float rotationSpeed() const { return _rotation_speed; }
  /// Set the rotation speed for key rotation updates: radians/s.
  inline void setRotationSpeed(float rotation_speed) { _rotation_speed = rotation_speed; }
  /// Get the mouse sensitivity: radians/pixel.
  inline float mouseSensitivity() const { return _mouse_sensitivity; }
  /// Set the mouse sensitivity: radians/pixel.
  inline void setMouseSensitivity(float mouse_sensitivity) { _mouse_sensitivity = mouse_sensitivity; }
  /// Get the movement key speed multiplier.
  inline float moveMultiplier() const { return _move_multiplier; }
  /// Set the movement key speed multiplier.
  inline void setMoveMultiplier(float move_multiplier) { _move_multiplier = move_multiplier; }
  /// Get the rotation key speed multiplier.
  inline float rotationMultiplier() const { return _rotation_multiplier; }
  /// Set the rotation key speed multiplier.
  inline void setRotationMultiplier(float rotation_multiplier) { _rotation_multiplier = rotation_multiplier; }
  /// Get the mouse sensitivity multiplier.
  inline float mouseMultiplier() const { return _mouse_multiplier; }
  /// Set the mouse sensitivity multiplier.
  inline void setMouseMultiplier(float mouse_multiplier) { _mouse_multiplier = mouse_multiplier; }

  void updateMouse(float dx, float dy, Camera &camera) override;

  void updateKeys(float dt, Magnum::Vector3i translate, Magnum::Vector3i rotate, Camera &camera) override;

private:
  /// Movement speed for key translation updates: m/s.
  float _move_speed = 8.0f;
  /// Rotation speed for key rotation updates: radians/s.
  float _rotation_speed = float(Magnum::Rad(Magnum::Deg(90.0f)));
  /// Mouse sensitivity: radians/pixel.
  float _mouse_sensitivity = float(Magnum::Rad(Magnum::Deg(5.0f)));
  /// Current movement multiplier.
  float _move_multiplier = 1.0f;
  /// Current rotation multiplier.
  float _rotation_multiplier = 1.0f;
  /// Current mouse sensitivity multiplier.
  float _mouse_multiplier = 1.0f;
};

void FlyCameraController::updateMouse(float dx, float dy, Camera &camera)
{
  if (isSet(Flag::InvertMouseY))
  {
    dy *= -1.0f;
  }
  if (isSet(Flag::InvertMouseX))
  {
    dx *= -1.0f;
  }
  camera.pitch += dy * _mouse_sensitivity * _mouse_multiplier;
  camera.yaw += dx * _mouse_sensitivity * _mouse_multiplier;

  clampRotation(camera);
}

void FlyCameraController::updateKeys(float dt, Magnum::Vector3i translate, Magnum::Vector3i rotate, Camera &camera)
{
  if (isSet(Flag::InvertKeyMoveX))
  {
    translate.x() *= -1;
  }
  if (isSet(Flag::InvertKeyMoveY))
  {
    translate.y() *= -1;
  }
  if (isSet(Flag::InvertKeyMoveZ))
  {
    translate.z() *= -1;
  }
  if (isSet(Flag::InvertKeyRotateX))
  {
    rotate.x() *= -1;
  }
  if (isSet(Flag::InvertKeyRotateY))
  {
    rotate.y() *= -1;
  }
  if (isSet(Flag::InvertKeyRotateZ))
  {
    rotate.z() *= -1;
  }

  const auto delta_translate = Magnum::Vector3(_move_speed * _move_multiplier * dt) * Magnum::Vector3(translate);
  const auto delta_rotate = Magnum::Vector3(_rotation_speed * _rotation_multiplier * dt) * Magnum::Vector3(rotate);

  camera.pitch += delta_rotate.x();
  camera.yaw += delta_rotate.y();
  clampRotation(camera);

  // Get the camera transform to extract the translation axes.
  Magnum::Matrix4 camera_transform = cameraTransform(camera);

  camera.position += camera_transform[0].xyz() * delta_translate.x();
  camera.position += camera_transform[1].xyz() * delta_translate.y();
  camera.position += camera_transform[2].xyz() * delta_translate.z();
}

/// Calculate the camera view matrix.
inline Magnum::Matrix4 cameraView(const Camera &camera)
{
  Magnum::Matrix4 frame_transform;
  // Build the transform to map into (OpenGL/Vulkan) view space: X right, -Z forward, Y up.
  switch (camera.frame)
  {
  // TODO(KS): fix the other coordinate frames.
  default:
  case tes::CoordinateFrame::XYZ:
    frame_transform = { { 1, 0, 0, 0 },   //
                        { 0, 0, -1, 0 },  //
                        { 0, 1, 0, 0 },   //
                        { 0, 0, 0, 1 } };
    break;
  }
  return frame_transform * cameraTransform(camera).inverted();
}

/// Generate the camera projection matrix.
inline Magnum::Matrix4 cameraProjection(const Camera &camera, Magnum::Vector2 &view_size)
{
  Magnum::Matrix4 projection = Magnum::Matrix4::perspectiveProjection(
    Magnum::Math::Rad(camera.fov_horizontal), view_size.aspectRatio(), camera.clip_near, camera.clip_far);
  Magnum::Matrix4 camera_transform = cameraView(camera);
  return projection * camera_transform;
}


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

  struct MeshAttributes
  {
    Magnum::Matrix4 transform;
    Magnum::Color3 colour;
  };

  Clock::time_point _last_sim_time = Clock::now();

  Magnum::GL::Mesh _mesh;

  MeshAttributes _mesh_attrs[6];
  Magnum::Shaders::Phong _shader;

  Magnum::Matrix4 _projection;
  Camera _camera;
  FlyCameraController _fly;

  bool _mouse_rotation_active = false;
  bool _continuous_sim = false;

  std::vector<KeyAxis> _move_keys;
  std::vector<KeyAxis> _rotate_keys;
};

Viewer::Viewer(const Arguments &arguments)
  : Magnum::Platform::Application{ arguments, Configuration{}.setTitle("3es Viewer") }
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
  using namespace Magnum::Math::Literals;

  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::DepthTest);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::FaceCulling);

  _mesh = Magnum::MeshTools::compile(Magnum::Primitives::cubeSolid());

  const float scale = 20.0f;
  _mesh_attrs[0] = { Magnum::Matrix4::translation({ scale, 0.0f, 0.0f }), 0xff0000_rgbf };
  _mesh_attrs[1] = { Magnum::Matrix4::translation({ -scale, 0.0f, 0.0f }), 0x00ffff_rgbf };
  _mesh_attrs[2] = { Magnum::Matrix4::translation({ 0.0f, scale, 0.0f }), 0x00ff00_rgbf };
  _mesh_attrs[3] = { Magnum::Matrix4::translation({ 0.0f, -scale, 0.0f }), 0xff00ff_rgbf };
  _mesh_attrs[4] = { Magnum::Matrix4::translation({ 0.0f, 0.0f, scale }), 0x0000ff_rgbf };
  _mesh_attrs[5] = { Magnum::Matrix4::translation({ 0.0f, 0.0f, -scale }), 0xffff00_rgbf };

  _projection =
    Magnum::Matrix4::perspectiveProjection(35.0_degf, Magnum::Vector2{ windowSize() }.aspectRatio(), 0.01f, 1000.0f) *
    Magnum::Matrix4::translation(Magnum::Vector3::zAxis(-10.0f));
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

void Viewer::checkContinuousSim()
{
  bool continuous_sim = _mouse_rotation_active;
  for (const auto &key : _move_keys)
  {
    continuous_sim = key.active || continuous_sim;
  }
  for (const auto &key : _rotate_keys)
  {
    continuous_sim = key.active || continuous_sim;
  }
  setContinuousSim(continuous_sim);
}

void Viewer::drawEvent()
{
  const auto now = Clock::now();
  const auto delta_time = now - _last_sim_time;
  _last_sim_time = now;

  const float dt = std::chrono::duration_cast<std::chrono::duration<float>>(delta_time).count();

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

  _fly.updateKeys(dt, key_translation, key_rotation, _camera);

  Magnum::GL::defaultFramebuffer.clear(Magnum::GL::FramebufferClear::Color);

  // TODO: Add your drawing code here
  using namespace Magnum::Math::Literals;

  auto projection_matrix = cameraProjection(_camera, Magnum::Vector2(windowSize()));

  Magnum::GL::defaultFramebuffer.clear(Magnum::GL::FramebufferClear::Color | Magnum::GL::FramebufferClear::Depth);

  _shader
    .setLightPositions({ { 1.4f, 1.0f, 0.75f } })  //
    // .setAmbientColor(0x7f7f7f_rgbf)                //
    // .setProjectionMatrix(_projection);
    .setProjectionMatrix(projection_matrix);

  for (int i = 0; i < 6; ++i)
  {
    _shader.setNormalMatrix(_mesh_attrs[i].transform.normalMatrix())
      .setTransformationMatrix(_mesh_attrs[i].transform)
      .setDiffuseColor(_mesh_attrs[i].colour)
      .draw(_mesh);
  }

  swapBuffers();
  if (_continuous_sim)
  {
    redraw();
  }
}

void Viewer::mousePressEvent(MouseEvent &event)
{
  if (event.button() != MouseEvent::Button::Left)
    return;

  _mouse_rotation_active = true;
  setContinuousSim(true);
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

  _fly.updateMouse(event.relativePosition().x(), event.relativePosition().y(), _camera);

  event.setAccepted();
  redraw();
  checkContinuousSim();
}

void Viewer::keyPressEvent(KeyEvent &event)
{
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
    _camera.position.y() -= 0.1f;
    dirty = true;
    event.setAccepted();
  }

  if (dirty)
  {
    setContinuousSim(true);
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
    checkContinuousSim();
    redraw();
  }
}
}  // namespace tes

MAGNUM_APPLICATION_MAIN(tes::Viewer)
