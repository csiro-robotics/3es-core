//
// author: Kazys Stepanas
//
#ifndef _3ESTESSELATECYLINDER_H_
#define _3ESTESSELATECYLINDER_H_

#include <3escore/CoreConfig.h>

#include "3esvector3.h"

#include <vector>

namespace tes
{
namespace cylinder
{
/// Tessellate a solid cylinder mesh. The mesh is considered solid in that it is not transparent (as opposed to
/// wireframe).
///
/// Vertices are duplicated as required in order not to smooth normals around corners even when not calculating
/// normals.
///
/// The centre of the cylinder is at (0, 0, 0).
///
/// @param[out] vertices Populated with the mesh vertices.
/// @param[out] indices Populated with the mesh indices.
/// @param[out] normals Populated with per vertex normals.
/// @param apex The location of the apex vertex. The @p axis points towards this vertex.
/// @param axis The cylinder axis.
/// @param height The height of the cylinder.
/// @param radius The cylinder radius.
/// @param facets The number of facets around the shape to tessellate with.
/// @param open True to leave the ends open.
void TES_CORE_API solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> &normals,
                        const Vector3f &axis, float height, float radius, unsigned facets, bool open = false);

/// @overload
/// Does not calculate normals.
void TES_CORE_API solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, const Vector3f &axis,
                        float height, float radius, unsigned facets, bool open = false);

/// Build a wireframe cylinder. This is two rings connected by lines.
/// @param[out] vertices Populated with the mesh vertices.
/// @param[out] indices Populated with the mesh indices.
/// @param[out] normals Populated with per vertex normals.
/// @param apex The location of the apex vertex. The @p axis points towards this vertex.
/// @param axis The cylinder axis.
/// @param height The height of the cylinder.
/// @param radius The cylinder radius.
/// @param segments Number of segments in the cylinder rings.
void TES_CORE_API wireframe(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, const Vector3f &axis,
                            float height, float radius, unsigned segments);
}  // namespace cylinder
}  // namespace tes

#endif  // _3ESTESSELATECYLINDER_H_
