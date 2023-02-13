//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SHAPES_MUTABLE_MESH_H
#define TES_CORE_SHAPES_MUTABLE_MESH_H

#include "SimpleMesh.h"

#include <3escore/CoreConfig.h>

namespace tes
{
class Connection;
class SimpleMesh;
class Transform;
struct MutableMeshImp;

/// A @c SimpleMesh wrapper which manages sending updates on changing mesh context, thereby
/// supporting mutation.
///
/// The following details are not mutable:
/// - Draw type
/// - Component flags.
/// - id
///
/// Note the mutable mesh is not a resource. The manages a @c SimpleMesh which is a resource.
class MutableMesh
{
public:
  MutableMesh(uint32_t id, DrawType draw_type = DtTriangles,
              unsigned components = SimpleMesh::Vertex | SimpleMesh::Index);
  ~MutableMesh();

  /// Exposes the internal mesh data.
  [[nodiscard]] const SimpleMesh &meshResource() const;

  void setTransform(const Transform &transform);

  void setTint(uint32_t tint);

  void setVertexCount(const UIntArg &count);
  void setIndexCount(const UIntArg &count);

  [[nodiscard]] unsigned pendingVertexCount() const;
  [[nodiscard]] unsigned pendingIndexCount() const;

  // void eraseVertices(const UIntArg &index, const UIntArg &count);
  // void eraseIndices(const UIntArg &index, const UIntArg &count);

  inline bool setVertex(const UIntArg &at, const Vector3f &v)
  {
    return setVertices(at, &v, 1u) == 1u;
  }
  unsigned setVertices(const UIntArg &at, const Vector3f *v, const UIntArg &count);

  inline bool setIndex(const UIntArg &at, uint32_t i) { return setIndices(at, &i, 1u) == 1u; }
  unsigned setIndices(const UIntArg &at, const uint32_t *idx, const UIntArg &count);

  inline bool setNormal(const UIntArg &at, const Vector3f &n)
  {
    return setNormals(at, &n, 1u) == 1u;
  }
  unsigned setNormals(const UIntArg &at, const Vector3f *n, const UIntArg &count);

  inline bool setColour(const UIntArg &at, uint32_t c) { return setColours(at, &c, 1u) == 1u; }
  unsigned setColours(const UIntArg &at, const uint32_t *c, const UIntArg &count);

  inline bool setUv(const UIntArg &at, float u, float v)
  {
    const std::array<float, 2> uv = { u, v };
    return setUvs(at, uv.data(), 1u) == 1u;
  }
  unsigned setUvs(const UIntArg &at, const float *uvs, const UIntArg &count);

  /// Update changes to the internal mesh and send changes to @c con. @c con may be null to finalise
  /// the initial mesh definition.
  void update(Connection *con);

private:
  void migratePending();

  std::unique_ptr<MutableMeshImp> _imp;
};
}  // namespace tes

#endif  // TES_CORE_SHAPES_MUTABLE_MESH_H
