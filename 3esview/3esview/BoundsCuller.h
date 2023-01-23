#ifndef TES_VIEW_BOUNDS_CULLER_H
#define TES_VIEW_BOUNDS_CULLER_H

#include "3esview/ViewConfig.h"

#include "FrameStamp.h"
#include "MagnumV3.h"
#include "util/ResourceList.h"

#include <3escore/Bounds.h>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Frustum.h>
#include <Magnum/Math/Vector3.h>

#include <mutex>
#include <vector>

namespace tes::view
{
using BoundsId = util::ResourceListId;

class TES_VIEWER_API Bounds : public tes::Bounds<Magnum::Float>
{
public:
  using Super = tes::Bounds<Magnum::Float>;

  inline Bounds()
    : Super()
  {}
  inline Bounds(const Bounds &other)
    : Super(other)
  {}
  inline Bounds(const Magnum::Vector3 &min_ext, const Magnum::Vector3 &max_ext)
    : Super(convert(min_ext), convert(max_ext))
  {}
  inline Bounds(const Magnum::Vector3 &point)
    : Super(convert(point))
  {}
  inline Bounds(const tes::Vector3<Magnum::Float> &min_ext, const tes::Vector3<Magnum::Float> &max_ext)
    : Super(min_ext, max_ext)
  {}
  inline Bounds(const tes::Vector3<Magnum::Float> &point)
    : Super(point)
  {}

  /// Create a bounds structure from centre and half extents values.
  /// @param centre The bounds centre.
  /// @param half_extents Bounds half extents.
  /// @return The bounds AABB.
  static Bounds fromCentreHalfExtents(const Magnum::Vector3 &centre, const Magnum::Vector3 &half_extents)
  {
    return { centre - half_extents, centre + half_extents };
  }

  /// Access the minimum extents.
  /// @return The minimal corder of the bounding box.
  inline const Magnum::Vector3 &minimum() const { return convert(Super::minimum()); }
  /// Access the maximum extents.
  /// @return The maximal corder of the bounding box.
  inline const Magnum::Vector3 &maximum() const { return convert(Super::maximum()); }

  /// Get the bounds centre point.
  /// @return The bounds centre.
  Magnum::Vector3 centre() const { return convert(Super::centre()); }
  /// Get the bounds half extents, from centre to max.
  /// @return The half extents, centre to max.
  Magnum::Vector3 halfExtents() const { return convert(Super::halfExtents()); }

  /// Expand the bounding box to include @p point.
  /// @param point The point to include.
  inline void expand(const Magnum::Vector3 &point) { expand(convert(point)); }

  /// Expand the bounding box to include @p other.
  /// @param other The bounds to include.
  inline void expand(const Bounds &other) { Super::expand(other); }

  /// Precise equality operator.
  /// @param other The object to compare to.
  /// @return True if this is precisely equal to @p other.
  inline bool operator==(const Bounds &other) const { return Super::operator==(other); }

  /// Precise inequality operator.
  /// @param other The object to compare to.
  /// @return True if this is no precisely equal to @p other.
  inline bool operator!=(const Bounds &other) const { return Super::operator!=(other); }

  /// Assignment operator.
  /// @param other The bounds to copy.
  /// @return @c this.
  inline Bounds &operator=(const Bounds &other)
  {
    Super::operator=(other);
    return *this;
  }
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
class TES_VIEWER_API BoundsCuller
{
public:
  using Bounds = tes::Bounds<Magnum::Float>;
  static constexpr BoundsId kInvalidId = util::kNullResource;

  /// Constructor.
  BoundsCuller();
  /// Destructor.
  ~BoundsCuller();

  /// Check if a bounds entry is visible at a particular @p render_mark .
  /// @param id Bounds entry ID to check visibility of. Must be a valid entry or behaviour is undefined.
  /// @param render_mark The render mark to check visibility against.
  /// @return True if the bounds entry with @p id is visible at the given @p render_mark .
  bool isVisible(BoundsId id, unsigned render_mark) const;

  /// Check if a bounds entry was visible at the last mark given to @p cull() .
  /// @param id Bounds entry ID to check visibility of. Must be a valid entry or behaviour is undefined.
  /// @return True if the bounds entry with @p id is visible by the last @c cull() call.
  inline bool isVisible(BoundsId id) const { return isVisible(id, _last_mark); }

  /// Allocate a new bounds entry with the given bounds.
  /// @param bounds Bounds AABB.
  /// @return The bound entry ID.
  BoundsId allocate(const Bounds &bounds);

  /// Release a bounds entry.
  /// @param id ID of the entry to release. Must be a valid entry or behaviour is undefined.
  void release(BoundsId id);

  /// Update an existing bounds entry to the given bounds.
  /// @param bounds Bounds AABB.
  /// @param id ID of the entry to release. Must be a valid entry or behaviour is undefined.
  void update(BoundsId id, const Bounds &bounds);

  /// Perform bounds culling on all registered bounds.
  /// @param mark The render mark to stamp visible bounds entries with.
  /// @param view_frustum The view frustum to cull against.
  void cull(unsigned mark, const Magnum::Math::Frustum<Magnum::Float> &view_frustum);

private:
  /// Culling bounds structure.
  struct CullBounds
  {
    Bounds bounds;
    /// Render stamp for which the bounds were last in view.
    RenderStamp visible_mark = 0;
  };

  using ResourceList = util::ResourceList<CullBounds>;
  ResourceList _bounds;
  RenderStamp _last_mark = ~0;
};


inline bool BoundsCuller::isVisible(BoundsId id, unsigned render_mark) const
{
  auto bounds = _bounds.at(id);
  return bounds.isValid() && bounds->visible_mark == render_mark;
}
}  // namespace tes::view

#endif  // TES_VIEW_BOUNDS_CULLER_H
