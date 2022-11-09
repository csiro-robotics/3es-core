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
  /// Bounds calculation function signature.
  using BoundsCalculator = ShapeCache::BoundsCalculator;

  /// Shape rendering type.
  enum class Type
  {
    Solid,        ///< Solid shape rendering.
    Wireframe,    ///< Wireframe or line based rendering.
    Transparent,  ///< Transparent shape rendering (triangles).
  };

  /// Construct a shape painter.
  /// @param culler The @c BoundsCuller used for visibility checking.
  /// @param solid Mesh used for solid rendering.
  /// @param wireframe Mesh used for wireframe rendering (line based).
  /// @param transparent Mesh used for transparent rendering.
  /// @param mesh_transform Additional transformation applied to a mesh after instance transforms.
  /// @param bounds_calculator Bounds calculation function.
  ShapePainter(std::shared_ptr<BoundsCuller> culler, Magnum::GL::Mesh &&solid, Magnum::GL::Mesh &&wireframe,
               Magnum::GL::Mesh &&transparent, const Magnum::Matrix4 &mesh_transform,
               BoundsCalculator bounds_calculator);
  /// Destructor.
  ~ShapePainter();

  /// Add a shape with the given @p id to paint.
  /// @param id The object @c Id for the shape. A zero @c Id is a transient shape and is removed between draw calls.
  /// @param type The draw type for the shape.
  /// @param transform The shape transformation.
  /// @param colour The shape colour.
  void add(const Id &id, Type type, const Magnum::Matrix4 &transform, const Magnum::Color4 &colour);

  /// Update an existing shape (non transient).
  ///
  /// This identifies the @c Type based on the @c Id .
  ///
  /// @param id The @c Id of the shape to update.
  /// @param transform The new shape transformation.
  /// @param colour The new shape colour.
  /// @return True if the @p id can be resolved and the shape updated.
  bool update(const Id &id, const Magnum::Matrix4 &transform, const Magnum::Color4 &colour);

  /// Remove a shape by @c Id .
  ///
  /// This identifies the @c Type based on the @c Id .
  ///
  /// @param id The @c Id of the shape to remove.
  /// @return True if the @p id can be resolved and the shape removed.
  bool remove(const Id &id);

  /// Render the current opaque (solid & wireframe) shapes set.
  /// @param render_mark
  /// @param projection_matrix The view projection matrix.
  void drawOpaque(unsigned render_mark, const Magnum::Matrix4 &projection_matrix);

  /// Render the current transparent shapes set.
  /// @param render_mark
  /// @param projection_matrix The view projection matrix.
  void drawTransparent(unsigned render_mark, const Magnum::Matrix4 &projection_matrix);

  /// Remove all the current transient objects.
  void endFrame();

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

  ShapeCache *cacheForType(Type type);

  /// Solid shape rendering cache.
  std::unique_ptr<ShapeCache> _solid_cache;
  /// Wireframe shape rendering cache.
  std::unique_ptr<ShapeCache> _wireframe_cache;
  /// Transparent shape rendering cache.
  std::unique_ptr<ShapeCache> _transparent_cache;
  /// Set of current transient shapes to remove on the next @c endFrame() call.
  std::vector<unsigned> _solid_transients;
  /// Set of current transient shapes to remove on the next @c endFrame() call.
  std::vector<unsigned> _wireframe_transients;
  /// Set of current transient shapes to remove on the next @c endFrame() call.
  std::vector<unsigned> _transparent_transients;
  /// Maps 3es @p Id to a draw type and index in the associated @c ShapeCache .
  IdIndexMap _id_index_map;
};
}  // namespace tes::viewer::painter

#endif  // TES_VIEWER_PAINTER_SHAPE_PAINTER_H
