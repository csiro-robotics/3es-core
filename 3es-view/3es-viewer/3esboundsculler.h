#ifndef TES_VIEWER_BOUNDS_CULLER_H
#define TES_VIEWER_BOUNDS_CULLER_H

#include "3es-viewer.h"

#include <Magnum/Magnum.h>
#include <Magnum/Math/Frustum.h>
#include <Magnum/Math/Vector3.h>

#include <vector>

namespace tes::viewer
{
using BoundsId = unsigned;

constexpr inline BoundsId invalidBoundsId()
{
  return ~0u;
}

/// Culling bounds structure.
struct Bounds
{
  /// Lower bounds.
  Magnum::Vector3 centre;
  /// Upper bounds.
  Magnum::Vector3 half_extents;
  /// Render stamp for which the bounds were last in view.
  unsigned visible_mark = 0;
  /// Bounds culling id.
  ///
  /// Internally used to address the next item in the free list.
  unsigned id = 0;
};

/// Bounds culling system.
///
/// Anything which requires a bounds check for rendering can add a bounds entry via @c allocate() , which is disposed of
/// with @c release() when no longer required. The @c allocate() function returns a @c BoundsId is used to @c update()
/// the bounds, check @c isVisible() or @c release() when done.
///
/// Before rendering, the @c BoundsCuller must have @c cull() called in order to update the visibility of all tracked
/// bounds entries. This requires a @c mark value which identifies the current frame. The particular value is especially
/// important, so long as it changes each frame and has a long period before returning to the same value. During
/// @p cull() each bounds visible bounds entry is stamped with this @p mark value. The same @p mark can later be used to
/// check visibility via @p isVisible() .
class BoundsCuller
{
public:
  /// Constructor.
  BoundsCuller();
  /// Destructor.
  ~BoundsCuller();

  /// Check if a bounds entry is visible at a particular @p render_mark .
  /// @param id Bounds entry ID to check visibility of. Must be a valid entry or behaviour is undefined.
  /// @param render_mark The render mark to check visibility against.
  /// @return True if the bounds entry with @p id is visible at the given @p render_mark .
  inline bool isVisible(BoundsId id, unsigned render_mark) const { return _bounds[id].visible_mark == render_mark; }

  /// Check if a bounds entry was visible at the last mark given to @p cull() .
  /// @param id Bounds entry ID to check visibility of. Must be a valid entry or behaviour is undefined.
  /// @return True if the bounds entry with @p id is visible by the last @c cull() call.
  inline bool isVisible(BoundsId id) const { return isVisible(id, _last_mark); }

  /// Allocate a new bounds entry with the given bounds.
  /// @param centre Bounds centre.
  /// @param half_extents Bounds AABB half extents.
  /// @return The bound entry ID.
  BoundsId allocate(const Magnum::Vector3 &centre, const Magnum::Vector3 &half_extents);

  /// Release a bounds entry.
  /// @param id ID of the entry to release. Must be a valid entry or behaviour is undefined.
  void release(BoundsId id);

  /// Update an existing bounds entry to the given bounds.
  /// @param centre Bounds centre.
  /// @param half_extents Bounds AABB half extents.
  /// @param id ID of the entry to release. Must be a valid entry or behaviour is undefined.
  void update(BoundsId id, const Magnum::Vector3 &centre, const Magnum::Vector3 &half_extents);

  /// Perform bounds culling on all registered bounds.
  /// @param mark The render mark to stamp visible bounds entries with.
  /// @param view_frustum The view frustum to cull against.
  void cull(unsigned mark, const Magnum::Math::Frustum<Magnum::Float> &view_frustum);

private:
  /// Bounds array.
  std::vector<Bounds> _bounds;
  /// Bounds array free list head.
  unsigned _free_list_head = invalidBoundsId();
  /// The mast @c mark given to @c cull() .
  unsigned _last_mark = 0;
};
}  // namespace tes::viewer

#endif  // TES_VIEWER_BOUNDS_CULLER_H
