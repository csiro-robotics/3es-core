//
// Author: Kazys Stepanas
//
#ifndef TES_VIEWER_SHADERS_VERTEX_COLOUR_H
#define TES_VIEWER_SHADERS_VERTEX_COLOUR_H

#include "3es-viewer.h"

#include "3esshader.h"

#include <Magnum/Shaders/VertexColor.h>

#include <memory>

namespace tes::viewer::shaders
{
/// Vertex colour shader. Can be used for solid, transparent and line based shapes.
class TES_VIEWER_API VertexColour : public Shader
{
public:
  /// Constructor.
  VertexColour();
  /// Destructor.
  ~VertexColour();

  Feature features() const override { return Feature::Transparent; }

  std::shared_ptr<Magnum::GL::AbstractShaderProgram> shader() const override { return _shader; }
  std::shared_ptr<Magnum::Shaders::VertexColor3D> typedShader() const { return _shader; }

  Shader &setProjectionMatrix(const Magnum::Matrix4 &projection) override;

  Shader &setColour(const Magnum::Color4 &colour) override;

  Shader &draw(Magnum::GL::Mesh &mesh) override;
  Shader &draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer, size_t instance_count) override;

private:
  /// Internal shader.
  std::shared_ptr<Magnum::Shaders::VertexColor3D> _shader;
};

}  // namespace tes::viewer::shaders

#endif  // TES_VIEWER_SHADERS_VERTEX_COLOUR_H
