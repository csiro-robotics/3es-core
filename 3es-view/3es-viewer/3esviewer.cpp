#include "3esviewer.h"

#include "shaders/3esedl.h"

#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Version.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Shaders/VertexColor.h>
#include <Magnum/Trade/MeshData.h>

// Things to learn about:
// - render to texture / frame buffer object (for EDL)
// - mesh building
// - text rendering
// - UI

// Things to implement:
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
void Viewer::Edl::init(const Magnum::Range2Di &viewport)
{
  colour_texture.setStorage(1, Magnum::GL::TextureFormat::RGBA8, viewport.size());
  depth_texture.setStorage(1, Magnum::GL::TextureFormat::DepthComponent32F, viewport.size());
  // depth_texture.setCompareFunction(Magnum::GL::SamplerCompareFunction::Always);

  colour_texture.setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge);
  depth_texture.setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge);

  frame_buffer = Magnum::GL::Framebuffer(viewport);
  frame_buffer.attachTexture(Magnum::GL::Framebuffer::ColorAttachment{ 0 }, colour_texture, 0);
  frame_buffer.attachTexture(Magnum::GL::Framebuffer::BufferAttachment::Depth, depth_texture, 0);

  if (!shader)
  {
    shader = std::make_unique<shaders::Edl>();
  }

  shader
    ->bindColourTexture(colour_texture)  //
    .bindDepthBuffer(depth_texture)      //
    .setScreenParams(viewport.size());

  struct QuadVertex
  {
    Magnum::Vector3 position;
    Magnum::Vector2 textureCoordinates;
  };
  const QuadVertex vertices[]{
    { { 1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } },  /* Bottom right */
    { { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },   /* Top right */
    { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } }, /* Bottom left */
    { { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } }   /* Top left */
  };
  const Magnum::UnsignedInt indices[]{
    // 3--1 1
    0, 1, 2,  // | / /|
    2, 1, 3   // |/ / |
  };          // 2 2--0

  mesh.setCount(Magnum::Containers::arraySize(indices))
    .addVertexBuffer(Magnum::GL::Buffer{ vertices }, 0, shaders::Edl::Position{}, shaders::Edl::TextureCoordinates{})
    .setIndexBuffer(Magnum::GL::Buffer{ indices }, 0, Magnum::GL::MeshIndexType::UnsignedInt);
}


void Viewer::Edl::resize(const Magnum::Vector2i &size)
{
  // For now just try recreating everything.
  init(Magnum::GL::defaultFramebuffer.viewport());
}


void Viewer::Edl::blit(float near_clip, float far_clip)
{
  Magnum::Matrix4 projection;
  Magnum::GL::Renderer::setDepthMask(false);
  Magnum::GL::Renderer::setStencilMask(false);
  shader->setProjectionMatrix(projection)
    .setClipParams(near_clip, far_clip)
    .setRadius(settings.radius)
    .setLinearScale(settings.linear_scale)
    .setExponentialScale(settings.exponential_scale)
    .setLightDirection(settings.light_direction);
  shader->draw(mesh);
  Magnum::GL::Renderer::setDepthMask(true);
}


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

  const float scale = 20.0f;
  _instances[0] = { Magnum::Matrix4::translation({ scale, 0.0f, 0.0f }), 0xff0000_rgbf };
  _instances[1] = { Magnum::Matrix4::translation({ -scale, 0.0f, 0.0f }), 0x00ffff_rgbf };
  _instances[2] = { Magnum::Matrix4::translation({ 0.0f, scale, 0.0f }), 0x00ff00_rgbf };
  _instances[3] = { Magnum::Matrix4::translation({ 0.0f, -scale, 0.0f }), 0xff00ff_rgbf };
  _instances[4] = { Magnum::Matrix4::translation({ 0.0f, 0.0f, scale }), 0x0000ff_rgbf };
  _instances[5] = { Magnum::Matrix4::translation({ 0.0f, 0.0f, -scale }), 0xffff00_rgbf };

  _instance_buffer = Magnum::GL::Buffer{};

  _box = Magnum::MeshTools::compile(Magnum::Primitives::cubeSolid());

  _shader = Magnum::Shaders::Flat3D{ Magnum::Shaders::Flat3D::Flag::VertexColor |
                                     Magnum::Shaders::Flat3D::Flag::InstancedTransformation };

  _edl.init(Magnum::GL::defaultFramebuffer.viewport());
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
  using namespace Magnum::Math::Literals;

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

  if (_edl.enabled)
  {
    _edl.frame_buffer.clear(Magnum::GL::FramebufferClear::Color | Magnum::GL::FramebufferClear::Depth).bind();
  }
  else
  {
    Magnum::GL::defaultFramebuffer.clear(Magnum::GL::FramebufferClear::Color | Magnum::GL::FramebufferClear::Depth)
      .bind();
  }

  auto projection_matrix = camera::viewProjection(_camera, Magnum::Vector2(windowSize()));

  _shader
    // .setAmbientColor(0x7f7f7f_rgbf)                //
    .setTransformationProjectionMatrix(projection_matrix);

  _instance_buffer.setData(_instances, Magnum::GL::BufferUsage::DynamicDraw);
  _box.setInstanceCount(_instances.size())
    .addVertexBufferInstanced(_instance_buffer, 1, 0, Magnum::Shaders::Flat3D::TransformationMatrix{},
                              Magnum::Shaders::Flat3D::Color3{});
  _shader.draw(_box);

  if (_edl.enabled)
  {
    Magnum::GL::defaultFramebuffer.bind();
    Magnum::GL::defaultFramebuffer.clear(Magnum::GL::FramebufferClear::Color | Magnum::GL::FramebufferClear::Depth);
    _edl.blit(_camera.clip_near, _camera.clip_far);
  }

  swapBuffers();
  if (_continuous_sim)
  {
    redraw();
  }
}

void Viewer::viewportEvent(ViewportEvent &event)
{
  _edl.resize(event.windowSize());
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

  if (event.key() == KeyEvent::Key::Tab)
  {
    _edl.enabled = !_edl.enabled;
    dirty = true;
    Magnum::Debug() << "EDL: " << (_edl.enabled ? "on" : "off");
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
