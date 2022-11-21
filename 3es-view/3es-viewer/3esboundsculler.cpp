#include "3esboundsculler.h"

#include <Magnum/Math/Intersection.h>


namespace tes::viewer
{
BoundsCuller::BoundsCuller() = default;
BoundsCuller::~BoundsCuller() = default;


BoundsId BoundsCuller::allocate(const Magnum::Vector3 &centre, const Magnum::Vector3 &half_extents)
{
  auto bounds = _bounds.allocate();
  bounds->centre = centre;
  bounds->half_extents = half_extents;
  // Ensure it's not visible.
  bounds->visible_mark = _last_mark - 1;
  bounds->id = bounds.id();

  return bounds.id();
}


void BoundsCuller::release(BoundsId id)
{
  _bounds.release(id);
}


void BoundsCuller::update(BoundsId id, const Magnum::Vector3 &centre, const Magnum::Vector3 &half_extents)
{
  auto bounds = _bounds.at(id);
  if (bounds.isValid())
  {
    bounds->centre = centre;
    bounds->half_extents = half_extents;
  }
}


void BoundsCuller::cull(unsigned mark, const Magnum::Math::Frustum<Magnum::Float> &view_frustum)
{
  for (auto &bounds : _bounds)
  {
    bounds.visible_mark = (Magnum::Math::Intersection::aabbFrustum(bounds.centre, bounds.half_extents, view_frustum)) ?
                            mark :
                            bounds.visible_mark;
  }
  _last_mark = mark;
}

}  // namespace tes::viewer
