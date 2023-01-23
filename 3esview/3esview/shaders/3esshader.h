//
// Author: Kazys Stepanas
//
#ifndef TES_VIEWER_SHADERS_SHADER_H
#define TES_VIEWER_SHADERS_SHADER_H

#include <3esview/ViewConfig.h>

#include "util/3esenum.h"

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
  /// Flags which identify supported shader features.
  enum class Feature : unsigned
  {
    /// No special features.
    None = 0u,
    /// Instance rendering feature.
    Instance = (1u << 0u),
    /// Transparent rendering feature.
    Transparent = (1u << 1u),
    /// Colour tint feature: @c setColour().
    Tint = (1u << 2u),
    /// Supports draw scale: @c setDrawScale().
    DrawScale = (1u << 3u),
  };

  /// The default point rendering size.
  static constexpr float kDefaultPointSize = 8.0f;
  /// The default line rendering width.
  static constexpr float kDefaultLineWidth = 2.0f;

  /// Virtual destructor.
  virtual ~Shader();

  /// Get the supported feature flags for this shader.
  /// @return
  virtual Feature features() const = 0;

  /// Check the supported features.
  ///
  /// Can be used to check a single feature, or a feature set, in which case all features must be supported.
  ///
  /// @param feature_flags The feature flag or set of feature flags to check for.
  /// @return True if all the given @p feature_flags are supported.
  bool supportsFeatures(Feature feature_flags) const;

  /// Check if any of the specified feature (flags) are supported.
  /// @param feature_flags The set of feature flags to check for
  /// @return True if any one of the specified features are supported.
  bool supportsFeatureAny(Feature feature_flags) const;

  /// Access the underlying shader.
  /// @return A pointer to the Magnum Graphics shader.
  virtual std::shared_ptr<Magnum::GL::AbstractShaderProgram> shader() const = 0;

  /// Set the projection matrix for the next @c draw() call. This is only the projection matrix and must be combined
  /// with the view and model matrices either in the shader or for the shader.
  /// @param matrix The next projection matrix to draw with.
  /// @return @c *this
  virtual Shader &setProjectionMatrix(const Magnum::Matrix4 &matrix) = 0;

  /// Set the view matrix for the next @c draw() call. This is only the view matrix and must be combined
  /// with the projection and model matrices either in the shader or for the shader.
  /// @param matrix The next view matrix to draw with.
  /// @return @c *this
  virtual Shader &setViewMatrix(const Magnum::Matrix4 &matrix) = 0;
  /// Set the model matrix for the next @c draw() call. This is only the model matrix and must be combined
  /// with the projection and view matrices either in the shader or for the shader.
  /// @param matrix The next model matrix to draw with.
  /// @return @c *this
  virtual Shader &setModelMatrix(const Magnum::Matrix4 &matrix) = 0;

  /// Set the near and far clip plane distances for the current view.
  /// @param near The near clip plane distance.
  /// @param far The far clip plane distance.
  /// @return @c *this
  virtual Shader &setClipPlanes(float near, float far)
  {
    (void)near;
    (void)far;
    return *this;
  }

  /// Set the viewport size (pixels).
  /// @param size The viewport size (pixels); x=>width, y=>height.
  /// @return @c *this
  virtual Shader &setViewportSize(const Magnum::Vector2i &size)
  {
    (void)size;
    return *this;
  }

  /// Set a colour tint to modulate the instance colour with.
  /// @param colour The tint colour.
  /// @return @c *this
  virtual Shader &setColour(const Magnum::Color4 &colour) = 0;

  /// Sets the draw scale for things which support it. This depends on the shader, but is used for point rendering size,
  /// line width, font size, etc.
  ///
  /// Setting a zero draw scale implies using the "default" draw scale. Negative values yield undefined behaviour.
  ///
  /// @param scale The draw scale to set.
  /// @return @c *this
  virtual Shader &setDrawScale(float scale) = 0;

  /// Draw the @p mesh with this shader.
  /// @param mesh The mesh to draw.
  /// @return @c *this
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
  /// @return @c *this
  virtual Shader &draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer, size_t instance_count) = 0;
};

TES_ENUM_FLAGS(Shader::Feature, unsigned);


inline bool Shader::supportsFeatures(Feature feature_flags) const
{
  return (features() & feature_flags) == feature_flags;
}


inline bool Shader::supportsFeatureAny(Feature feature_flags) const
{
  return (features() & feature_flags) != Feature::None;
}
}  // namespace tes::viewer::shaders

#endif  // TES_VIEWER_SHADERS_SHADER_H
