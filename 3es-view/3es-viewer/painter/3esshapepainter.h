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

  /// Shape rendering type.
  enum class Type
  {
    Solid,        ///< Solid shape rendering.
    Wireframe,    ///< Wireframe or line based rendering.
    Transparent,  ///< Transparent shape rendering (triangles).
  };

  /// An id returned from @p add() which can be passed to @c addSubShape() to create child shapes.
  class ParentId
  {
  public:
    inline explicit ParentId(const unsigned id)
      : _id(id)
    {}
    ParentId() = default;
    ParentId(const ParentId &other) = default;
    ParentId(ParentId &&other) = default;

    /// Internal ID value.
    /// @return Internal value.
    inline unsigned id() const { return _id; }

  private:
    unsigned _id = ~0u;
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
  /// @param id The object @c Id for the shape. A zero @c Id is a transient shape and is removed between draw calls.
  /// @param frame_number The frame at which the shape becomes visible.
  /// @param type The draw type for the shape.
  /// @param transform The shape transformation.
  /// @param colour The shape colour.
  /// @return An id value which can be passed to @c addSubShape() to add child shapes. This is transient and should
  /// only be used immediately after calling @c add() to call @c addSubShape() .
  virtual ParentId add(const Id &id, FrameNumber frame_number, Type type, const Magnum::Matrix4 &transform,
                       const Magnum::Color4 &colour);

  /// Add a sub shape part.
  ///
  /// This supports multi-shape messages where multiple shapes are part of the same @p id . Shapes added with
  /// @c addSubShape() are essentially children of the first shape in a scene hierarchy sense, and the primary shape
  /// transform also affects children. Removing a shape also removes its sub shapes.
  ///
  /// To add a sub shape, first call @p add() with a new @p id , then call @c addSubShape() for each sub/child shape.
  /// Remember passing an identity @p transform for a sub shape co-locates the sub shape with the first shape.
  ///
  /// @param parent_id The parent id obtained from @c add() .
  /// @param frame_number The frame at which the shape becomes visible.
  /// @param type The draw type for the shape.
  /// @param transform The shape transformation.
  /// @param colour The shape colour.
  virtual void addSubShape(const ParentId &parent_id, FrameNumber frame_number, Type type,
                           const Magnum::Matrix4 &transform, const Magnum::Color4 &colour);

  /// Update an existing shape (non transient).
  ///
  /// This identifies the @c Type based on the @c Id .
  ///
  /// @param id The @c Id of the shape to update.
  /// @param frame_number The frame at which the shape update takes effect.
  /// @param transform The new shape transformation.
  /// @param colour The new shape colour.
  /// @return True if the @p id can be resolved and the shape updated.
  virtual bool update(const Id &id, FrameNumber frame_number, const Magnum::Matrix4 &transform,
                      const Magnum::Color4 &colour);

  /// Read the current proeprties for a shape instance.
  /// @param id Shape id of interest.
  /// @param frame_number The frame at which to get the properties.
  /// @param include_parent_transform True to have @p transform include the parent's transform.
  /// @param[out] transform Transform output. Does not include any parent transform.
  /// @param[out] colour Colour output.
  /// @return True if @p id is valid.
  virtual bool readProperties(const Id &id, FrameNumber frame_number, bool include_parent_transform,
                              Magnum::Matrix4 &transform, Magnum::Color4 &colour) const;
  /// overload
  inline bool readProperties(const Id &id, FrameNumber frame_number, Magnum::Matrix4 &transform,
                             Magnum::Color4 &colour) const
  {
    return readProperties(id, frame_number, false, transform, colour);
  }

  /// Remove a shape by @c Id .
  ///
  /// This identifies the @c Type based on the @c Id .
  ///
  /// @param id The @c Id of the shape to remove.
  /// @param frame_number The frame number at which the shape is no longer visible.
  /// @return True if the @p id can be resolved and the shape removed.
  virtual bool remove(const Id &id, FrameNumber frame_number);

  /// Render the current opaque (solid & wireframe) shapes set.
  /// @param stamp The frame stamp to draw at.
  /// @param projection_matrix The view projection matrix.
  virtual void drawOpaque(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix);

  /// Render the current transparent shapes set.
  /// @param stamp The frame stamp to draw at.
  /// @param projection_matrix The view projection matrix.
  virtual void drawTransparent(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix);

  /// Remove all the current transient objects.
  /// @param frame_number The number of the frame ending.
  void endFrame(FrameNumber frame_number);

protected:
  /// Identifies a shape type and index into the associated @c ShapeCache .
  struct CacheIndex
  {
    Type type = {};
    unsigned index = {};
  };

  /// Mapping of shape @c Id to @c ShapeCache index.
  /// @todo Use a different map; MSVC @c std::unordered_map performance is terrible.
  using IdIndexMap = std::unordered_map<Id, CacheIndex>;

  virtual unsigned addShape(const ViewableWindow &view_window, Type type, const Magnum::Matrix4 &transform,
                            const Magnum::Color4 &colour, const ParentId &parent_id = ParentId());

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
};
}  // namespace tes::viewer::painter

#endif  // TES_VIEWER_PAINTER_SHAPE_PAINTER_H
