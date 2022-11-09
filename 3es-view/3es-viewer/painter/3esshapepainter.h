#include "3es-viewer.h"

#include "3esshapecache.h"

#include <shapes/3esid.h>

#include <Magnum/GL/Mesh.h>

#include <memory>
#include <unordered_map>

namespace tes::viewer::painter
{
class ShapePainter
{
public:
  using BoundsCalculator = ShapeCache::BoundsCalculator;

  enum class Type
  {
    Solid,
    Wireframe,
    Transparent,
  };

  ShapePainter(std::shared_ptr<BoundsCuller> culler, Magnum::GL::Mesh &&solid, Magnum::GL::Mesh &&wireframe,
               Magnum::GL::Mesh &&transparent, const Magnum::Matrix4 &mesh_transform,
               BoundsCalculator bounds_calculator);
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

  /// Render the current shapes set.
  /// @param render_mark
  /// @param projection_matrix The view projection matrix.
  void draw(unsigned render_mark, const Magnum::Matrix4 &projection_matrix);

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

  std::unique_ptr<ShapeCache> _solid_cache;
  std::unique_ptr<ShapeCache> _wireframe_cache;
  std::unique_ptr<ShapeCache> _transparent_cache;
  /// Set of current transient shapes to remove on the next @c endFrame() call.
  std::vector<unsigned> _solid_transients;
  /// Set of current transient shapes to remove on the next @c endFrame() call.
  std::vector<unsigned> _wireframe_transients;
  /// Set of current transient shapes to remove on the next @c endFrame() call.
  std::vector<unsigned> _transparent_transients;
  IdIndexMap _id_index_map;
};
}  // namespace tes::viewer::painter
