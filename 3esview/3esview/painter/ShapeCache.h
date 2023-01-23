#ifndef TES_VIEWER_SHAPE_CACHE_H
#define TES_VIEWER_SHAPE_CACHE_H

#include <3esview/ViewConfig.h>

#include "3esboundsculler.h"
#include "3esviewablewindow.h"
#include "util/3esresourcelist.h"
#include "util/3esenum.h"

#include <shapes/3esid.h>

#include <Magnum/GL/Mesh.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/Shaders/MeshVisualizer.h>

#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace tes::viewer
{
class BoundsCuller;
}  // namespace tes::viewer

namespace tes::viewer::shaders
{
class Shader;
}  // namespace tes::viewer::shaders

namespace tes::viewer::painter
{
/// An instanced shape rendering cache.
///
/// A shape cache is designed to rendering the same mesh/shape multiple times using instanced rendering, with each shape
/// afforded a transformation matrix and a colour. A shape cache has the following components passed on construction to
/// assist in preparation and rendering;
///
/// - a @c BoundsCuller (shared) for visibility determination
/// - bounds calculation for each primitive to update bounds for the @c BoundsCuller
/// - a @c Mesh to draw (transferred ownership)
/// - a @c Shader to draw the mesh with (transferred ownership)
/// - an optional mesh transformation applied to the @c Mesh
///
/// Shapes are added using @c add() specifying the transform and colour for the shape instance. This in turn adds a new
/// bounds entry in the @c BoundsCuller , calculated using @c calcBounds() . The cache draws all current shapes
/// when @c draw() is called, using the cache's shader to draw it's mesh using instanced semantics.
///
/// When added, a shape is assigned an ID (ostensibly it's internal index), which can be used to @c update() or
/// @c remove() the shape. Updating a shape recalculates the bounds. Removing a shape releases the shape bounds entry
/// back to the @c BoundsCuller .
///
/// Internally the cache maintains a free list which recycles IDs/indices in a LIFO order.
///
/// Shapes may be added with a parent specified. Shapes with a parent use the parent transform in calculating their
/// final transform and are visible so long as the parent is visible. @c endShape() should only be called for the parent
/// shape and not for child shapes. Shape parenting primarily supports multi-shape specifications allowing shapes to be
/// addressed collectively. The are added by first adding the parent shape and noting its index. Other shapes in the are
/// added passing this index to the @c add() function. The parent shape forms the head of a linked list, with additional
/// shapes inserted just after the parent shape.
///
/// Child shapes may have @c update() called, although the parent transform always affects the child transform.
/// Shape chains are removed collectively by specifying the parent shape.
class TES_VIEWER_API ShapeCache
{
  struct Shape;

public:
  /// Helper function used to implement @c calcBounds() for the cached shape type.
  ///
  /// The calculation must vary depending on the shape type. For spheres, for example, the bounds are constant and the
  /// default implementation is used, where the @p transform scale is mapped to @p halfExtents . Other shapes need
  /// to consider the effects of rotation.
  ///
  /// @param transform The shape transformation matrix.
  /// @param[out] bounds Set to the calculated bounds axis aligned box.
  using BoundsCalculator = std::function<void(const Magnum::Matrix4 &transform, Bounds &bounds)>;
  /// Signature for the modifier function applied when rendering a shape transform.
  /// @param[in,out] transform The transform to modify.
  using TransformModifier = std::function<void(Magnum::Matrix4 &transform)>;

  class const_iterator;

  /// Shape marker flags.
  enum class ShapeFlag : unsigned
  {
    /// No flags.
    None = 0u,
    // External use.
    /// Marks a transient shape, which expires on the @c commit() call after it becomes visible.
    /// Removes shapes are also marked as @c Transient so they are removed on the next @c commit().
    Transient = 1u << 0u,
    /// Set to hide the shape and prevent rendering thereof.
    Hidden = 1u << 1u,

    // Internal use.
    /// Internal: Marks a shape as pending "creation" after the next @c commit().
    Pending = 1u << 8u,
    /// Internal: Marks a shape as pending an update, changing it's shape properties on the next @c commit().
    Dirty = 1u << 9u
  };

  /// Shape instance data.
  struct TES_VIEWER_API ShapeInstance
  {
    /// The instance transformation matrix.
    Magnum::Matrix4 transform = {};
    /// The instance colour.
    Magnum::Color4 colour = {};
  };

  /// A mesh and transform part for use with the @c ShapeCache .
  ///
  /// A @c ShapeCache can have one or more @c Part objects to render. Each mesh is rendered by first applying an
  /// instance transform, then the @c Part::transform then the projection matrix thusly;
  ///
  /// @code{.unparsed}
  ///   transform = projection_matrix * part.transform * instance_transform;
  /// @endcode
  struct TES_VIEWER_API Part
  {
    /// Mesh shared pointer; must not be null.
    std::shared_ptr<Magnum::GL::Mesh> mesh;
    /// Transform to apply to @c mesh before rendering.
    Magnum::Matrix4 transform = {};
    /// Tint to apply to the shape colour.
    Magnum::Color4 colour = { 1, 1, 1, 1 };

    Part() = default;
    Part(const Part &other) = default;
    Part(Part &&other) = default;
    explicit Part(std::shared_ptr<Magnum::GL::Mesh> mesh)
      : Part(std::move(mesh), Magnum::Matrix4())
    {}
    Part(std::shared_ptr<Magnum::GL::Mesh> mesh, const Magnum::Matrix4 &transform)
      : mesh(std::exchange(mesh, nullptr))
      , transform(transform)
    {}
    explicit Part(Magnum::GL::Mesh &&mesh)
      : Part(std::make_shared<Magnum::GL::Mesh>(std::move(mesh)), Magnum::Matrix4())
    {}
    Part(Magnum::GL::Mesh &&mesh, const Magnum::Matrix4 &transform)
      : Part(std::make_shared<Magnum::GL::Mesh>(std::move(mesh)), transform)
    {}

    Part &operator=(const Part &other) = default;
    Part &operator=(Part &&other) = default;
  };


  /// The default implementation of a @c BoundsCalculator , calculating a spherical bounds, unaffected by rotation.
  /// @param transform The shape transformation matrix.
  /// @param[out] centre Set to the bounds centre.
  /// @param[out] halfExtents Set to the bounds half extents vector.
  static void calcSphericalBounds(const Magnum::Matrix4 &transform, Bounds &bounds);

  /// Calculate bounds of a cylindrical object. Assumes cylinder major axis is (0, 0, 1).
  /// @param transform The shape transformation matrix.
  /// @param radius Cylindrical radius.
  /// @param length Cylindrical length.
  /// @param[out] centre Set to the bounds centre.
  /// @param[out] halfExtents Set to the bounds half extents vector.
  static void calcCylindricalBounds(const Magnum::Matrix4 &transform, float radius, float length, Bounds &bounds);

  /// Internal free list terminator value.
  static constexpr size_t kListEnd = util::kNullResource;

  /// @overload
  ShapeCache(std::shared_ptr<BoundsCuller> culler, std::shared_ptr<shaders::Shader> shader, const Part &part,
             BoundsCalculator bounds_calculator = ShapeCache::calcSphericalBounds);

  /// Construct a shape cache.
  /// @param culler The bounds culler used to create bounds and manage bounds.
  /// @param parts The mesh parts to render. An overload accepts a single @c Part .
  /// @param shader The shader used to draw the mesh.
  /// @param bounds_calculator Bounds calculation function.
  ShapeCache(std::shared_ptr<BoundsCuller> culler, std::shared_ptr<shaders::Shader> shader,
             const std::vector<Part> &parts, BoundsCalculator bounds_calculator = ShapeCache::calcSphericalBounds);

  /// @overload
  ShapeCache(std::shared_ptr<BoundsCuller> culler, std::shared_ptr<shaders::Shader> shader,
             std::initializer_list<Part> parts, BoundsCalculator bounds_calculator = ShapeCache::calcSphericalBounds);

  /// Calculate the bounds for a shape instance with the given transform.
  /// @param transform The shape instance transformation matrix.
  /// @param[out] centre Calculated bounds centre.
  /// @param[out] halfExtents Calculated bounds half extents.
  void calcBounds(const Magnum::Matrix4 &transform, Bounds &bounds) const;

  std::shared_ptr<shaders::Shader> shader() const { return _shader; }

  /// Set the bounds calculation function.
  /// @param bounds_calculator New bounds calculation function.
  void setBoundsCalculator(BoundsCalculator bounds_calculator) { _bounds_calculator = std::move(bounds_calculator); }

  /// Get the active transform modifier. May be empty.
  /// @return The transform modifier function.
  const TransformModifier &transformModifier() const { return _transform_modifier; }
  /// Set the active transform modifier. May be empty.
  ///
  /// Applied when finalising the render transform for a shape. The @c transform passed to the @p modifier will have
  /// the parent transform included.
  ///
  /// @param modifier The transform modifier function.
  void setTransformModifier(const TransformModifier &modifier) { _transform_modifier = modifier; }

  /// Add a shape instance which persists over the specified @p window . Use an open window if the end frame is not yet
  /// known.
  ///
  ///
  /// @param shape_id The user ID for the shape. This is only kept as a user value, it is never used to address a shape
  /// in the cache.
  /// @param transform The shape instance transformation.
  /// @param colour The shape instance colour.
  /// @param parent_rid Index of the parent shape whose transform also affects this shape. Use ~0u for no parent.
  ///   Must be valid when specified - i.e., the parent must be added first and removed last. Specifying the parent
  ///   index also forms a shape chain.
  /// @param child_index When adding a child shape, this will be set to the index of the child in the parent (if not
  ///   null). Behaviour is undefined when @p parent_rid is invalid.
  /// @return The shape ID/index. Must be used to @c remove() or @c update() the shape.
  util::ResourceListId add(const tes::Id &shape_id, const Magnum::Matrix4 &transform, const Magnum::Color4 &colour,
                           ShapeFlag flags = ShapeFlag::None, util::ResourceListId parent_rid = kListEnd,
                           unsigned *child_index = nullptr);
  /// Mark a shape for removal on the next @c commit() .
  /// @param id Id of the shape to remove.
  /// @return True if the @p id is valid.
  bool endShape(util::ResourceListId id);

  /// Update an existing shape instance.
  /// @param id Id of the shape to update.
  /// @param transform The shape instance transformation.
  /// @param colour The shape instance colour.
  /// @return True if the @p id was valid and an instance updated.
  bool update(util::ResourceListId id, const Magnum::Matrix4 &transform, const Magnum::Color4 &colour);

  /// Get the details of an existing shape instance at the @c activeWindow().startFrame() .
  /// @param id Id of the shape to update.
  //// @param apply_parent_transform Apply parentage when retrieving the transform?
  /// @param[out] transform Set to the shape instance transformation.
  /// @param[out] colour Set to the shape instance colour.
  /// @return True if the @p id was valid and an instance data retrieved. The out values are undefined when @p id is
  /// invalid.
  bool get(util::ResourceListId id, bool apply_parent_transform, Magnum::Matrix4 &transform,
           Magnum::Color4 &colour) const;
  /// @overload
  bool get(util::ResourceListId id, Magnum::Matrix4 &transform, Magnum::Color4 &colour) const
  {
    return get(id, false, transform, colour);
  }

  /// Lookup the resource id for a child shape.
  ///
  /// @note This is a linked list lookup, O(n).
  ///
  /// @param parent_id The parent shape's resource Id.
  /// @param child_index The index of the child.
  /// @return The resource id of the child shape, or @c util::kNullResource if the id arguments are invalid.
  util::ResourceListId getChildId(util::ResourceListId parent_id, unsigned child_index) const;

  /// Expire all shapes which were viewable before, but not at @p before_frame .
  /// @param before_frame The frame before which to expire shapes.
  void commit();

  /// Draw all shape instances considered visible by the @p render_mark .
  ///
  /// Before calling this function, the @c BoundsCuller::cull() should be called with the same @p render_mark , which
  /// ensure the bounds entries are marked as visibly for the @p render_mark .
  ///
  /// @param stamp The frame stamp to draw shapes for.
  /// @param projection_matrix View to projection matrix.
  /// @param projection_matrix World to view (inverse camera) matrix.
  void draw(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix, const Magnum::Matrix4 &view_matrix);

  /// Clear the shape cache, removing all shapes.
  ///
  /// @note Bounds are returned to the @c BoundsCuller iteratively.
  void clear();

  /// Iterator for the shape cache. Iteration is read only and shows a proxy @c View for the shape, rather than
  /// addressing the actual shape data.
  class TES_VIEWER_API const_iterator
  {
  public:
    /// An external view of a shape in the cache.
    struct TES_VIEWER_API View
    {
      /// The shape user id.
      Id id;
      /// The shape render attributes.
      ShapeInstance attributes;
      /// Number of child shapes this has.
      unsigned child_count = 0;
    };

    /// Default constructor.
    const_iterator() = default;
    /// Iteration constructor
    /// @param cursor Initial iterator position
    /// @param end End iterator for the internal cache data.
    const_iterator(util::ResourceList<Shape>::const_iterator &&cursor, util::ResourceList<Shape>::const_iterator &&end)
      : _cursor(std::move(cursor))
      , _end(std::move(end))
    {}
    /// Copy constructor.
    /// @param other Iterator to copy.
    const_iterator(const const_iterator &other) = default;
    /// Copy constructor.
    /// @param other Iterator to move.
    const_iterator(const_iterator &&other) = default;

    /// Copy assignment operator.
    /// @param other Iterator to copy.
    /// @return @c *this
    const_iterator &operator=(const const_iterator &other) = default;
    /// Move assignment operator.
    /// @param other Iterator to move.
    /// @return @c *this
    const_iterator &operator=(const_iterator &&other) = default;

    /// Equality test. Only compares the cursor.
    /// @param other Iterator to compare.
    /// @return True if the iterators are semantically equivalent.
    bool operator==(const const_iterator &other) const { return _cursor == other._cursor; }
    /// Inequality test. Only compares the cursor.
    /// @param other Iterator to compare.
    /// @return True unless the iterators are semantically equivalent.
    bool operator!=(const const_iterator &other) const { return !operator==(other); }

    /// Prefix increment
    /// @return @c *this
    const_iterator &operator++()
    {
      next();
      return *this;
    }

    /// Postfix increment
    /// @return An iterator before incrementing.
    const_iterator operator++(int)
    {
      auto iter = *this;
      next();
      return iter;
    }

    /// Dereference to a @c View.
    /// @return A @c View to the current item.
    const View &operator*() const { return _view; }
    /// Dereference to a @c View.
    /// @return A @c View to the current item.
    const View *operator->() const { return &_view; }

    /// Get the internal resource ID of the current item.
    /// @return
    util::ResourceListId rid() const { return _cursor.id(); }

  private:
    /// Iterate to the next item.
    void next();

    util::ResourceList<Shape>::const_iterator _cursor;
    util::ResourceList<Shape>::const_iterator _end;
    View _view = {};
  };

  /// Begin iteration of the shapes in the cache.
  /// @return The starting iterator.
  const_iterator begin() const { return const_iterator(_shapes.begin(), _shapes.end()); }
  /// End iterator.
  /// @return The end iterator.
  const_iterator end() const { return const_iterator(_shapes.end(), _shapes.end()); }

private:
  friend const_iterator;

  /// An entry in the shape cache.
  ///
  /// The entry is fairly intricate, consisting of;
  /// - a @c view_index into the viewables array.
  struct Shape
  {
    /// The current shape details.
    ShapeInstance current = {};
    /// The updates shape details. Only relevant if `flags & (ShapeFlag::Dirty)` is non zero.
    ShapeInstance updated = {};
    /// Behavioural flags.
    ShapeFlag flags = ShapeFlag::None;
    /// The shape entry @c BoundsCuller entry ID.
    BoundsId bounds_id = ~0u;
    /// Index of the "parent" shape. The parent shape transform also affects this shape's final transformation.
    util::ResourceListId parent_rid = kListEnd;
    /// Shape list (linked list) next item ID. Used to link the free list when a shape is not in used. Used to specify
    /// a multi-shape chain dependency for valid shapes. This value is @c kListEnd for the end of the
    /// list.
    /// @note Children appear in reverse order with the oldest at the end of the list, which is child "index" zero.
    util::ResourceListId next = kListEnd;
    /// Number of children for a parent shape.
    unsigned child_count = 0;
    /// The user shape ID. For information purposes only. Never used to address the shape.
    Id shape_id = {};

    /// Check if this is a parent shape.
    /// @return True for a parent shape.
    bool isParent() const { return parent_rid == kListEnd && next != kListEnd; }
    /// Check if this is a child shape.
    /// @return True for a child shape.
    bool isChild() const { return parent_rid != kListEnd; }
  };

  /// Instance buffer used to render shapes. Only valid during the @c draw() call.
  struct InstanceBuffer
  {
    /// Graphics buffer to which shape instances are marshalled.
    Magnum::GL::Buffer buffer{ Magnum::NoCreate };
    /// Number of items in the buffer.
    unsigned count = 0;
  };

  void calcBoundsForShape(const Shape &child, Bounds &bounds) const;

  /// Release a shape to the free list. This also releases the shape chain if this is the head of a chain.
  ///
  /// Must only be called for the head of a shape chain, not the links.
  ///
  /// @param id The shape resource id to release
  /// @return True if the shape was valid for release and successfully released.
  bool release(util::ResourceListId id);

  /// Fill the @p InstanceBuffer objects in @c _instance_buffers .
  /// @param frame_number The frame number to draw shapes for.
  /// @param render_mark Visibility render mark used to determine shape instance visibility in the @c BoundsCuller .
  void buildInstanceBuffers(const FrameStamp &stamp);


  /// The bounds culler used to determine visibility.
  std::shared_ptr<BoundsCuller> _culler;
  /// Instantiated shape array. Some may be pending first view.
  util::ResourceList<Shape> _shapes;
  /// Mesh parts to render.
  std::vector<Part> _parts;
  /// Transformation matrix applied to the shape before rendering. This allows the Magnum primitives to be transformed
  /// to suit the 3rd Eye Scene rendering.
  std::vector<InstanceBuffer> _instance_buffers;
  /// Buffer used to marshal active shape instances in @c buildInstanceBuffers() . The size of this array determines
  /// the number of instances per @p InstanceBuffer .
  std::array<ShapeInstance, 2048> _marshal_buffer;
  /// Shaper used to draw the shapes.
  std::shared_ptr<shaders::Shader> _shader;
  /// Bounds calculation function.
  BoundsCalculator _bounds_calculator = ShapeCache::calcSphericalBounds;
  /// Optional transform modifier.
  TransformModifier _transform_modifier = {};
};

TES_ENUM_FLAGS(ShapeCache::ShapeFlag, unsigned);


inline void ShapeCache::const_iterator::next()
{
  while (_cursor != _end && (_cursor->flags & ShapeFlag::Pending) == ShapeFlag::Pending)
  {
    ++_cursor;
  }
  if (_cursor != _end)
  {
    _view = View{ _cursor->shape_id, _cursor->current, _cursor->child_count };
  }
  else
  {
    _view = {};
  }
}
}  // namespace tes::viewer::painter

#endif  // TES_VIEWER_SHAPE_CACHE_H
