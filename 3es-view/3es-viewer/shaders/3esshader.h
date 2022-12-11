//
// Author: Kazys Stepanas
//
#ifndef TES_VIEWER_SHADERS_SHADER_H
#define TES_VIEWER_SHADERS_SHADER_H

#include "3es-viewer.h"

#include <Magnum/Magnum.h>

#include <memory>

namespace Magnum::GL
{
class AbstractShaderProgram;
class Buffer;
class Mesh;
}  // namespace Magnum::GL

namespace tes::viewer::shaders
{
/// A shader abstraction which provides a common interfaces for magnum shaders.
///
/// This abstracts away the details of the shader to a common interface since Magnum shaders often use different
/// function names for similar operations.
class TES_VIEWER_API Shader
{
public:
  /// Virtual destructor.
  virtual ~Shader();

  /// Access the underlying shader.
  /// @return
  virtual std::shared_ptr<Magnum::GL::AbstractShaderProgram> shader() const = 0;

  /// Set the projection matrix for the next @c draw() call.
  /// @param projection The next projection matrix to draw with.
  virtual Shader &setProjectionMatrix(const Magnum::Matrix4 &projection) = 0;

  /// Set a colour tint to modulate the instance colour with.
  /// @param colour The tint colour.
  virtual Shader &setColour(const Magnum::Color4 &colour) = 0;

  /// Draw the @p mesh with this shader.
  /// @param mesh The mesh to draw.
  virtual Shader &draw(Magnum::GL::Mesh &mesh) = 0;

  /// Draw the @p mesh with this shader with shape instances from @p buffer .
  ///
  /// May be called multiple times for each frame with only one call to @c setProjectionMatrix() in between.
  ///
  /// The underlying shader must support instanced rendering.
  ///
  /// @param mesh The mesh to draw.
  /// @param buffer The shape instance buffer or @c Magnum::Matrix4 and @c Magnum::Color4 pairs per instance.
  /// @param instance_count Number of instances in @p buffer .
  virtual Shader &draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer, size_t instance_count) = 0;
};
}  // namespace tes::viewer::shaders

#endif  // TES_VIEWER_SHADERS_SHADER_H
