//
// author: Kazys Stepanas
//
#ifndef _3ESTESSELATEBOX_H_
#define _3ESTESSELATEBOX_H_

#include <3escore/CoreConfig.h>

#include "Vector3.h"

#include <vector>

namespace tes
{
namespace box
{
/// Tessellate a solid box mesh. The mesh is considered solid in that it is not transparent (as opposed to
/// wireframe).
///
/// Vertices are duplicated as required in order not to smooth normals around corners even when not calculating
/// normals.
///
/// @param[out] vertices Populated with the mesh vertices.
/// @param[out] indices Populated with the mesh indices.
/// @param[out] normals Populated with per vertex normals.
/// @param scale The scale of the box along each axis where 1 makes a unit length side.
void TES_CORE_API solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> &normals,
                        const Vector3f &scale = Vector3f(1.0f));

/// @overload
/// Does not calculate normals.
void TES_CORE_API solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
                        const Vector3f &scale = Vector3f(1.0f));

/// Build a line drawn/wireframe box mesh.
///
/// @param[out] vertices Populated with the mesh vertices.
/// @param[out] indices Populated with the mesh indices.
/// @param scale The scale of the box along each axis where 1 makes a unit length side.
void TES_CORE_API wireframe(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
                            const Vector3f &scale = Vector3f(1.0f));
}  // namespace box
}  // namespace tes

#endif  // _3ESTESSELATEBOX_H_
