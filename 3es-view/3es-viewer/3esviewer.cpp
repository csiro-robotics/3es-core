#include "3esviewer.h"

#include "3esedleffect.h"
#include "painter/3esarrow.h"
#include "painter/3esbox.h"
#include "painter/3escapsule.h"
#include "painter/3escylinder.h"
#include "painter/3esplane.h"
#include "painter/3espose.h"
#include "painter/3essphere.h"
#include "painter/3esstar.h"

#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Version.h>

// Things to learn about:
// - text rendering
// - UI

// Things to implement:
// - mesh renderer
// - point cloud rendering
//  - simple from vertex buffer
//  - with point shader
//  - voxel shader

namespace tes::viewer
{
Viewer::Viewer(const Arguments &arguments)
  : Magnum::Platform::Application{ arguments, Configuration{}.setTitle("3rd Eye Scene Viewer") }
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
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::Blending);

  _edl_effect = std::make_shared<EdlEffect>(Magnum::GL::defaultFramebuffer.viewport());
  _active_fbo_effect = _edl_effect;

  _culler = std::make_shared<BoundsCuller>();
  initialisePainters();
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


void Viewer::initialisePainters()
{
  _painters.emplace(SIdSphere, std::make_shared<painter::Sphere>(_culler));
  _painters.emplace(SIdBox, std::make_shared<painter::Box>(_culler));
  _painters.emplace(SIdCylinder, std::make_shared<painter::Cylinder>(_culler));
  _painters.emplace(SIdCapsule, std::make_shared<painter::Capsule>(_culler));
  _painters.emplace(SIdPlane, std::make_shared<painter::Plane>(_culler));
  _painters.emplace(SIdStar, std::make_shared<painter::Star>(_culler));
  _painters.emplace(SIdArrow, std::make_shared<painter::Arrow>(_culler));
  _painters.emplace(SIdPose, std::make_shared<painter::Pose>(_culler));

  Magnum::Matrix4 shape_transform = {};

  // Axis box markers
  _painters[SIdBox]->add(Id(2), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ 10, 0, 0 }) * shape_transform, Magnum::Color4{ 1, 0, 0 });
  _painters[SIdBox]->add(Id(3), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ 0, 10, 0 }) * shape_transform, Magnum::Color4{ 0, 1, 0 });
  _painters[SIdBox]->add(Id(4), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ 0, 0, 10 }) * shape_transform, Magnum::Color4{ 0, 0, 1 });
  _painters[SIdBox]->add(Id(5), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ -10, 0, 0 }) * shape_transform, Magnum::Color4{ 0, 1, 1 });
  _painters[SIdBox]->add(Id(6), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ 0, -10, 0 }) * shape_transform, Magnum::Color4{ 1, 0, 1 });
  _painters[SIdBox]->add(Id(7), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ 0, 0, -10 }) * shape_transform, Magnum::Color4{ 1, 1, 0 });

  // Add debug shapes.
  float x = 0;
  shape_transform = {};
  _painters[SIdSphere]->add(Id(1), painter::ShapePainter::Type::Solid,
                            Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform, Magnum::Color4{ 1, 1, 0 });
  _painters[SIdSphere]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                            Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform, Magnum::Color4{ 0, 1, 1 });
  _painters[SIdSphere]->add(Id(1), painter::ShapePainter::Type::Transparent,
                            Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                            Magnum::Color4{ 1, 0, 1, 0.4f });

  x = -2.5f;
  shape_transform = {};
  _painters[SIdBox]->add(Id(1), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform, Magnum::Color4{ 1, 0, 0 });
  _painters[SIdBox]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                         Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform, Magnum::Color4{ 0, 1, 1 });
  _painters[SIdBox]->add(Id(1), painter::ShapePainter::Type::Transparent,
                         Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform, Magnum::Color4{ 1, 0, 1, 0.4f });

  x = 2.5f;
  shape_transform = Magnum::Matrix4::rotationX(Magnum::Deg(35.0f)) * Magnum::Matrix4::scaling({ 0.3f, 0.3f, 1.0f });
  _painters[SIdCylinder]->add(Id(1), painter::ShapePainter::Type::Solid,
                              Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform, Magnum::Color4{ 1, 1, 0 });
  _painters[SIdCylinder]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                              Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform, Magnum::Color4{ 0, 1, 1 });
  _painters[SIdCylinder]->add(Id(1), painter::ShapePainter::Type::Transparent,
                              Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                              Magnum::Color4{ 1, 0, 1, 0.4f });

  x = -5.0f;
  shape_transform = Magnum::Matrix4::rotationX(Magnum::Deg(35.0f)) * Magnum::Matrix4::scaling({ 0.3f, 0.3f, 1.0f });
  _painters[SIdCapsule]->add(Id(1), painter::ShapePainter::Type::Solid,
                             Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform, Magnum::Color4{ 1, 1, 0 });
  _painters[SIdCapsule]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                             Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform, Magnum::Color4{ 0, 1, 1 });
  _painters[SIdCapsule]->add(Id(1), painter::ShapePainter::Type::Transparent,
                             Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                             Magnum::Color4{ 1, 0, 1, 0.4f });

  x = 7.5f;
  shape_transform = Magnum::Matrix4::rotationX(Magnum::Deg(35.0f)) * Magnum::Matrix4::scaling({ 1.0f, 1.0f, 1.0f });
  _painters[SIdPlane]->add(Id(1), painter::ShapePainter::Type::Solid,
                           Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform, Magnum::Color4{ 1, 1, 0 });
  _painters[SIdPlane]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                           Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform, Magnum::Color4{ 0, 1, 1 });
  _painters[SIdPlane]->add(Id(1), painter::ShapePainter::Type::Transparent,
                           Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                           Magnum::Color4{ 1, 0, 1, 0.4f });

  x = -7.5f;
  shape_transform = Magnum::Matrix4::scaling({ 1.0f, 1.0f, 1.0f });
  _painters[SIdStar]->add(Id(1), painter::ShapePainter::Type::Solid,
                          Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform, Magnum::Color4{ 1, 1, 0 });
  _painters[SIdStar]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                          Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform, Magnum::Color4{ 0, 1, 1 });
  _painters[SIdStar]->add(Id(1), painter::ShapePainter::Type::Transparent,
                          Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform, Magnum::Color4{ 1, 0, 1, 0.4f });

  x = 10.0f;
  shape_transform = Magnum::Matrix4::rotationX(Magnum::Deg(35.0f)) * Magnum::Matrix4::scaling({ 0.1f, 0.1f, 1.0f });
  _painters[SIdArrow]->add(Id(1), painter::ShapePainter::Type::Solid,
                           Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform, Magnum::Color4{ 1, 1, 0 });
  _painters[SIdArrow]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                           Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform, Magnum::Color4{ 0, 1, 1 });
  _painters[SIdArrow]->add(Id(1), painter::ShapePainter::Type::Transparent,
                           Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                           Magnum::Color4{ 1, 0, 1, 0.4f });

  x = -10.0f;
  shape_transform = Magnum::Matrix4::rotationX(Magnum::Deg(35.0f)) * Magnum::Matrix4::scaling({ 1.0f, 1.0f, 1.0f });
  _painters[SIdPose]->add(Id(1), painter::ShapePainter::Type::Solid,
                          Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform, Magnum::Color4{ 1, 1, 1 });
  _painters[SIdPose]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                          Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform, Magnum::Color4{ 1, 1, 1 });
  _painters[SIdPose]->add(Id(1), painter::ShapePainter::Type::Transparent,
                          Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform, Magnum::Color4{ 1, 0, 1, 0.4f });

  for (auto &painter : _painters)
  {
    painter.second->commit();
  }
}


void Viewer::drawEvent()
{
  using namespace Magnum::Math::Literals;

  const auto now = Clock::now();
  const auto delta_time = now - _last_sim_time;
  _last_sim_time = now;
  const float dt = std::chrono::duration_cast<std::chrono::duration<float>>(delta_time).count();

  updateCamera(dt);

  auto projection_matrix = camera::viewProjection(_camera, Magnum::Vector2(windowSize()));
  ++_render_stamp.render_mark;
  _culler->cull(_render_stamp.render_mark, Magnum::Frustum::fromMatrix(projection_matrix));

  if (_active_fbo_effect)
  {
    _active_fbo_effect->prepareFrame(projection_matrix, FboEffect::ProjectionType::Perspective, _camera.clip_near,
                                     _camera.clip_far);
  }
  else
  {
    Magnum::GL::defaultFramebuffer.clear(Magnum::GL::FramebufferClear::Color | Magnum::GL::FramebufferClear::Depth)
      .bind();
  }

  drawShapes(dt, projection_matrix);

  if (_active_fbo_effect)
  {
    Magnum::GL::defaultFramebuffer.bind();
    Magnum::GL::defaultFramebuffer.clear(Magnum::GL::FramebufferClear::Color | Magnum::GL::FramebufferClear::Depth);
    _active_fbo_effect->completeFrame();
  }

  swapBuffers();
  if (_continuous_sim)
  {
    redraw();
  }
}

void Viewer::viewportEvent(ViewportEvent &event)
{
  _edl_effect->viewportChange(Magnum::GL::defaultFramebuffer.viewport());
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

  dirty = checkEdlKeys(event) || dirty;

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


bool Viewer::checkEdlKeys(KeyEvent &event)
{
  bool dirty = false;
  if (event.key() == KeyEvent::Key::Tab)
  {
    bool edl_on = false;
    if (!_active_fbo_effect)
    {
      _active_fbo_effect = _edl_effect;
      edl_on = true;
    }
    else
    {
      _active_fbo_effect = nullptr;
    }
    event.setAccepted(true);
    dirty = true;
    Magnum::Debug() << "EDL: " << (edl_on ? "on" : "off");
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
      _edl_effect->setLinearScale(_edl_effect->linearScale() + delta);
      Magnum::Debug() << "EDL linear scale: " << _edl_effect->linearScale();
      event.setAccepted(true);
      dirty = true;
      break;
    case EdlParam::ExponentialScale:
      _edl_effect->setExponentialScale(_edl_effect->exponentialScale() + delta);
      Magnum::Debug() << "EDL exponential scale: " << _edl_effect->exponentialScale();
      event.setAccepted(true);
      dirty = true;
      break;
    case EdlParam::Radius:
      _edl_effect->setRadius(_edl_effect->radius() + delta);
      Magnum::Debug() << "EDL radius scale: " << _edl_effect->radius();
      event.setAccepted(true);
      dirty = true;
      break;
    default:
      break;
    }
  }

  return dirty;
}


void Viewer::updateCamera(float dt)
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

  _fly.updateKeys(dt, key_translation, key_rotation, _camera);
}


void Viewer::drawShapes(float dt, const Magnum::Matrix4 &projection_matrix)
{
  // Draw opaque then transparent for proper blending.
  for (const auto &[id, painter] : _painters)
  {
    painter->drawOpaque(_render_stamp, projection_matrix);
  }
  for (const auto &[id, painter] : _painters)
  {
    painter->drawTransparent(_render_stamp, projection_matrix);
  }
}
}  // namespace tes::viewer
