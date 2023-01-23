//
// author: Kazys Stepanas
//
#ifndef _3ESMESHPLACEHOLDER_H_
#define _3ESMESHPLACEHOLDER_H_

#include <3escore/CoreConfig.h>

#include "3esmeshresource.h"

namespace tes
{
/// A placeholder for a mesh resource, carrying only a mesh ID. All other fields
/// and data manipulations are null and void.
///
/// This can be use to reference an existing mesh resource, primarily when using the
/// macro interface to release a mesh set such as with the @c tesmacros.
class _3es_coreAPI MeshPlaceholder : public MeshResource
{
public:
  /// Create a placeholder mesh resource for the given @p id.
  /// @param id The ID this placeholder publishes.
  MeshPlaceholder(uint32_t id);

  /// Changes the ID the placeholder publishes. Use with care.
  /// @param newId The new value for @c id().
  void setId(uint32_t newId);

  /// Returns the ID the placeholder was constructed with.
  uint32_t id() const override;

  /// @copydoc MeshResource::transform()
  Transform transform() const override;
  /// @copydoc MeshResource::tint()
  uint32_t tint() const override;
  /// @copydoc MeshResource::drawType()
  uint8_t drawType(int stream = 0) const override;
  /// @copydoc MeshResource::vertexCount()
  unsigned vertexCount(int stream = 0) const override;
  /// @copydoc MeshResource::indexCount()
  unsigned indexCount(int stream = 0) const override;
  /// @copydoc MeshResource::vertices()
  DataBuffer vertices(int stream = 0) const override;
  /// @copydoc MeshResource::indices()
  DataBuffer indices(int stream = 0) const override;
  /// @copydoc MeshResource::normals()
  DataBuffer normals(int stream = 0) const override;
  /// @copydoc MeshResource::uvs()
  DataBuffer uvs(int stream = 0) const override;
  /// @copydoc MeshResource::colours()
  DataBuffer colours(int stream = 0) const override;

  /// @copydoc Resource::clone()
  Resource *clone() const override;

private:
  uint32_t _id;
};
}  // namespace tes

#endif  // _3ESMESHPLACEHOLDER_H_
