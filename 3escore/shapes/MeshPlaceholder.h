//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SHAPES_MESH_PLACEHOLDER_H
#define TES_CORE_SHAPES_MESH_PLACEHOLDER_H

#include <3escore/CoreConfig.h>

#include "MeshResource.h"

namespace tes
{
/// A placeholder for a mesh resource, carrying only a mesh ID. All other fields
/// and data manipulations are null and void.
///
/// This can be use to reference an existing mesh resource, primarily when using the
/// macro interface to release a mesh set such as with the @c tesmacros.
class TES_CORE_API MeshPlaceholder : public MeshResource
{
public:
  /// Create a placeholder mesh resource for the given @p id.
  /// @param id The ID this placeholder publishes.
  MeshPlaceholder(uint32_t id);

  /// Changes the ID the placeholder publishes. Use with care.
  /// @param new_id The new value for @c id().
  void setId(uint32_t new_id);

  /// Returns the ID the placeholder was constructed with.
  [[nodiscard]] uint32_t id() const override;

  [[nodiscard]] Transform transform() const override;
  [[nodiscard]] uint32_t tint() const override;
  [[nodiscard]] uint8_t drawType(int stream) const override;
  using MeshResource::drawType;
  [[nodiscard]] float drawScale(int stream) const override;
  using MeshResource::drawScale;
  [[nodiscard]] unsigned vertexCount(int stream) const override;
  using MeshResource::vertexCount;
  [[nodiscard]] unsigned indexCount(int stream) const override;
  using MeshResource::indexCount;
  [[nodiscard]] DataBuffer vertices(int stream) const override;
  using MeshResource::vertices;
  [[nodiscard]] DataBuffer indices(int stream) const override;
  using MeshResource::indices;
  [[nodiscard]] DataBuffer normals(int stream) const override;
  using MeshResource::normals;
  [[nodiscard]] DataBuffer uvs(int stream) const override;
  using MeshResource::uvs;
  [[nodiscard]] DataBuffer colours(int stream) const override;
  using MeshResource::colours;

  /// @copydoc Resource::clone()
  [[nodiscard]] std::shared_ptr<Resource> clone() const override;

private:
  uint32_t _id;
};
}  // namespace tes

#endif  // TES_CORE_SHAPES_MESH_PLACEHOLDER_H
