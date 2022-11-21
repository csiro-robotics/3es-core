#ifndef TES_VIEWER_PAINTER_SHAPE_PAINTER_H
#define TES_VIEWER_PAINTER_SHAPE_PAINTER_H

#include "3es-viewer.h"

#include "3esshapecache.h"

#include <shapes/3esid.h>

#include <Magnum/GL/Mesh.h>

#include <memory>
#include <unordered_map>

namespace tes::viewer::painter
{
/// A @c ShapePainter renders a single primitive shape type in either solid, wireframe or transparent forms. The
/// painter also associated 3rd Eye Scene shape @c Id objects with renderable objects. The painter effects the @c Id
/// semantics, with zero value ids representing transient shapes, removed when @c endFrame() is called.
///
/// The painter is supported by the @c ShapeCache class, one instance for each drawing @c Type . As such it has similar
/// supporting requirements; a @c BoundsCuller , @c Mesh objects for solid, wireframe and transparent rendering and a
/// bounds calculation function.
class ShapePainter
{
public:
  /// Part alias from @c ShapeCache .
  using Part = ShapeCache::Part;
  /// Bounds calculation function signature.
  using BoundsCalculator = ShapeCache::BoundsCalculator;

  class const_iterator;

  /// Shape rendering type.
  enum class Type : int
  {
    Solid,        ///< Solid shape rendering.
    Wireframe,    ///< Wireframe or line based rendering.
    Transparent,  ///< Transparent shape rendering (triangles).
    // Note: reordering this breaks const_iterator
  };

  /// An id returned from @p add() which can be passed to @c addChild() to create child shapes.
  class ParentId
  {
  public:
    inline explicit ParentId(const Id &shape_id, const util::ResourceListId resource_id)
      : _shape_id(shape_id)
      , _resource_id(resource_id)
    {}
    ParentId() = default;
    ParentId(const ParentId &other) = default;
    ParentId(ParentId &&other) = default;

    inline Id shapeId() const { return _shape_id; }

    /// Internal ID value.
    /// @return Internal value.
    inline util::ResourceListId resourceId() const { return _resource_id; }

  private:
    Id _shape_id;
    util::ResourceListId _resource_id = util::kNullResource;
  };

  /// A child or sub-shape identifier.
  class ChildId
  {
  public:
    inline ChildId(const Id shape_id, unsigned child_index)
      : _shape_id(shape_id)
      , _index(child_index)
    {}

    ChildId() = default;
    ChildId(const ChildId &other) = default;
    ChildId(ChildId &&other) = default;

    inline Id shapeId() const { return _shape_id; }
    inline unsigned index() const { return _index; }

  private:
    /// The primary shape id.
    Id _shape_id;
    /// The child resource id (in some ways the true id).
    unsigned _index = 0;
  };

  /// Construct a shape painter.
  /// @param culler The @c BoundsCuller used for visibility checking.
  /// @param solid Mesh used for solid rendering.
  /// @param wireframe Mesh used for wireframe rendering (line based).
  /// @param transparent Mesh used for transparent rendering.
  /// @param bounds_calculator Bounds calculation function.
  ShapePainter(std::shared_ptr<BoundsCuller> culler, std::initializer_list<Part> solid,
               std::initializer_list<Part> wireframe, std::initializer_list<Part> transparent,
               BoundsCalculator bounds_calculator);
  /// @overload
  ShapePainter(std::shared_ptr<BoundsCuller> culler, const std::vector<Part> &solid, const std::vector<Part> &wireframe,
               const std::vector<Part> &transparent, BoundsCalculator bounds_calculator);
  /// Destructor.
  ~ShapePainter();

  /// Clear the painter, removing all shapes.
  virtual void reset();

  /// Add a shape with the given @p id to paint.
  ///
  /// This change is not effected util the next @c commit() call.
  /// @param id The object @c Id for the shape. A zero @c Id is a transient shape and is removed between draw calls.
  /// @param frame_number The frame at which the shape becomes visible.
  /// @param type The draw type for the shape.
  /// @param transform The shape transformation.
  /// @param colour The shape colour.
  /// @return An id value which can be passed to @c addChild() to add child shapes. This is transient and should
  /// only be used immediately after calling @c add() to call @c addChild() .
  virtual ParentId add(const Id &id, Type type, const Magnum::Matrix4 &transform, const Magnum::Color4 &colour);

  /// Add a sub shape part.
  ///
  /// This supports multi-shape messages where multiple shapes are part of the same @p id . Shapes added with
  /// @c addChild() are essentially children of the first shape in a scene hierarchy sense, and the primary shape
  /// transform also affects children. Removing a shape also removes its sub shapes.
  ///
  /// To add a sub shape, first call @p add() with a new @p id , then call @c addChild() for each sub/child shape.
  /// Remember passing an identity @p transform for a sub shape co-locates the sub shape with the first shape.
  ///
  /// This change is not effected util the next @c commit() call.
  /// @param parent_id The parent id obtained from @c add() .
  /// @param type The draw type for the shape.
  /// @param transform The shape transformation.
  /// @param colour The shape colour.
  virtual ChildId addChild(const ParentId &parent_id, Type type, const Magnum::Matrix4 &transform,
                           const Magnum::Color4 &colour);

  /// Update an existing shape (non transient).
  ///
  /// This identifies the @c Type based on the @c Id .
  ///
  /// This change is not effected util the next @c commit() call.
  /// @param id The @c Id of the shape to update.
  /// @param transform The new shape transformation.
  /// @param colour The new shape colour.
  /// @return True if the @p id can be resolved and the shape updated.
  virtual bool update(const Id &id, const Magnum::Matrix4 &transform, const Magnum::Color4 &colour);

  virtual bool updateChildShape(const ChildId &child_id, const Magnum::Matrix4 &transform,
                                const Magnum::Color4 &colour);

  /// Remove a shape by @c Id .
  ///
  /// This identifies the @c Type based on the @c Id .
  ///
  /// This change is not effected util the next @c commit() call.
  /// @param id The @c Id of the shape to remove.
  /// @return True if the @p id can be resolved, and the shape removed.
  virtual bool remove(const Id &id);

  /// Read the current properties for a shape instance as of the last @c commit().
  /// @param id Shape id of interest.
  /// @param[out] transform Transform output. Does not include any parent transform.
  /// @param[out] colour Colour output.
  /// @return True if @p id is valid.
  virtual bool readShape(const Id &id, Magnum::Matrix4 &transform, Magnum::Color4 &colour) const;

  /// Read the current properties for a shape instance as of the last @c commit().
  /// @param id Shape id of interest.
  /// @param include_parent_transform True to have @p transform include the parent's transform.
  /// @param[out] transform Transform output. Does not include any parent transform.
  /// @param[out] colour Colour output.
  /// @return True if @p id is valid.
  virtual bool readChildShape(const ChildId &child_id, bool include_parent_transform, Magnum::Matrix4 &transform,
                              Magnum::Color4 &colour) const;

  /// Render the current opaque (solid & wireframe) shapes set.
  /// @param stamp The frame stamp to draw at.
  /// @param projection_matrix The view projection matrix.
  virtual void drawOpaque(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix);

  /// Render the current transparent shapes set.
  /// @param stamp The frame stamp to draw at.
  /// @param projection_matrix The view projection matrix.
  virtual void drawTransparent(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix);

  /// Commit the pending changes.
  ///
  /// This removes the current transient objects, then effects changes from the following function calls:
  /// - @c add()
  /// - @c addChild()
  /// - @c update ()
  /// - @c updateChild()
  /// - @c remove()
  virtual void commit();

  /// An iterator for the shapes in the painter. Only iterates one shape @c Type at a time.
  ///
  /// Contents are read only and provide a @c View to the shape. For shapes with a @c child_count, use
  /// @c getChild() to iterate the children.
  class const_iterator
  {
  public:
    /// An external view of a shape.
    struct View
    {
      Id id;
      /// The instance transformation matrix.
      Magnum::Matrix4 transform = {};
      /// The instance colour.
      Magnum::Color4 colour = {};
      /// Number of children this shape has.
      unsigned child_count = 0;
    };

    /// Default constructor.
    const_iterator();
    /// Iteration constructor
    /// @param cache_type The type being iterated.
    /// @param cache The shape to iterate.
    /// @param begin True to begin iterator, false to create an @c end iterator.
    const_iterator(Type cache_type, const ShapeCache *cache, bool begin = true)
      : _cache_type(cache_type)
      , _cache(cache)
    {
      if (_cache)
      {
        _end = _cache->end();
        if (begin)
        {
          _cursor = _cache->begin();
        }
        else
        {
          _cursor = _end;
        }
      }
    }
    /// Copy constructor.
    /// @param other Iterator to copy.
    inline const_iterator(const const_iterator &other) = default;
    /// Copy constructor.
    /// @param other Iterator to move.
    inline const_iterator(const_iterator &&other) = default;

    /// Copy assignment operator.
    /// @param other Iterator to copy.
    /// @return @c *this
    inline const_iterator &operator=(const const_iterator &other) = default;
    /// Move assignment operator.
    /// @param other Iterator to move.
    /// @return @c *this
    inline const_iterator &operator=(const_iterator &&other) = default;

    /// Equality test. Only compares the cursor.
    /// @param other Iterator to compare.
    /// @return True if the iterators are semantically equivalent.
    inline bool operator==(const const_iterator &other) const
    {
      return _cache == other._cache && _cache_type == other._cache_type && _cursor == other._cursor;
    }
    /// Inequality test. Only compares the cursor.
    /// @param other Iterator to compare.
    /// @return True unless the iterators are semantically equivalent.
    inline bool operator!=(const const_iterator &other) const { return !operator==(other); }

    /// Prefix increment
    /// @return @c *this
    inline const_iterator &operator++()
    {
      next();
      return *this;
    }

    /// Postfix increment
    /// @return An iterator before incrementing.
    inline const_iterator operator++(int)
    {
      auto iter = *this;
      next();
      return iter;
    }

    /// Dereference to a @c View.
    /// @return A @c View to the current item.
    inline const View &operator*() const { return _view; }
    /// Dereference to a @c View.
    /// @return A @c View to the current item.
    inline const View *operator->() const { return &_view; }

    /// Get a @c View to the child at @p child_index.
    /// @param child_index The index of the child to retrieve.
    /// @return The child @c View.
    View getChild(unsigned child_index) const
    {
      View view = {};
      view.id = _view.id;
      view.child_count = 0;
      auto child_rid = _cache->getChildId(_cursor.rid(), child_index);
      if (child_rid != util::kNullResource)
      {
        _cache->get(child_rid, false, view.transform, view.colour);
      }
      return view;
    }

  protected:
    /// Iterate to the next item.
    void next()
    {
      ++_cursor;
      if (_cursor != _end)
      {
        _view = { _cursor->id, _cursor->attributes.transform, _cursor->attributes.colour };
      }
      else
      {
        _view = {};
      }
    }

  private:
    ShapeCache::const_iterator _cursor;
    ShapeCache::const_iterator _end;
    const ShapeCache *_cache = nullptr;
    Type _cache_type = Type::Solid;
    View _view = {};
  };

  /// Begin iteration of the shapes of @p type.
  /// @param type The @c Type of shape to iterate.
  /// @return The starting iterator.
  inline const_iterator begin(Type type) const { return const_iterator(type, cacheForType(type), true); }
  /// End iterator for shapes of @p type.
  /// @param type The @c Type of shape to iterate.
  /// @return The end iterator.
  inline const_iterator end(Type type) const { return const_iterator(type, cacheForType(type), false); }

protected:
  /// Identifies a shape type and index into the associated @c ShapeCache .
  struct CacheIndex
  {
    Type type = {};
    util::ResourceListId index = {};
  };

  /// Mapping of shape @c Id to @c ShapeCache index.
  /// @todo Use a different map; MSVC @c std::unordered_map performance is terrible.
  using IdIndexMap = std::unordered_map<Id, CacheIndex>;

  virtual util::ResourceListId addShape(const Id &shape_id, Type type, const Magnum::Matrix4 &transform,
                                        const Magnum::Color4 &colour, const ParentId &parent_id = ParentId(),
                                        unsigned *child_index = nullptr);

  ShapeCache *cacheForType(Type type);
  const ShapeCache *cacheForType(Type type) const;

  /// Solid shape rendering cache.
  std::unique_ptr<ShapeCache> _solid_cache;
  /// Wireframe shape rendering cache.
  std::unique_ptr<ShapeCache> _wireframe_cache;
  /// Transparent shape rendering cache.
  std::unique_ptr<ShapeCache> _transparent_cache;
  /// Maps 3es @p Id to a draw type and index in the associated @c ShapeCache .
  IdIndexMap _id_index_map;
  /// Ids pending removal on next @c commit() .
  std::vector<Id> _pending_removal;
};
}  // namespace tes::viewer::painter

#endif  // TES_VIEWER_PAINTER_SHAPE_PAINTER_H
