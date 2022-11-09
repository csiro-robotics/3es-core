#ifndef TES_VIEWER_SHAPE_CACHE_H
#define TES_VIEWER_SHAPE_CACHE_H

#include "3es-viewer.h"

#include "3esboundsculler.h"

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

namespace tes::viewer::painter
{
/// A shader abstraction used with a @c ShapeCache .
///
/// This abstracts away the details of the shader for the @c ShapeCache such that it simply needs to call
/// @c setProjectionMatrix() and @c draw() .
class ShapeCacheShader
{
public:
  /// Virtual destructor.
  virtual ~ShapeCacheShader();

  /// Set the projection matrix for the next @c draw() call.
  /// @param projection The next projection matrix to draw with.
  virtual void setProjectionMatrix(const Magnum::Matrix4 &projection) = 0;

  /// Draw the @p mesh with this shader with shape instances from @p buffer .
  ///
  /// May be called multiple times for each frame with only one call to @c setProjectionMatrix() in between.
  ///
  /// @param mesh The mesh to draw.
  /// @param buffer The shape instance buffer or @c Magnum::Matrix4 and @c Magnum::Color4 pairs per instance.
  /// @param instance_count Number of instances in @p buffer .
  virtual void draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer, size_t instance_count) = 0;
};

/// Flat colour shader for a @p ShapeCache . Can be used for solid, transparent and line based shapes.
class ShapeCacheShaderFlat : public ShapeCacheShader
{
public:
  /// Constructor.
  ShapeCacheShaderFlat();
  /// Destructor.
  ~ShapeCacheShaderFlat();

  void setProjectionMatrix(const Magnum::Matrix4 &projection) override;

  void draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer, size_t instance_count) override;

private:
  /// Internal shader.
  Magnum::Shaders::Flat3D _shader;
};

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
class ShapeCache
{
public:
  /// Helper function used to implement @c calcBounds() for the cached shape type.
  ///
  /// The calculation must vary depending on the shape type. For spheres, for example, the bounds are constant and the
  /// default implementation is used, where the @p transform scale is mapped to @p half_extents . Other shapes need
  /// to consider the effects of rotation.
  ///
  /// @param transform The shape transformation matrix.
  /// @param[out] centre Set to the bounds centre.
  /// @param[out] half_extents Set to the bounds half extents vector.
  using BoundsCalculator =
    std::function<void(const Magnum::Matrix4 &transform, Magnum::Vector3 &centre, Magnum::Vector3 &half_extents)>;

  /// A mesh and transform part for use with the @c ShapeCache .
  ///
  /// A @c ShapeCache can have one or more @c Part objects to render. Each mesh is rendered by first applying an
  /// instance transform, then the @c Part::transform then the projection matrix thusly;
  ///
  /// @code{.unparsed}
  ///   transform = projection_matrix * part.transform * instance_transform;
  /// @endcode
  struct Part
  {
    /// Mesh shared pointer; must not be null.
    std::shared_ptr<Magnum::GL::Mesh> mesh;
    /// Transform to apply to @c mesh before rendering.
    Magnum::Matrix4 transform = {};

    inline Part() = default;
    inline Part(const Part &other) = default;
    inline Part(Part &&other) = default;
    explicit inline Part(std::shared_ptr<Magnum::GL::Mesh> mesh)
      : Part(std::move(mesh), Magnum::Matrix4())
    {}
    inline Part(std::shared_ptr<Magnum::GL::Mesh> mesh, const Magnum::Matrix4 &transform)
      : mesh(std::exchange(mesh, nullptr))
      , transform(transform)
    {}
    explicit inline Part(Magnum::GL::Mesh &&mesh)
      : Part(std::make_shared<Magnum::GL::Mesh>(std::move(mesh)), Magnum::Matrix4())
    {}
    inline Part(Magnum::GL::Mesh &&mesh, const Magnum::Matrix4 &transform)
      : Part(std::make_shared<Magnum::GL::Mesh>(std::move(mesh)), transform)
    {}

    inline Part &operator=(const Part &other) = default;
    inline Part &operator=(Part &&other) = default;
  };


  /// The default implementation of a @c BoundsCalculator , which is a cube with 2m sides scaled by the @p transform.
  /// @param transform The shape transformation matrix.
  /// @param[out] centre Set to the bounds centre.
  /// @param[out] half_extents Set to the bounds half extents vector.
  static void defaultCalcBounds(const Magnum::Matrix4 &transform, Magnum::Vector3 &centre,
                                Magnum::Vector3 &half_extents);

  /// Internal free list terminator value.
  static constexpr unsigned kFreeListEnd = ~0u;

  /// @overload
  ShapeCache(std::shared_ptr<BoundsCuller> culler, const Part &part,
             std::unique_ptr<ShapeCacheShaderFlat> &&shader = std::make_unique<ShapeCacheShaderFlat>(),
             BoundsCalculator bounds_calculator = ShapeCache::defaultCalcBounds);

  /// Construct a shape cache.
  /// @param culler The bounds culler used to create bounds and manage bounds.
  /// @param parts The mesh parts to render. An overload accepts a single @c Part .
  /// @param shader The shader used to draw the mesh.
  /// @param bounds_calculator Bounds calculation function.
  ShapeCache(std::shared_ptr<BoundsCuller> culler, const std::vector<Part> &parts,
             std::unique_ptr<ShapeCacheShaderFlat> &&shader = std::make_unique<ShapeCacheShaderFlat>(),
             BoundsCalculator bounds_calculator = ShapeCache::defaultCalcBounds);

  /// @overload
  ShapeCache(std::shared_ptr<BoundsCuller> culler, std::initializer_list<Part> parts,
             std::unique_ptr<ShapeCacheShaderFlat> &&shader = std::make_unique<ShapeCacheShaderFlat>(),
             BoundsCalculator bounds_calculator = ShapeCache::defaultCalcBounds);

  /// Calculate the bounds for a shape instance with the given transform.
  /// @param transform The shape instance transformation matrix.
  /// @param[out] centre Calculated bounds centre.
  /// @param[out] half_extents Calculated bounds half extents.
  void calcBounds(const Magnum::Matrix4 &transform, Magnum::Vector3 &centre, Magnum::Vector3 &half_extents);

  /// Set the bounds calculation function.
  /// @param bounds_calculator New bounds calculation function.
  inline void setBoundsCalculator(BoundsCalculator bounds_calculator)
  {
    _bounds_calculator = std::move(bounds_calculator);
  }

  /// Add a shape instance.
  /// @param transform The shape instance transformation.
  /// @param colour The shape instance colour.
  /// @return The shape ID/index. Must be used to @c remove() or @c update() the shape.
  unsigned add(const Magnum::Matrix4 &transform, const Magnum::Color4 &colour);
  /// Remove the shape with the given @p id .
  /// @param id Id of the shape to remove.
  /// @return True if the @p id was valid and an instance removed.
  bool remove(unsigned id);
  /// Update an existing shape instance.
  /// @param id Id of the shape to update.
  /// @param transform The shape instance transformation.
  /// @param colour The shape instance colour.
  /// @return True if the @p id was valid and an instance updated.
  bool update(unsigned id, const Magnum::Matrix4 &transform, const Magnum::Color4 &colour);

  /// Get the details of an existing shape instance.
  /// @param id Id of the shape to update.
  /// @param[out] transform Set to the shape instance transformation.
  /// @param[out] colour Set to the shape instance colour.
  /// @return True if the @p id was valid and an instance data retrieved. The out values are undefined when @p id is
  /// invalid.
  bool get(unsigned id, Magnum::Matrix4 &transform, Magnum::Color4 &colour) const;

  /// Clear the shape cache, removing all shapes.
  ///
  /// @note Bounds are returned to the @c BoundsCuller iteratively.
  void clear();

  /// Draw all shape instances considered visible by the @p render_mark .
  ///
  /// Before calling this function, the @c BoundsCuller::cull() should be called with the same @p render_mark , which
  /// ensure the bounds entries are marked as visibly for the @p render_mark .
  ///
  /// @param render_mark The render visibility mark.
  /// @param projection_matrix World to projection matrix.
  void draw(unsigned render_mark, const Magnum::Matrix4 &projection_matrix);

private:
  /// Shape instance data.
  struct ShapeInstance
  {
    /// The instance transformation matrix.
    Magnum::Matrix4 transform;
    /// The instance colour.
    Magnum::Color4 colour;
  };

  /// An entry in the shape cache.
  struct Shape
  {
    /// Shape entry instance data.
    ShapeInstance instance;
    /// The shape entry @c BoundsCuller entry ID.
    BoundsId bounds_id;
    /// Free list (linked list) next item ID. This value is @c kFreeListEnd when not on the free list. Otherwise it
    /// is set to the index of the next entry in the free list.
    unsigned free_next = kFreeListEnd;
  };

  /// Instance buffer used to render shapes. Only valid during the @c draw() call.
  struct InstanceBuffer
  {
    /// Graphics buffer to which shape instances are marshalled.
    Magnum::GL::Buffer buffer{ Magnum::NoCreate };
    /// Number of items in the buffer.
    unsigned count = 0;
  };

  /// Fill the @p InstanceBuffer objects in @c _instance_buffers .
  /// @param render_mark Visibility render mark used to determine shape instance visibility in the @c BoundsCuller .
  void buildInstanceBuffers(unsigned render_mark);

  /// The bounds culler used to determine visibility.
  std::shared_ptr<BoundsCuller> _culler;
  /// Shape instance array.
  std::vector<Shape> _shapes;
  /// Free list head terminated by a value of @c kFreeListEnd .
  unsigned _free_list = kFreeListEnd;
  /// Mesh parts to render.
  std::vector<Part> _parts;
  /// Transformation matrix applied to the shape before rendering. This allows the Magnum primitives to be transformed
  /// to suit the 3rd Eye Scene rendering.
  std::vector<InstanceBuffer> _instance_buffers;
  /// Buffer used to marshal active shape instances in @c buildInstanceBuffers() . The size of this array determines
  /// the number of instances per @p InstanceBuffer .
  std::array<ShapeInstance, 2048> _marshal_buffer;
  /// Shaper used to draw the shapes.
  std::unique_ptr<ShapeCacheShaderFlat> _shader;
  /// Bounds calculation function.
  BoundsCalculator _bounds_calculator = ShapeCache::defaultCalcBounds;
};
}  // namespace tes::viewer::painter

#endif  // TES_VIEWER_SHAPE_CACHE_H
