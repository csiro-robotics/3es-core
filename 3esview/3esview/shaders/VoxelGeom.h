//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SHADERS_VOXEL_GEOM_H
#define TES_VIEW_SHADERS_VOXEL_GEOM_H

#include <3esview/ViewConfig.h>

#include "Shader.h"
#include "Pvm.h"

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/Shaders/Generic.h>

#include <memory>

namespace tes::view::shaders
{
class VoxelGeomProgram;

/// A point shader using geometry to make circular points.
class TES_VIEWER_API VoxelGeom : public Shader
{
public:
  /// Constructor.
  VoxelGeom();
  /// Destructor.
  ~VoxelGeom();

  Feature features() const override { return Feature::Transparent | Feature::Tint | Feature::DrawScale; }

  std::shared_ptr<Magnum::GL::AbstractShaderProgram> shader() const override;
  std::shared_ptr<VoxelGeomProgram> typedShader() const { return _shader; }

  Shader &setProjectionMatrix(const Magnum::Matrix4 &matrix) override;
  Shader &setViewMatrix(const Magnum::Matrix4 &matrix) override;
  Shader &setModelMatrix(const Magnum::Matrix4 &matrix) override;

  Shader &setColour(const Magnum::Color4 &colour) override;

  Shader &setDrawScale(float scale) override;

  Shader &draw(Magnum::GL::Mesh &mesh) override;
  Shader &draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer, size_t instance_count) override;

private:
  void updateTransform();

  /// Internal shader.
  std::shared_ptr<VoxelGeomProgram> _shader;
  Pvm _pvm;
};

/// The underlying Magnum shader implementation for point rendering.
class TES_VIEWER_API VoxelGeomProgram : public Magnum::GL::AbstractShaderProgram
{
public:
  using Generic = Magnum::Shaders::Generic<3>;

  using Position = Generic::Position;
  using Color3 = Generic::Color3;
  using Color4 = Generic::Color4;
  using Int = Magnum::Int;

  enum : Magnum::UnsignedInt
  {
    /**
     * Color shader output. @ref shaders-generic "Generic output",
     * present always. Expects three- or four-component floating-point
     * or normalized buffer attachment.
     */
    ColorOutput = Generic::ColorOutput
  };

  explicit VoxelGeomProgram();
  explicit VoxelGeomProgram(Magnum::NoCreateT) noexcept
    : AbstractShaderProgram(Magnum::NoCreate)
  {}

  VoxelGeomProgram(const VoxelGeomProgram &) = delete;
  VoxelGeomProgram(VoxelGeomProgram &&) noexcept = default;
  VoxelGeomProgram &operator=(const VoxelGeomProgram &) = delete;
  VoxelGeomProgram &operator=(VoxelGeomProgram &&) noexcept = default;

  /// Set the projection * view matrix.
  /// @param matrix
  /// @return
  VoxelGeomProgram &setProjectionViewTransform(const Magnum::Matrix4 &matrix);
  /// Set just the model matrix.
  /// @param matrix
  /// @return
  VoxelGeomProgram &setModelMatrix(const Magnum::Matrix4 &matrix);

  VoxelGeomProgram &setTint(const Magnum::Color4 &colour);

  /// Set the voxel rendering scale.
  /// @param scale
  /// @return
  VoxelGeomProgram &setVoxelScale(const Magnum::Vector3 &scale);

private:
  Int _model_matrix_uniform = 0;
  Int _tint_uniform = 1;
  Int _pv_matrix_uniform = 2;
  Int _scale_uniform = 3;
};
}  // namespace tes::view::shaders

#endif  // TES_VIEW_SHADERS_VOXEL_GEOM_H
