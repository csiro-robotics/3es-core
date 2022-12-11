//
// Author: Kazys Stepanas
//
#ifndef TES_VIEWER_SHADERS_FLAT_H
#define TES_VIEWER_SHADERS_FLAT_H

#include "3es-viewer.h"

#include "3esshader.h"

#include <Magnum/Shaders/Flat.h>

namespace tes::viewer::shaders
{
/// Flat colour shader for a @p ShapeCache . Can be used for solid, transparent and line based shapes.
class TES_VIEWER_API Flat : public Shader
{
public:
  /// Constructor.
  Flat();
  /// Destructor.
  ~Flat();

  std::shared_ptr<Magnum::GL::AbstractShaderProgram> shader() const override { return _shader; }
  std::shared_ptr<Magnum::Shaders::Flat3D> flatShader() const { return _shader; }

  Shader &setProjectionMatrix(const Magnum::Matrix4 &projection) override;

  Shader &setColour(const Magnum::Color4 &colour) override;

  Shader &draw(Magnum::GL::Mesh &mesh) override;
  Shader &draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer, size_t instance_count) override;

private:
  /// Internal shader.
  std::shared_ptr<Magnum::Shaders::Flat3D> _shader;
};

}  // namespace tes::viewer::shaders

#endif  // TES_VIEWER_SHADERS_FLAT_H
