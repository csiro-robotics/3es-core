//
// author: Kazys Stepanas
//
#ifndef _3ESTESSELATECONE_H_
#define _3ESTESSELATECONE_H_

#include <3escore/CoreConfig.h>

#include "3esvector3.h"

#include <vector>

namespace tes
{
namespace cone
{
/// Tessellate a solid cone mesh. The mesh is considered solid in that it is not transparent (as opposed to
/// wireframe).
///
/// Vertices are duplicated as required in order not to smooth normals around corners even when not calculating
/// normals.
///
/// @param[out] vertices Populated with the mesh vertices.
/// @param[out] indices Populated with the mesh indices.
/// @param[out] normals Populated with per vertex normals.
/// @param apex The location of the apex vertex. The @p axis points towards this vertex.
/// @param axis The cone axis. The base is at <tt>apex - height * axis</tt>
/// @param height The height of the cone.
/// @param angle The angle between the cone axis vector and the walls.
/// @param facets The number of facets around the shape to tessellate with.
void TES_CORE_API solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> &normals,
                        const Vector3f &apex, const Vector3f &axis, float height, float angle, unsigned facets);

/// @overload
/// Does not calculate normals.
void TES_CORE_API solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, const Vector3f &apex,
                        const Vector3f &axis, float height, float angle, unsigned facets);

/// Build a wireframe cone. This builds a ring around the base and connects them to the apex. The apex is the first
/// vertex added.
///
/// @param[out] vertices Populated with the mesh vertices.
/// @param[out] indices Populated with the mesh indices.
/// @param[out] normals Populated with per vertex normals.
/// @param apex The location of the apex vertex. The @p axis points towards this vertex.
/// @param axis The cone axis. The base is at <tt>apex - height * axis</tt>
/// @param height The height of the cone.
/// @param angle The angle between the cone axis vector and the walls.
/// @param segments Number of segments to the base ring.
/// @return
void TES_CORE_API wireframe(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, const Vector3f &apex,
                            const Vector3f &axis, float height, float angle, unsigned segments);
}  // namespace cone
}  // namespace tes

#endif  // _3ESTESSELATECONE_H_
