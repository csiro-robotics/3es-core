#include "3esthirdeyescene.h"

#include "3esedleffect.h"

#include "handler/3escamera.h"
#include "handler/3escategory.h"
#include "handler/3esmeshresource.h"
#include "handler/3esmeshset.h"
#include "handler/3esmeshshape.h"
#include "handler/3esmessage.h"
#include "handler/3esshape.h"
#include "handler/3estext2d.h"
#include "handler/3estext3d.h"

#include "painter/3esarrow.h"
#include "painter/3esbox.h"
#include "painter/3escapsule.h"
#include "painter/3escone.h"
#include "painter/3escylinder.h"
#include "painter/3esplane.h"
#include "painter/3espose.h"
#include "painter/3esshapepainter.h"
#include "painter/3essphere.h"
#include "painter/3esstar.h"
#include "painter/3estext.h"

#include "shaders/3esflat.h"
#include "shaders/3esshaderlibrary.h"
#include "shaders/3esvertexcolour.h"

#include <3eslog.h>

#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Text/DistanceFieldGlyphCache.h>

// Things to learn about:
// - UI

// Things to implement:
// - point cloud rendering
//  - with point shader
//  - voxel shader

namespace tes::viewer
{
ThirdEyeScene::ThirdEyeScene()
{
  using namespace Magnum::Math::Literals;

  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::DepthTest);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::FaceCulling);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::Blending);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::ProgramPointSize);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::ProgramPointSize);
  Magnum::GL::Renderer::setPointSize(8);

  _camera.position = { 0, -5, 0 };

  _culler = std::make_shared<BoundsCuller>();
  // Initialise the font.
  initialiseFont();
  initialiseShaders();
  initialiseHandlers();
}


ThirdEyeScene::~ThirdEyeScene()
{
  // Need an ordered cleanup.
  _messageHandlers.clear();
  _orderedMessageHandlers.clear();
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
    { SIdPointCloud, "point cloud" },
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
  std::unique_lock guard(_render_mutex);
  for (auto &handler : _orderedMessageHandlers)
  {
    handler->reset();
  }
  _unknown_handlers.clear();
}


void ThirdEyeScene::render(float dt, const Magnum::Vector2 &window_size)
{
  using namespace Magnum::Math::Literals;
  std::unique_lock guard(_render_mutex);

  // Update frame if needed.
  if (_have_new_frame || _new_server_info)
  {
    // Update server info.
    if (_new_server_info)
    {
      for (auto &handler : _orderedMessageHandlers)
      {
        handler->updateServerInfo(_server_info);
      }
      _new_server_info = false;
    }

    _render_stamp.frame_number = _new_frame;
    _have_new_frame = false;

    for (auto &handler : _orderedMessageHandlers)
    {
      handler->beginFrame(_render_stamp);
    }
  }

  const DrawParams params(_camera, window_size);
  ++_render_stamp.render_mark;
  _culler->cull(_render_stamp.render_mark, Magnum::Frustum::fromMatrix(params.pv_transform));

  if (_active_fbo_effect)
  {
    _active_fbo_effect->prepareFrame(params.pv_transform, FboEffect::ProjectionType::Perspective, _camera.clip_near,
                                     _camera.clip_far);
  }
  else
  {
    Magnum::GL::defaultFramebuffer.clear(Magnum::GL::FramebufferClear::Color | Magnum::GL::FramebufferClear::Depth)
      .bind();
  }

  drawShapes(dt, params);

  if (_active_fbo_effect)
  {
    Magnum::GL::defaultFramebuffer.bind();
    Magnum::GL::defaultFramebuffer.clear(Magnum::GL::FramebufferClear::Color | Magnum::GL::FramebufferClear::Depth);
    _active_fbo_effect->completeFrame();
  }
}


void ThirdEyeScene::updateToFrame(FrameNumber frame)
{
  std::lock_guard guard(_render_mutex);
  if (frame != _render_stamp.frame_number)
  {
    for (auto &handler : _orderedMessageHandlers)
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
  auto handler = _messageHandlers.find(packet.routingId());
  if (handler != _messageHandlers.end())
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

  _orderedMessageHandlers.emplace_back(std::make_shared<handler::Category>());
  _orderedMessageHandlers.emplace_back(std::make_shared<handler::Camera>());

  _orderedMessageHandlers.emplace_back(std::make_shared<handler::Shape>(SIdSphere, "sphere", _painters[SIdSphere]));
  _orderedMessageHandlers.emplace_back(std::make_shared<handler::Shape>(SIdBox, "box", _painters[SIdBox]));
  _orderedMessageHandlers.emplace_back(std::make_shared<handler::Shape>(SIdCone, "cone", _painters[SIdCone]));
  _orderedMessageHandlers.emplace_back(
    std::make_shared<handler::Shape>(SIdCylinder, "cylinder", _painters[SIdCylinder]));
  _orderedMessageHandlers.emplace_back(std::make_shared<handler::Shape>(SIdCapsule, "capsule", _painters[SIdCapsule]));
  _orderedMessageHandlers.emplace_back(std::make_shared<handler::Shape>(SIdPlane, "plane", _painters[SIdPlane]));
  _orderedMessageHandlers.emplace_back(std::make_shared<handler::Shape>(SIdStar, "star", _painters[SIdStar]));
  _orderedMessageHandlers.emplace_back(std::make_shared<handler::Shape>(SIdArrow, "arrow", _painters[SIdArrow]));
  _orderedMessageHandlers.emplace_back(std::make_shared<handler::Shape>(SIdPose, "pose", _painters[SIdPose]));

  auto mesh_resources = std::make_shared<handler::MeshResource>(_shader_library);
  _orderedMessageHandlers.emplace_back(mesh_resources);
  _orderedMessageHandlers.emplace_back(std::make_shared<handler::MeshShape>(_culler, _shader_library));
  _orderedMessageHandlers.emplace_back(std::make_shared<handler::MeshSet>(_culler, mesh_resources));

  _orderedMessageHandlers.emplace_back(std::make_shared<handler::Text2D>(_text_painter));
  _orderedMessageHandlers.emplace_back(std::make_shared<handler::Text3D>(_text_painter));

  // TODO:
  // - point cloud
  // - multi-shape

  // Copy message handlers to the routing set and initialise.
  for (auto &handler : _orderedMessageHandlers)
  {
    handler->initialise();
    _messageHandlers.emplace(handler->routingId(), handler);
  }
}


void ThirdEyeScene::initialiseShaders()
{
  _shader_library = std::make_shared<shaders::ShaderLibrary>();
  _shader_library->registerShader(shaders::ShaderLibrary::ID::Flat, std::make_shared<shaders::Flat>());
  auto vertex_colour_shader = std::make_shared<shaders::VertexColour>();
  _shader_library->registerShader(shaders::ShaderLibrary::ID::VertexColour, vertex_colour_shader);
  _shader_library->registerShader(shaders::ShaderLibrary::ID::Line, vertex_colour_shader);
  _shader_library->registerShader(shaders::ShaderLibrary::ID::PointCloud, vertex_colour_shader);
}


void ThirdEyeScene::drawShapes(float dt, const DrawParams &params)
{
  (void)dt;
  const auto camera_matrix = camera::matrix(_camera);
  // Draw opaque then transparent for proper blending.
  for (const auto &handler : _orderedMessageHandlers)
  {
    handler->draw(handler::Message::DrawPass::Opaque, _render_stamp, params);
  }
  for (const auto &handler : _orderedMessageHandlers)
  {
    handler->draw(handler::Message::DrawPass::Transparent, _render_stamp, params);
  }
  for (const auto &handler : _orderedMessageHandlers)
  {
    handler->draw(handler::Message::DrawPass::Overlay, _render_stamp, params);
  }
}
}  // namespace tes::viewer
