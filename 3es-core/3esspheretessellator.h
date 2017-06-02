//
// author: Kazys Stepanas
//
#ifndef _3ESSPHERETESSELLATOR_H_
#define _3ESSPHERETESSELLATOR_H_

#include "3es-core.h"

#include "3esvector3.h"
#include "3esvectorhash.h"

#include <unordered_map>
#include <vector>

/// Tessellates the plane primitive.
namespace tes
{
  struct _3es_coreAPI SphereVertexHash
  {
    size_t operator()(const Vector3f &v) const;
    size_t operator()(const Vector3d &v) const;
  };

  typedef std::unordered_map<Vector3f, unsigned, SphereVertexHash> SphereVertexMap;

  inline size_t SphereVertexHash::operator()(const Vector3f &v) const
  {
    return vhash::hash(v.x, v.y, v.z);
  }


  inline size_t SphereVertexHash::operator()(const Vector3d &v) const
  {
    return vhash::hash(float(v.x), float(v.y), float(v.z));
  }

  /// Initialise a unity sphere as an icosahedron.
  /// @param vertices Sphere vertices.
  /// @param indices Triangle indices
  /// @param vertexMap Optional map to populate. Must be provided when using @c subdivideUnitSphere().
  void _3es_coreAPI sphereInitialise(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, SphereVertexMap *vertexMap = nullptr);

  /// Subdivide a unit sphere. Intended for iterative subdivision of a sphere generated by @c sphereInitialise().
  /// @param vertices Sphere vertices.
  /// @param indices Triangle indices.
  /// @param vertexMap Vertex hash map for preventing duplicate vertices.
  void _3es_coreAPI subdivideUnitSphere(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, SphereVertexMap &vertexMap);

  /// Tessellate a sphere using a subdivision technique starting from an icosahedron.
  void _3es_coreAPI sphereSubdivision(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, float radius = 1.0f, const Vector3f &origin = Vector3f(0.0f), int depth = 2);
};

#endif // _3ESSPHERETESSELLATOR_H_
