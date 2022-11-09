#ifndef TES_VIEWER_SHADERS_EDL_H
#define TES_VIEWER_SHADERS_EDL_H

#include "3es-viewer.h"

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/Shaders/Generic.h>

namespace tes::shaders
{
class Edl : public Magnum::GL::AbstractShaderProgram
{
public:
  using Position = Magnum::Shaders::Generic3D::Position;
  using TextureCoordinates = Magnum::GL::Attribute<1, Magnum::Vector2>;

  Edl();
  ~Edl();

  Edl &bindColourTexture(Magnum::GL::Texture2D &texture);

  Edl &bindDepthBuffer(Magnum::GL::Texture2D &texture);

  Edl &setProjectionMatrix(const Magnum::Matrix4 &matrix);

  /// Set the projection parameters based on the near/far clip plane distances.
  Edl &setClipParams(Magnum::Float near, Magnum::Float far, bool perspective = true, bool reverse_depth = false);

  /// Set the screen/view size in pixels.
  Edl &setScreenParams(const Magnum::Vector2i &view_size);

  inline Edl &setRadius(float radius)
  {
    setUniform(_radiusUniform, radius);
    return *this;
  }

  inline Edl &setLinearScale(float linearScale)
  {
    setUniform(_linearScaleUniform, linearScale);
    return *this;
  }

  inline Edl &setExponentialScale(float exponentialScale)
  {
    setUniform(_exponentialScaleUniform, exponentialScale);
    return *this;
  }

private:
  enum : Magnum::Int
  {
    ColourUnit = 0,
    DepthUnit = 1
  };

  Magnum::Int _projectionMatrixUniform = 0;
  Magnum::Int _projectionParamsUniform = 0;
  Magnum::Int _screenParamsUniform = 0;
  Magnum::Int _radiusUniform = 0;
  Magnum::Int _linearScaleUniform = 0;
  Magnum::Int _exponentialScaleUniform = 0;
};
}  // namespace tes::shaders

#endif  // TES_VIEWER_SHADERS_EDL_H
