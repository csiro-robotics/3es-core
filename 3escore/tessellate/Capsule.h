//
// author: Kazys Stepanas
//
#ifndef _3ESTESSELATECAPSULE_H_
#define _3ESTESSELATECAPSULE_H_

#include <3escore/CoreConfig.h>

//
#include <3escore/Vector3.h>

#include <array>
#include <vector>

namespace tes
{
namespace capsule
{
/// Indices for the capsule parts in @c part_isolated_index_offsets argument of @c tes::capsule::solid() .
enum class PartIndex : int
{
  TopStart,     ///< Top hemisphere start indices.
  BottomStart,  ///< Bottom hemisphere start indices.
  BodyStart,    ///< Body cylinder start indices
  BodyEnd,      ///< Body cylinder end indices

  TopEnd = BottomStart,  ///< Top hemisphere end indices.
  BottomEnd = BodyStart  ///< Bottom hemisphere end indices.
};

/// Offsets for the indices when building a capsule where we keep the three parts separate in the output.
///
/// When using @c tes::capsule::solid(), the @c part_isolated_index_offsets parameter can be used to indicate the three
/// parts should keep isolated indexing for the three parts of the capsule - top cap, end cap and cylinder body.
/// When given, each element of @p part_isolated_index_offsets is populated with the starting @p vertices index and
/// @p indices index for each part.
///
/// For example, we may end up with the given @c vertices / @c indices index pairs:
///
/// - 0, 0 (top hemisphere)
/// - 48, 144 (bottom hemisphere)
/// - 96, 288 (cylinder)
/// - 128, 480 (end)
///
/// This indicates the top hemisphere vertices start at vertices array index 0 and the indices at indices array index
/// zero. The indices range to the bottom hemisphere start indices at 48, 144 - vertices array and indices array indexes
/// respectively. The bottom hemisphere ends where the cylinder starts at 96, 288 and ends at the values given by the
/// last entry.
///
/// The @c PartIndex enum can be used to semantically index into @c part_isolated_index_offsets .
struct PartIndexOffset
{
  /// Vertices array index offset.
  unsigned vertices;
  /// Indices array index offset.
  unsigned indices;
};

/// Tessellate a solid capsule mesh. The mesh is considered solid in that it is not transparent (as opposed to
/// wireframe).
///
/// Vertices are duplicated as required in order not to smooth normals around corners even when not calculating
/// normals.
///
/// The centre of the capsule is at (0, 0, 0), it points "up" along the @p axis . The cylindrical body length is given
/// by @p height and the @p radius applies to the cylinder and the hemispherical end caps.
///
/// The mesh is normally generated as a single, coherent mesh, but it can be constructed using isolated indices for each
/// of the three parts - top cap, bottom cap and (cylinder) body. Passing a non-null @p part_isolated_index_offsets
/// builds with isolated indexing - see @c PartIndexOffset for details. Additionally, when building isolated components,
/// it can be useful to keep the end caps at the centre of the capsule, rather than offset to connect to the body. This
/// is controlled by the parameter @p local_end_caps .
///
/// This entire use case is designed to allow a single capsule to be constructed, then rendered at different scales by
/// applying a different transformation matrix to each of the components as uniform scaling does not apply well to the
/// end caps. Typically, a capsule should be scaled such that;
///
/// - The body is scaled along the axis by the desired capsule length (Z by default)
/// - The body is scaled perpendicular to the axes by the desired radius (X/Y by default)
/// - The end caps are scaled uniformly by the desired radius.
/// - The end caps are offset by half the capsule length along the axis: + for top, - for bottom.
///
/// @param[out] vertices Populated with the mesh vertices.
/// @param[out] indices Populated with the mesh indices.
/// @param[out] normals Populated with per vertex normals.
/// @param height The height (length) of the cylinder body. The radius also adds to the total height.
/// @param radius Radius of the end caps and the cylinder body.
/// @param facets Number of faces for the cylinder body. Also affects sphere tesselation to suit.
/// @param axis The capsule primary axis.
/// @param[out] part_isolated_index_offsets Optional - populated with index isolation offsets. See documentation body.
/// @param local_end_caps True to build the end caps at the capsule cylinder. Only intended for isolated construction.
void TES_CORE_API solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> &normals,
                        float height, float radius, unsigned facets, const Vector3f &axis = Vector3f(0, 0, 1),
                        std::array<PartIndexOffset, 4> *part_isolated_index_offsets = nullptr,
                        bool local_end_caps = false);

/// @overload
/// Does not calculate normals.
void TES_CORE_API solid(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, float height, float radius,
                        unsigned facets, const Vector3f &axis = Vector3f(0, 0, 1),
                        std::array<PartIndexOffset, 4> *part_isolated_index_offsets = nullptr,
                        bool local_end_caps = false);

/// Tesselate a wireframe or line based "capsule".
///
/// The @p part_isolated_index_offsets and @p local_end_caps parameters have the same semantics as for @c solid() .
///
/// @param[out] vertices Populated with the mesh vertices.
/// @param[out] indices Populated with the mesh indices.
/// @param[out] normals Populated with per vertex normals.
/// @param height The height (length) of the cylinder body. The radius also adds to the total height.
/// @param radius Radius of the end caps and the cylinder body.
/// @param segments Number of line segments used for the end cap rings. Does not affect the cylinder part.
/// @param axis The capsule primary axis.
/// @param[out] part_isolated_index_offsets Optional - populated with index isolation offsets. See documentation body.
/// @param local_end_caps True to build the end caps at the capsule cylinder. Only intended for isolated construction.
void TES_CORE_API wireframe(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, float height, float radius,
                            unsigned segments, const Vector3f &axis = Vector3f(0, 0, 1),
                            std::array<PartIndexOffset, 4> *part_isolated_index_offsets = nullptr,
                            bool local_end_caps = false);
}  // namespace capsule
}  // namespace tes

#endif  // _3ESTESSELATECAPSULE_H_
