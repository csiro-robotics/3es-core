//
// author: Kazys Stepanas
//
#ifndef _3ESSIMPLEMESH_H_
#define _3ESSIMPLEMESH_H_

#include "3es-core.h"

#include "3escolour.h"
#include "3esintarg.h"
#include "3esmeshmessages.h"
#include "3esmeshresource.h"

namespace tes
{
struct SimpleMeshImp;

/// An encapsulated definition of a mesh. It manages all its own vertices, indices, etc.
class _3es_coreAPI SimpleMesh : public MeshResource
{
public:
  /// Flags indicating which components are present. @c Vertex flag is
  /// always set. Other flags are optional, though @c Index is preferred.
  enum ComponentFlag
  {
    Vertex = (1 << 0),  ///< Contains vertices. This flag is enforced.
    Index = (1 << 1),
    Colour = (1 << 2),
    Color = Colour,
    Normal = (1 << 3),
    Uv = (1 << 4)
  };

  /// Construct a @c SimpleMesh resource.
  /// @param id An ID unique among all @c tes::Resource objects.
  /// @param vertexCount Number of vertices to preallocate.
  /// @param indexCount Number of indices to preallocate.
  /// @param drawType Defines the primitive type being indexed.
  /// @param components The components defined by this mesh. See @c ComponentFlag.
  SimpleMesh(uint32_t id, size_t vertexCount = 0u, size_t indexCount = 0u,
             DrawType drawType = DtTriangles, unsigned components = Vertex | Index);

protected:
  /// Copy constructor supporting initial, shallow copy with copy on write semantics.
  /// @param other The mesh to copy.
  SimpleMesh(const SimpleMesh &other);

public:
  /// Destructor.
  ~SimpleMesh();

  /// Reset this mesh to a simple mesh with only @c Vertex and @c Index components.
  virtual void clear();

  /// Clear only the data arrays. Memory is preserved.s
  virtual void clearData();

  /// @copydoc Resource::id()
  virtual uint32_t id() const override;

  /// @copydoc Resource::transform()
  virtual Transform transform() const override;

  /// Set the object transformation matrix for this mesh.
  ///
  /// This will often be redundant when the mesh is used with a @c MeshSet object as that object defines its own
  /// object matrix and a transformation matrix for each contains @c MeshResource.
  ///
  /// @param transform The object transformation matrix for the mesh.
  void setTransform(const Transform &transform);

  /// @copydoc MeshResource::tint()
  virtual uint32_t tint() const override;
  /// Set the colour tint value for the mesh. The colour is defined in hex as 0xRRGGBBAA, best calculated using the
  /// @c Colour class.
  /// @param tint The RGBA tint colour.
  void setTint(uint32_t tint);

  /// Performs a shallow copy of this mesh. Note that any modification
  /// of the mesh data results in a copy of the existing data. Otherwise
  /// @c SimpleMesh objects can share their data.
  SimpleMesh *clone() const override;

  /// @copydoc::MeshResource::drawType()
  virtual uint8_t drawType(int stream) const override;

  /// Get the @c drawType() as a @c DrawType value.
  DrawType getDrawType() const;
  /// Set the draw type as a @c DrawType value.
  /// @param type The draw type to set.
  void setDrawType(DrawType type);

  /// Query the @c ComponentFlag components used by this mesh.
  /// @return The @c ComponentFlag values.
  unsigned components() const;

  /// Set the @c ComponentFlag components for this mesh.
  /// @param components @c ComponentFlag values to set.
  void setComponents(unsigned components);

  /// Add @c ComponentFlag values to the existing set.
  /// @param components Additional @c ComponentFlag values to set. Already set values are effectively ignored.
  void addComponents(unsigned components);

  unsigned vertexCount() const;
  virtual unsigned vertexCount(int stream) const override;
  void setVertexCount(size_t count);
  void reserveVertexCount(size_t count);

  inline unsigned addVertex(const Vector3f &v) { return addVertices(&v, 1u); }
  unsigned addVertices(const Vector3f *v, size_t count);
  inline bool setVertex(size_t at, const Vector3f &v) { return setVertices(at, &v, 1u) == 1u; }
  unsigned setVertices(size_t at, const Vector3f *v, size_t count);
  const Vector3f *vertices() const;
  virtual VertexStream vertices(int stream) const override;

  unsigned indexCount() const;
  virtual unsigned indexCount(int stream) const override;
  void setIndexCount(size_t count);
  void reserveIndexCount(size_t count);

  inline void addIndex(uint32_t i) { return addIndices(&i, 1u); }
  void addIndices(const uint32_t *idx, size_t count);
  inline bool setIndex(size_t at, uint32_t i) { return setIndices(at, &i, 1u) == 1u; }
  unsigned setIndices(size_t at, const uint32_t *idx, size_t count);
  const uint32_t *indices() const;
  virtual VertexStream indices(int stream) const override;

  inline bool setNormal(size_t at, const Vector3f &n) { return setNormals(at, &n, 1u) == 1u; }
  unsigned setNormals(size_t at, const Vector3f *n, size_t count);
  const Vector3f *normals() const;
  virtual VertexStream normals(int stream) const override;

  inline bool setColour(size_t at, uint32_t c) { return setColours(at, &c, 1u) == 1u; }
  unsigned setColours(size_t at, const uint32_t *c, size_t count);
  const uint32_t *colours() const;
  virtual VertexStream colours(int stream) const override;

  inline bool setUv(size_t at, float u, float v)
  {
    const float uv[2] = { u, v };
    return setUvs(at, uv, 1u) == 1u;
  }
  unsigned setUvs(size_t at, const float *uvs, size_t count);
  const float *uvs() const;
  virtual VertexStream uvs(int stream) const override;

private:
  void copyOnWrite();

  bool processCreate(const MeshCreateMessage &msg, const ObjectAttributesd &attributes) override;
  bool processVertices(const MeshComponentMessage &msg, const VertexStream &stream) override;
  bool processIndices(const MeshComponentMessage &msg, const VertexStream &stream) override;
  bool processColours(const MeshComponentMessage &msg, const VertexStream &stream) override;
  bool processNormals(const MeshComponentMessage &msg, const VertexStream &stream) override;
  bool processUVs(const MeshComponentMessage &msg, const VertexStream &stream) override;

  SimpleMeshImp *_imp;
};


inline void SimpleMesh::addComponents(unsigned components)
{
  setComponents(this->components() | components);
}
}  // namespace tes

#endif  // _3ESSIMPLEMESH_H_
