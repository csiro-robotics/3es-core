//
// Author: Kazys Stepanas
//
#ifndef TES_VIEWER_SHADERS_SHADER_CACHE_H
#define TES_VIEWER_SHADERS_SHADER_CACHE_H

#include "3es-viewer.h"

#include <string>

namespace tes::viewer::shaders
{
/// Stores the common shaders used by the 3rd Eye Scene viewer.
class TES_VIEWER_API ShaderCache
{
  /// IDs of known 3es shaders.
  enum class ID
  {
    /// Flat shader with no vertex colour. Suitable for primitive rendering; supports instancing.
    Flat,
    /// Mesh shader with vertex colour support.
    VertexColour,
    /// Point cloud shader using hardware points.
    PointCloudPoints,
    /// Point cloud shader using geometry shaders.
    PointCloudGeometry,
    /// Voxel geometry based shader.
    VoxelGeometry,
  };

  /// Get the shader name of a known shader.
  /// @param id
  /// @return
  static std::string shaderName(ID id);
};
}  // namespace tes::viewer::shaders

#endif  // TES_VIEWER_SHADERS_SHADER_CACHE_H
