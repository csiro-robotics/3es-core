#include "EdlEffect.h"

#include "shaders/Edl.h"

#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Matrix4.h>

namespace tes::view
{
struct TES_VIEWER_API EdlEffectDetail
{
  struct TES_VIEWER_API Settings
  {
    float radius = 1.0f;
    float linear_scale = 1.0f;
    float exponential_scale = 3.0f;
    Magnum::Vector3 light_direction{ 0, 0, 1 };  ///< Light direction in camera space.
    Magnum::Range2Di viewport{ Magnum::Vector2i{ 0 }, Magnum::Vector2i{ 1 } };
  };

  Magnum::GL::Texture2D colour_texture;
  Magnum::GL::Texture2D depth_texture;
  Magnum::GL::Framebuffer frame_buffer{ Magnum::NoCreate };
  shaders::Edl shader;
  Settings settings;
  Magnum::GL::Mesh mesh;
  float near_clip = 1.0f;   ///< Cached when preparing the frame.
  float far_clip = 100.0f;  ///< Cached when preparing the frame.
};

EdlEffect::EdlEffect(const Magnum::Range2Di &viewport)
  : _imp(std::make_unique<EdlEffectDetail>())
{
  makeBuffers(viewport);

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

  _imp->mesh.setCount(Magnum::Int(Magnum::Containers::arraySize(indices)))
    .addVertexBuffer(Magnum::GL::Buffer{ vertices }, 0, shaders::Edl::Position{},
                     shaders::Edl::TextureCoordinates{})
    .setIndexBuffer(Magnum::GL::Buffer{ indices }, 0, Magnum::GL::MeshIndexType::UnsignedInt);
}


EdlEffect::~EdlEffect() = default;


void EdlEffect::setRadius(float radius)
{
  _imp->settings.radius = radius;
}


float EdlEffect::radius() const
{
  return _imp->settings.radius;
}


void EdlEffect::setLinearScale(float linear_scale)
{
  _imp->settings.linear_scale = linear_scale;
}


float EdlEffect::linearScale() const
{
  return _imp->settings.linear_scale;
}


void EdlEffect::setExponentialScale(float exponential_scale)
{
  _imp->settings.exponential_scale = exponential_scale;
}


float EdlEffect::exponentialScale() const
{
  return _imp->settings.exponential_scale;
}


void EdlEffect::setLightDirection(const Magnum::Vector3 &light_direction)
{
  _imp->settings.light_direction = light_direction;
}


const Magnum::Vector3 &EdlEffect::lightDirection() const
{
  return _imp->settings.light_direction;
}


void EdlEffect::prepareFrame(const Magnum::Matrix4 &projection_matrix,
                             ProjectionType projection_type, float near_clip, float far_clip)
{
  (void)projection_matrix;
  (void)projection_type;
  _imp->frame_buffer
    .clear(Magnum::GL::FramebufferClear::Color | Magnum::GL::FramebufferClear::Depth)
    .bind();
  _imp->near_clip = near_clip;
  _imp->far_clip = far_clip;
}


void EdlEffect::completeFrame()
{
  Magnum::Matrix4 projection;  // Identity.
  _imp->shader.setProjectionMatrix(projection)
    .bindColourTexture(_imp->colour_texture)
    .bindDepthBuffer(_imp->depth_texture)
    .setClipParams(_imp->near_clip, _imp->far_clip)
    .setRadius(_imp->settings.radius)
    .setLinearScale(_imp->settings.linear_scale)
    .setExponentialScale(_imp->settings.exponential_scale)
    .setLightDirection(_imp->settings.light_direction);
  _imp->shader.draw(_imp->mesh);
}


void EdlEffect::viewportChange(const Magnum::Range2Di &viewport)
{
  if (viewport != _imp->settings.viewport)
  {
    makeBuffers(viewport);
  }
}


void EdlEffect::makeBuffers(const Magnum::Range2Di &viewport)
{
  Magnum::Vector2i size = viewport.size();
  size.x() = std::max(size.x(), 1);
  size.y() = std::max(size.y(), 1);

  _imp->colour_texture.setStorage(1, Magnum::GL::TextureFormat::RGBA8, size);
  _imp->depth_texture.setStorage(1, Magnum::GL::TextureFormat::DepthComponent32F, size);

  _imp->colour_texture.setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge);
  _imp->depth_texture.setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge);

  _imp->frame_buffer = Magnum::GL::Framebuffer(viewport);
  _imp->frame_buffer.attachTexture(Magnum::GL::Framebuffer::ColorAttachment{ 0 },
                                   _imp->colour_texture, 0);
  _imp->frame_buffer.attachTexture(Magnum::GL::Framebuffer::BufferAttachment::Depth,
                                   _imp->depth_texture, 0);

  _imp->shader.setScreenParams(size);
}
}  // namespace tes::view
