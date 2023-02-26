#include "ThirdEyeScene.h"

#include "EdlEffect.h"

#include "handler/Camera.h"
#include "handler/Category.h"
#include "handler/MeshResource.h"
#include "handler/MeshSet.h"
#include "handler/MeshShape.h"
#include "handler/Message.h"
#include "handler/Shape.h"
#include "handler/Text2D.h"
#include "handler/Text3D.h"

#include "painter/Arrow.h"
#include "painter/Box.h"
#include "painter/Capsule.h"
#include "painter/Cone.h"
#include "painter/Cylinder.h"
#include "painter/Plane.h"
#include "painter/Pose.h"
#include "painter/ShapePainter.h"
#include "painter/Sphere.h"
#include "painter/Star.h"
#include "painter/Text.h"

#include "settings/Loader.h"

#include "shaders/Flat.h"
#include "shaders/ShaderLibrary.h"
#include "shaders/PointGeom.h"
#include "shaders/VertexColour.h"
#include "shaders/VoxelGeom.h"

#include <3escore/Log.h>

#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Text/DistanceFieldGlyphCache.h>

#include <iterator>

// Things to learn about:
// - UI

// Things to implement:
// - point cloud message handler

namespace tes::view
{
ThirdEyeScene::ThirdEyeScene()
  : _main_thread_id(std::this_thread::get_id())
{
  using namespace Magnum::Math::Literals;

  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::DepthTest);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::FaceCulling);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::Blending);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::ProgramPointSize);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::ProgramPointSize);
  Magnum::GL::Renderer::setPointSize(8);

  restoreSettings();

  _culler = std::make_shared<BoundsCuller>();
  // Initialise the font.
  initialiseFont();
  initialiseShaders();
  initialiseHandlers();

  const auto config = _settings.config();
  onCameraConfigChange(config);

  _settings.addObserver(
    settings::Settings::Category::Camera,
    [this](const settings::Settings::Config &config) { onCameraConfigChange(config); });
}


ThirdEyeScene::~ThirdEyeScene()
{
  storeSettings();
  // Need an ordered cleanup.
  _message_handlers.clear();
  _ordered_message_handlers.clear();
  _main_draw_handlers.clear();
  _secondary_draw_handlers.clear();
  _painters.clear();
  _text_painter = nullptr;
}


const std::unordered_map<uint32_t, std::string> ThirdEyeScene::defaultHandlerNames()
{
  static const std::unordered_map<uint32_t, std::string> mappings = {
    { MtNull, "null" },
    { MtServerInfo, "server info" },
    { MtControl, "control" },
    { MtCollatedPacket, "collated packet" },
    { MtMesh, "mesh" },
    { MtCamera, "camera" },
    { MtCategory, "category" },
    { MtMaterial, "material" },
    { SIdSphere, "sphere" },
    { SIdBox, "box" },
    { SIdCone, "cone" },
    { SIdCylinder, "cylinder" },
    { SIdCapsule, "capsule" },
    { SIdPlane, "plane" },
    { SIdStar, "star" },
    { SIdArrow, "arrow" },
    { SIdMeshShape, "mesh shape" },
    { SIdMeshSet, "mesh set" },
    { SIdPointCloudDeprecated, "point cloud (deprecated)" },
    { SIdText3D, "text 3D" },
    { SIdText2D, "text 2D" },
    { SIdPose, "pose" },
  };
  return mappings;
}


void ThirdEyeScene::setActiveFboEffect(std::shared_ptr<FboEffect> effect)
{
  std::swap(_active_fbo_effect, effect);
}


void ThirdEyeScene::clearActiveFboEffect()
{
  _active_fbo_effect = nullptr;
}


void ThirdEyeScene::reset()
{
  std::unique_lock lock(_render_mutex);
  if (std::this_thread::get_id() == _main_thread_id)
  {
    effectReset();
  }
  else
  {
    _reset = true;
    _reset_notify.wait(lock, [target_reset = _reset_marker + 1, this]()  //
                       { return _reset_marker >= target_reset; });
  }
  if (_reset_callback)
  {
    _reset_callback();
  }
}


void ThirdEyeScene::render(float dt, const Magnum::Vector2i &window_size)
{
  //---------------------------------------------------------------------------
  // This section is protected by the _render_mutex
  // It must ensure that there can be no additional handler::Message::endFrame() calls in between
  // calling handler::Message::prepareFrame() and handler::Message::draw()
  // After the draw calls we release the mutex while finalising the rendering.
  std::unique_lock guard(_render_mutex);
  if (_reset)
  {
    effectReset();
  }

  // Update frame if needed.
  if (_have_new_frame || _new_server_info)
  {
    // Update server info.
    if (_new_server_info)
    {
      for (auto &handler : _ordered_message_handlers)
      {
        handler->updateServerInfo(_server_info);
      }
      _new_server_info = false;
    }

    _render_stamp.frame_number = _new_frame;
    _have_new_frame = false;

    for (auto &handler : _ordered_message_handlers)
    {
      handler->prepareFrame(_render_stamp);
    }
  }

  const DrawParams params(_camera, window_size);
  ++_render_stamp.render_mark;

  _culler->cull(_render_stamp.render_mark, Magnum::Frustum::fromMatrix(params.pv_transform));

  if (_active_fbo_effect)
  {
    _active_fbo_effect->prepareFrame(params.pv_transform, FboEffect::ProjectionType::Perspective,
                                     _camera.clip_near, _camera.clip_far);
  }
  else
  {
    Magnum::GL::defaultFramebuffer
      .clear(Magnum::GL::FramebufferClear::Color | Magnum::GL::FramebufferClear::Depth)
      .bind();
  }

  drawPrimary(dt, params);
  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  // This section is not protected by the _render_mutex
  if (_active_fbo_effect)
  {
    Magnum::GL::defaultFramebuffer
      .clear(Magnum::GL::FramebufferClear::Color | Magnum::GL::FramebufferClear::Depth)
      .bind();
    _active_fbo_effect->completeFrame();
  }

  updateFpsDisplay(dt, params);
  drawSecondary(dt, params);
  //---------------------------------------------------------------------------
}


void ThirdEyeScene::updateToFrame(FrameNumber frame)
{
  // Called from the data thread, not the main thread.
  // Must invoke endFrame() between prepareFrame() and draw() calls.
  std::lock_guard guard(_render_mutex);
  if (frame != _render_stamp.frame_number)
  {
    for (auto &handler : _ordered_message_handlers)
    {
      handler->endFrame(_render_stamp);
    }
  }
  _new_frame = frame;
  _have_new_frame = true;
}


void ThirdEyeScene::updateServerInfo(const ServerInfoMessage &server_info)
{
  std::lock_guard guard(_render_mutex);
  _server_info = server_info;
  _new_server_info = true;
}


void ThirdEyeScene::processMessage(PacketReader &packet)
{
  auto handler = _message_handlers.find(packet.routingId());
  if (handler != _message_handlers.end())
  {
    handler->second->readMessage(packet);
  }
  else if (_unknown_handlers.find(packet.routingId()) == _unknown_handlers.end())
  {
    const auto known_ids = defaultHandlerNames();
    const auto search = known_ids.find(packet.routingId());
    if (search == known_ids.end())
    {
      log::error("No message handler for id ", packet.routingId());
    }
    else
    {
      log::error("No message handler for ", search->second);
    }
    _unknown_handlers.emplace(packet.routingId());
  }
}


void ThirdEyeScene::createSampleShapes()
{
  Magnum::Matrix4 shape_transform = {};

  // Axis box markers
  _painters[SIdBox]->add(Id(2), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ 10, 0, 0 }) * shape_transform,
                         Magnum::Color4{ 1, 0, 0 });
  _painters[SIdBox]->add(Id(3), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ 0, 10, 0 }) * shape_transform,
                         Magnum::Color4{ 0, 1, 0 });
  _painters[SIdBox]->add(Id(4), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ 0, 0, 10 }) * shape_transform,
                         Magnum::Color4{ 0, 0, 1 });
  _painters[SIdBox]->add(Id(5), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ -10, 0, 0 }) * shape_transform,
                         Magnum::Color4{ 0, 1, 1 });
  _painters[SIdBox]->add(Id(6), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ 0, -10, 0 }) * shape_transform,
                         Magnum::Color4{ 1, 0, 1 });
  _painters[SIdBox]->add(Id(7), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ 0, 0, -10 }) * shape_transform,
                         Magnum::Color4{ 1, 1, 0 });

  // Add debug shapes.
  float x = 0;
  shape_transform = {};
  _painters[SIdSphere]->add(Id(1), painter::ShapePainter::Type::Solid,
                            Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform,
                            Magnum::Color4{ 1, 1, 0 });
  _painters[SIdSphere]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                            Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform,
                            Magnum::Color4{ 0, 1, 1 });
  _painters[SIdSphere]->add(Id(1), painter::ShapePainter::Type::Transparent,
                            Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                            Magnum::Color4{ 1, 0, 1, 0.4f });

  x = -2.5f;
  shape_transform = {};
  _painters[SIdBox]->add(Id(1), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform,
                         Magnum::Color4{ 1, 0, 0 });
  _painters[SIdBox]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                         Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform,
                         Magnum::Color4{ 0, 1, 1 });
  _painters[SIdBox]->add(Id(1), painter::ShapePainter::Type::Transparent,
                         Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                         Magnum::Color4{ 1, 0, 1, 0.4f });

  x = 2.5f;
  shape_transform =
    Magnum::Matrix4::rotationX(Magnum::Deg(35.0f)) * Magnum::Matrix4::scaling({ 0.3f, 0.3f, 1.0f });
  _painters[SIdCylinder]->add(Id(1), painter::ShapePainter::Type::Solid,
                              Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform,
                              Magnum::Color4{ 1, 1, 0 });
  _painters[SIdCylinder]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                              Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform,
                              Magnum::Color4{ 0, 1, 1 });
  _painters[SIdCylinder]->add(Id(1), painter::ShapePainter::Type::Transparent,
                              Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                              Magnum::Color4{ 1, 0, 1, 0.4f });

  x = -5.0f;
  shape_transform =
    Magnum::Matrix4::rotationX(Magnum::Deg(35.0f)) * Magnum::Matrix4::scaling({ 0.3f, 0.3f, 1.0f });
  _painters[SIdCapsule]->add(Id(1), painter::ShapePainter::Type::Solid,
                             Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform,
                             Magnum::Color4{ 1, 1, 0 });
  _painters[SIdCapsule]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                             Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform,
                             Magnum::Color4{ 0, 1, 1 });
  _painters[SIdCapsule]->add(Id(1), painter::ShapePainter::Type::Transparent,
                             Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                             Magnum::Color4{ 1, 0, 1, 0.4f });

  x = 7.5f;
  shape_transform =
    Magnum::Matrix4::rotationX(Magnum::Deg(35.0f)) * Magnum::Matrix4::scaling({ 1.0f, 1.0f, 1.0f });
  _painters[SIdPlane]->add(Id(1), painter::ShapePainter::Type::Solid,
                           Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform,
                           Magnum::Color4{ 1, 1, 0 });
  _painters[SIdPlane]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                           Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform,
                           Magnum::Color4{ 0, 1, 1 });
  _painters[SIdPlane]->add(Id(1), painter::ShapePainter::Type::Transparent,
                           Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                           Magnum::Color4{ 1, 0, 1, 0.4f });

  x = -7.5f;
  shape_transform = Magnum::Matrix4::scaling({ 1.0f, 1.0f, 1.0f });
  _painters[SIdStar]->add(Id(1), painter::ShapePainter::Type::Solid,
                          Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform,
                          Magnum::Color4{ 1, 1, 0 });
  _painters[SIdStar]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                          Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform,
                          Magnum::Color4{ 0, 1, 1 });
  _painters[SIdStar]->add(Id(1), painter::ShapePainter::Type::Transparent,
                          Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                          Magnum::Color4{ 1, 0, 1, 0.4f });

  x = 10.0f;
  shape_transform =
    Magnum::Matrix4::rotationX(Magnum::Deg(35.0f)) * Magnum::Matrix4::scaling({ 0.1f, 0.1f, 1.0f });
  _painters[SIdArrow]->add(Id(1), painter::ShapePainter::Type::Solid,
                           Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform,
                           Magnum::Color4{ 1, 1, 0 });
  _painters[SIdArrow]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                           Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform,
                           Magnum::Color4{ 0, 1, 1 });
  _painters[SIdArrow]->add(Id(1), painter::ShapePainter::Type::Transparent,
                           Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                           Magnum::Color4{ 1, 0, 1, 0.4f });

  x = -10.0f;
  shape_transform =
    Magnum::Matrix4::rotationX(Magnum::Deg(35.0f)) * Magnum::Matrix4::scaling({ 1.0f, 1.0f, 1.0f });
  _painters[SIdPose]->add(Id(1), painter::ShapePainter::Type::Solid,
                          Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform,
                          Magnum::Color4{ 1, 1, 1 });
  _painters[SIdPose]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                          Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform,
                          Magnum::Color4{ 1, 1, 1 });
  _painters[SIdPose]->add(Id(1), painter::ShapePainter::Type::Transparent,
                          Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                          Magnum::Color4{ 1, 0, 1, 0.4f });

  for (auto &painter : _painters)
  {
    painter.second->commit();
  }
}


void ThirdEyeScene::effectReset()
{
  // _render_mutex must be locked before calling.
  for (auto &handler : _ordered_message_handlers)
  {
    handler->reset();
  }
  _unknown_handlers.clear();
  ++_reset_marker;
  _reset = false;
  // Slight inefficiency as we notify while the mutex is still locked.
  _reset_notify.notify_all();
}


void ThirdEyeScene::initialiseFont()
{
  // TODO(KS): get resources strings passed in as it's the exe which must include the resources.
  _text_painter = std::make_shared<painter::Text>(_font_manager);
}


void ThirdEyeScene::initialiseHandlers()
{
  _painters.emplace(SIdSphere, std::make_shared<painter::Sphere>(_culler, _shader_library));
  _painters.emplace(SIdBox, std::make_shared<painter::Box>(_culler, _shader_library));
  _painters.emplace(SIdCone, std::make_shared<painter::Cone>(_culler, _shader_library));
  _painters.emplace(SIdCylinder, std::make_shared<painter::Cylinder>(_culler, _shader_library));
  _painters.emplace(SIdCapsule, std::make_shared<painter::Capsule>(_culler, _shader_library));
  _painters.emplace(SIdPlane, std::make_shared<painter::Plane>(_culler, _shader_library));
  _painters.emplace(SIdStar, std::make_shared<painter::Star>(_culler, _shader_library));
  _painters.emplace(SIdArrow, std::make_shared<painter::Arrow>(_culler, _shader_library));
  _painters.emplace(SIdPose, std::make_shared<painter::Pose>(_culler, _shader_library));

  _ordered_message_handlers.emplace_back(std::make_shared<handler::Category>());
  _camera_handler = std::make_shared<handler::Camera>();
  _ordered_message_handlers.emplace_back(_camera_handler);

  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::Shape>(SIdSphere, "sphere", _painters[SIdSphere]));
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::Shape>(SIdBox, "box", _painters[SIdBox]));
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::Shape>(SIdCone, "cone", _painters[SIdCone]));
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::Shape>(SIdCylinder, "cylinder", _painters[SIdCylinder]));
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::Shape>(SIdCapsule, "capsule", _painters[SIdCapsule]));
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::Shape>(SIdPlane, "plane", _painters[SIdPlane]));
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::Shape>(SIdStar, "star", _painters[SIdStar]));
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::Shape>(SIdArrow, "arrow", _painters[SIdArrow]));
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::Shape>(SIdPose, "pose", _painters[SIdPose]));

  auto mesh_resources = std::make_shared<handler::MeshResource>(_shader_library);
  _ordered_message_handlers.emplace_back(mesh_resources);
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::MeshShape>(_culler, _shader_library));
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::MeshSet>(_culler, mesh_resources));

  // Copy main draw handlers
  _main_draw_handlers = _ordered_message_handlers;

  // Add secondary draw handlers
  _secondary_draw_handlers.emplace_back(std::make_shared<handler::Text2D>(_text_painter));
  _secondary_draw_handlers.emplace_back(std::make_shared<handler::Text3D>(_text_painter));

  std::copy(_secondary_draw_handlers.begin(), _secondary_draw_handlers.end(),
            std::back_inserter(_ordered_message_handlers));

  // TODO:
  // - point cloud
  // - multi-shape

  // Copy message handlers to the routing set and initialise.
  for (auto &handler : _ordered_message_handlers)
  {
    handler->initialise();
    _message_handlers.emplace(handler->routingId(), handler);
  }
}


void ThirdEyeScene::initialiseShaders()
{
  _shader_library = std::make_shared<shaders::ShaderLibrary>();
  _shader_library->registerShader(shaders::ShaderLibrary::ID::Flat,
                                  std::make_shared<shaders::Flat>());
  auto vertex_colour_shader = std::make_shared<shaders::VertexColour>();
  _shader_library->registerShader(shaders::ShaderLibrary::ID::VertexColour, vertex_colour_shader);
  _shader_library->registerShader(shaders::ShaderLibrary::ID::Line, vertex_colour_shader);
  _shader_library->registerShader(shaders::ShaderLibrary::ID::PointCloud,
                                  std::make_shared<shaders::PointGeom>());
  _shader_library->registerShader(shaders::ShaderLibrary::ID::Voxel,
                                  std::make_shared<shaders::VoxelGeom>());
}


void ThirdEyeScene::drawPrimary(float dt, const DrawParams &params)
{
  draw(dt, params, _main_draw_handlers);
}


void ThirdEyeScene::drawSecondary(float dt, const DrawParams &params)
{
  draw(dt, params, _secondary_draw_handlers);
}


void ThirdEyeScene::draw(float dt, const DrawParams &params,
                         const std::vector<std::shared_ptr<handler::Message>> &drawers)
{
  (void)dt;
  // Draw opaque then transparent for proper blending.
  for (const auto &handler : drawers)
  {
    handler->draw(handler::Message::DrawPass::Opaque, _render_stamp, params);
  }
  for (const auto &handler : drawers)
  {
    handler->draw(handler::Message::DrawPass::Transparent, _render_stamp, params);
  }
  for (const auto &handler : drawers)
  {
    handler->draw(handler::Message::DrawPass::Overlay, _render_stamp, params);
  }
}


void ThirdEyeScene::updateFpsDisplay(float dt, const DrawParams &params)
{
  // Update stats.
  _fps.push(dt);
  // Calculate FPS.
  const auto fps = _fps.fps();
  // Render
  // FIXME(KS): the transform should be adjusted to consider screen resolution and text size.
  painter::Text::TextEntry fps_text = {};
  fps_text.transform = Magnum::Matrix4::translation(Magnum::Vector3(0.01f, 0.015f, 0.0f));
  fps_text.text = std::to_string(fps);
  _text_painter->draw2D(fps_text, params);
}


void ThirdEyeScene::onCameraConfigChange(const settings::Settings::Config &config)
{
  _camera.clip_far = config.camera.far_clip.value();
  _camera.clip_near = config.camera.near_clip.value();
  _camera.fov_horizontal_deg = config.camera.fov.value();
}


void ThirdEyeScene::restoreSettings()
{
  settings::Settings::Config config = {};
  if (settings::load(config))
  {
    _settings.update(config);
  }
}


void ThirdEyeScene::storeSettings()
{
  const auto config = _settings.config();
  settings::save(config);
}
}  // namespace tes::view
