#include "BoundsCuller.h"

#include <Magnum/Math/Intersection.h>


namespace tes::view
{
Bounds Bounds::calculateLooseBounds(const Magnum::Matrix4 &transform) const
{
  const auto centre = this->centre();
  const auto half_ext = this->halfExtents();
  std::array<Magnum::Vector3, 8> vertices = {
    centre + Magnum::Vector3(-half_ext.x(), -half_ext.y(), -half_ext.z()),
    centre + Magnum::Vector3(half_ext.x(), -half_ext.y(), -half_ext.z()),
    centre + Magnum::Vector3(half_ext.x(), half_ext.y(), -half_ext.z()),
    centre + Magnum::Vector3(-half_ext.x(), half_ext.y(), -half_ext.z()),
    centre + Magnum::Vector3(-half_ext.x(), -half_ext.y(), half_ext.z()),
    centre + Magnum::Vector3(half_ext.x(), -half_ext.y(), half_ext.z()),
    centre + Magnum::Vector3(half_ext.x(), half_ext.y(), half_ext.z()),
    centre + Magnum::Vector3(-half_ext.x(), half_ext.y(), half_ext.z()),
  };

  for (auto &vert : vertices)
  {
    vert = (transform * Magnum::Vector4(vert, 1)).xyz();
  }

  Bounds loose_bounds(vertices[0], vertices[0]);
  for (const auto &vert : vertices)
  {
    loose_bounds.expand(vert);
  }

  return loose_bounds;
}


constexpr BoundsId BoundsCuller::kInvalidId;

BoundsCuller::BoundsCuller() = default;
BoundsCuller::~BoundsCuller() = default;


BoundsId BoundsCuller::allocate(const Bounds &bounds)
{
  auto cull_bounds = _bounds.allocate();
  cull_bounds->bounds = bounds;
  // Ensure it's not visible.
  cull_bounds->visible_mark = _last_mark - 1;

  return cull_bounds.id();
}


void BoundsCuller::release(BoundsId id)
{
  if (id != kInvalidId)
  {
    _bounds.release(id);
  }
}


void BoundsCuller::update(BoundsId id, const Bounds &bounds)
{
  auto cull_bounds = _bounds.at(id);
  if (cull_bounds.isValid())
  {
    cull_bounds->bounds = bounds;
  }
}


void BoundsCuller::cull(unsigned mark, const Magnum::Math::Frustum<Magnum::Float> &view_frustum)
{
  for (auto &bounds : _bounds)
  {
    const auto centre = bounds.bounds.centre();
    const auto half_extents = bounds.bounds.halfExtents();
    bounds.visible_mark =
      (Magnum::Math::Intersection::aabbFrustum(
        { centre.x(), centre.y(), centre.z() },
        { half_extents.x(), half_extents.y(), half_extents.z() }, view_frustum)) ?
        mark :
        bounds.visible_mark;
  }
  _last_mark = mark;
}

}  // namespace tes::view
