//
// Author: Kazys Stepanas
//
#ifndef TES_VIEWER_SHADERS_SHADER_LIBRARY_H
#define TES_VIEWER_SHADERS_SHADER_LIBRARY_H

#include "3es-viewer.h"

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

namespace tes::viewer::shaders
{
class Shader;

/// Stores the common shaders used by the 3rd Eye Scene viewer.
class TES_VIEWER_API ShaderLibrary
{
public:
  /// IDs of known 3es shaders.
  /// @todo Work out if the EDL shader belongs here or if it's just mesh rendering.
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
    /// Number of core sharers. Not to be used as an ID.
    Count
  };

  /// Get the shader name of a known shader.
  /// @param id
  /// @return
  static std::string shaderName(ID id);

  ShaderLibrary();
  ~ShaderLibrary();

  /// Lookup a shader by @c ID.
  /// @param id The shader @c ID to lookup
  /// @return The shader of the given name, or a nullptr on lookup failure.
  std::shared_ptr<Shader> lookup(ID id) const;

  /// Lookup a shader by @c ID and try cast to a derived shader type.
  /// @tparam T The type to cast to, changing the return value to a @c std::shared_ptr of type @c T.
  /// @param id The shader @c ID to lookup
  /// @return The shader of the given name, or a nullptr on lookup failure or dynamic cast failure.
  template <typename T>
  std::shared_ptr<T> lookup(ID id) const
  {
    return std::dynamic_pointer_cast<T>(lookup(id));
  };

  /// Lookup a shader by name.
  /// @param name The shader name to lookup
  /// @return The shader of the given name, or a nullptr on lookup failure.
  std::shared_ptr<Shader> lookup(const std::string &name) const;

  /// Lookup a shader by name and try cast to a derived shader type.
  /// @tparam T The type to cast to, changing the return value to a @c std::shared_ptr of type @c T.
  /// @param name The shader name to lookup
  /// @return The shader of the given name, or a nullptr on lookup failure or dynamic cast failure.
  template <typename T>
  std::shared_ptr<T> lookup(const std::string &name) const
  {
    return std::dynamic_pointer_cast<T>(lookup(name));
  };

  /// Register a shader by known @c ID. This replaces any existing shader with that @c ID.
  ///
  /// @param id The shader @c ID.
  /// @param shader The shader to register.
  void registerShader(ID id, std::shared_ptr<Shader> shader);

  /// Register a shader by name. This replaces any existing shader of that name.
  ///
  /// > Note: use the overload for a shader with a known @c ID.
  ///
  /// @param name The shader name.
  /// @param shader The shader to register.
  void registerShader(const std::string &name, std::shared_ptr<Shader> shader);

private:
  /// Shaders by name.
  std::unordered_map<std::string, std::shared_ptr<Shader>> _shaders;
  /// Shaders by @c ID.
  std::vector<std::shared_ptr<Shader>> _core_shaders;
};
}  // namespace tes::viewer::shaders

#endif  // TES_VIEWER_SHADERS_SHADER_LIBRARY_H
