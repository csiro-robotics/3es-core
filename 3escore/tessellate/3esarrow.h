//
// author: Kazys Stepanas
//
#ifndef _3ESTESSELATEARROW_H_
#define _3ESTESSELATEARROW_H_

#include <3escore/CoreConfig.h>

#include "3esvector3.h"

#include <vector>

namespace tes
{
namespace arrow
{
/// Tessellate a solid arrow mesh. The mesh is considered solid in that it is not transparent (as opposed to
/// wireframe).
///
/// Vertices are duplicated as required in order not to smooth normals around corners even when not calculating
/// normals.
///
/// @param[out] vertices Populated with the mesh vertices.
/// @param[out] indices Populated with the mesh indices.
/// @param[out] normals Populated with per vertex normals.
/// @param facets Number of facets around the arrow head and cylinder to tessellate with. Must be > 2.
/// @param headRadius Radius of the arrow head base. Should be larger than the @p cylinderRadius
/// @param cylinderRadius The radius of the arrow shaft. Must be smaller than the @p headRadius
/// @param cylinderLength Length of the cylinder/shaft part. Must be less than the @p arrowLength
/// @param axis The axis along which the arrow points.
/// @return True if the parameterisation results in a valid shape.
bool _3es_coreAPI solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> &normals,
                        unsigned facets, float headRadius, float cylinderRadius, float cylinderLength,
                        float arrowLength, const Vector3f axis = Vector3f(0, 0, 1));

/// @overload
/// Does not calculate normals.
bool _3es_coreAPI solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, unsigned facets,
                        float headRadius, float cylinderRadius, float cylinderLength, float arrowLength,
                        const Vector3f axis = Vector3f(0, 0, 1));

/// Build a wireframe arrow. This is a wireframe cone and cylinder combined.
/// @param[out] vertices Populated with the mesh vertices.
/// @param[out] indices Populated with the mesh indices.
/// @param[out] normals Populated with per vertex normals.
/// @param facets Number of facets around the arrow head and cylinder to tessellate with. Must be > 2.
/// @param headRadius Radius of the arrow head base. Should be larger than the @p cylinderRadius
/// @param cylinderRadius The radius of the arrow shaft. Must be smaller than the @p headRadius
/// @param cylinderLength Length of the cylinder/shaft part. Must be less than the @p arrowLength
/// @param axis The axis along which the arrow points.
/// @return True if the parameterisation results in a valid shape.
bool _3es_coreAPI wireframe(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, unsigned facets,
                            float headRadius, float cylinderRadius, float cylinderLength, float arrowLength,
                            const Vector3f axis = Vector3f(0, 0, 1));
}  // namespace arrow
}  // namespace tes

#endif  // _3ESTESSELATEARROW_H_
