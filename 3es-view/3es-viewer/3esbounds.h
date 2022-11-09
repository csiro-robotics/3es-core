#ifndef TES_VIEWER_BOUNDS_H
#define TES_VIEWER_BOUNDS_H

#include "3es-viewer.h"

#include <Magnum/Magnum.h>
#include <Magnum/Math/Frustum.h>
#include <Magnum/Math/Vector3.h>

#include <vector>

namespace tes
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
class BoundsCuller
{
public:
  inline bool isVisible(BoundsId id, unsigned render_mark) const { return _bounds[id].visible_mark == render_mark; }
  inline bool isVisible(BoundsId id) const { return isVisible(id, _last_mark); }

  BoundsId allocate(const Magnum::Vector3 &centre, const Magnum::Vector3 &half_extents);
  void release(BoundsId id);

  void update(BoundsId id, const Magnum::Vector3 &centre, const Magnum::Vector3 &half_extents);

  void cull(unsigned mark, const Magnum::Math::Frustum<Magnum::Float> &view_frustum);

private:
  std::vector<Bounds> _bounds;
  unsigned _free_list_head = invalidBoundsId();
  unsigned _last_mark = 0;
};
}  // namespace tes

#endif  // TES_VIEWER_BOUNDS_H
