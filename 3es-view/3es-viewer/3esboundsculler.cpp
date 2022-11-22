#include "3esboundsculler.h"

#include <Magnum/Math/Intersection.h>


namespace tes::viewer
{
constexpr BoundsId BoundsCuller::kInvalidId;

BoundsCuller::BoundsCuller() = default;
BoundsCuller::~BoundsCuller() = default;


BoundsId BoundsCuller::allocate(const Bounds &bounds)
{
  auto bounds = _bounds.allocate();
  bounds->bounds = bounds;
  // Ensure it's not visible.
  bounds->visible_mark = _last_mark - 1;

  return bounds.id();
}


void BoundsCuller::release(BoundsId id)
{
  _bounds.release(id);
}


void BoundsCuller::update(BoundsId id, const Bounds &bounds)
{
  auto bounds = _bounds.at(id);
  if (bounds.isValid())
  {
    bounds->bounds = bounds;
  }
}


void BoundsCuller::cull(unsigned mark, const Magnum::Math::Frustum<Magnum::Float> &view_frustum)
{
  for (auto &bounds : _bounds)
  {
    const auto centre = bounds.bounds.centre();
    const auto half_extents = bounds.bounds.halfExtents();
    bounds.visible_mark =
      (Magnum::Math::Intersection::aabbFrustum({ centre.x, centre.y, centre.z },
                                               { half_extents.x, half_extents.y, half_extents.z }, view_frustum)) ?
        mark :
        bounds.visible_mark;
  }
  _last_mark = mark;
}

}  // namespace tes::viewer
