#include "3esboundsculler.h"

#include <Magnum/Math/Intersection.h>

namespace tes::viewer
{
BoundsCuller::BoundsCuller() = default;
BoundsCuller::~BoundsCuller() = default;


BoundsId BoundsCuller::allocate(const Magnum::Vector3 &centre, const Magnum::Vector3 &half_extents)
{
  BoundsId new_id = invalidBoundsId();
  if (_free_list_head != invalidBoundsId())
  {
    new_id = _free_list_head;
    _free_list_head = _bounds[_free_list_head].id;
  }
  else
  {
    new_id = BoundsId(_bounds.size());
    _bounds.emplace_back();
  }

  auto &bounds = _bounds[new_id];
  bounds.centre = centre;
  bounds.half_extents = half_extents;
  // Ensure it's not visible.
  bounds.visible_mark = _last_mark - 1;
  bounds.id = new_id;

  return new_id;
}


void BoundsCuller::release(BoundsId id)
{
  if (id < _bounds.size())
  {
    if (_bounds[id].id == id)
    {
      _bounds[id].id = _free_list_head;
      _free_list_head = id;
    }
  }
}


void BoundsCuller::update(BoundsId id, const Magnum::Vector3 &centre, const Magnum::Vector3 &half_extents)
{
  auto &bounds = _bounds.at(id);
  bounds.centre = centre;
  bounds.half_extents = half_extents;
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
