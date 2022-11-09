#ifndef TES_VIEWER_SHAPE_CACHE_H
#define TES_VIEWER_SHAPE_CACHE_H

#include "3es-viewer.h"

#include "3esbounds.h"

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

namespace tes
{
class BoundsCuller;

class ShapeCacheShader
{
public:
  virtual ~ShapeCacheShader();

  virtual void setProjectionMatrix(const Magnum::Matrix4 &projection) = 0;

  virtual void draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer, size_t instance_count) = 0;
};

class ShapeCacheShaderFlat : public ShapeCacheShader
{
public:
  ShapeCacheShaderFlat();
  ~ShapeCacheShaderFlat();

  void setProjectionMatrix(const Magnum::Matrix4 &projection) override;

  void draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer, size_t instance_count) override;

private:
  Magnum::Shaders::Flat3D _shader;
};

class ShapeCacheShaderWireframe : public ShapeCacheShader
{
public:
  ShapeCacheShaderWireframe();
  ~ShapeCacheShaderWireframe();

  void setProjectionMatrix(const Magnum::Matrix4 &projection) override;

  void draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer, size_t instance_count) override;

private:
  Magnum::Shaders::Flat3D _shader;
};

class ShapeCache
{
public:
  enum class Type : unsigned
  {
    Solid,
    Transparent
  };

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

  /// The default implementation of a @c BoundsCalculator .
  /// @param transform The shape transformation matrix.
  /// @param[out] centre Set to the bounds centre.
  /// @param[out] half_extents Set to the bounds half extents vector.
  static void defaultCalcBounds(const Magnum::Matrix4 &transform, Magnum::Vector3 &centre,
                                Magnum::Vector3 &half_extents);

  static constexpr unsigned kFreeListEnd = ~0u;

  ShapeCache(Type type, std::shared_ptr<BoundsCuller> culler, Magnum::GL::Mesh &&mesh,
             const Magnum::Matrix4 &mesh_transform, const Magnum::Vector3 &half_extents,
             std::unique_ptr<ShapeCacheShaderFlat> &&shader,
             BoundsCalculator bounds_calculator = ShapeCache::defaultCalcBounds);

  void calcBounds(const Magnum::Matrix4 &transform, Magnum::Vector3 &centre, Magnum::Vector3 &half_extents);
  inline void setBoundsCalculator(BoundsCalculator bounds_calculator)
  {
    _bounds_calculator = std::move(bounds_calculator);
  }

  unsigned add(const Magnum::Matrix4 &transform, const Magnum::Color3 &colour);
  void remove(unsigned id);
  void update(unsigned id, const Magnum::Matrix4 &transform, const Magnum::Color3 &colour);

  void draw(unsigned render_mark, const Magnum::Matrix4 &projection_matrix);

private:
  struct ShapeInstance
  {
    Magnum::Matrix4 transform;
    Magnum::Color3 colour;
  };

  struct Shape
  {
    ShapeInstance instance;
    BoundsId bounds_id;
    unsigned free_next = kFreeListEnd;
  };

  struct InstanceBuffer
  {
    Magnum::GL::Buffer buffer{ Magnum::NoCreate };
    unsigned count = 0;
  };

  inline unsigned freeListNext(const Shape &shape) { return shape.free_next; }
  inline void setFreeListNext(Shape &shape, unsigned next) { shape.free_next = next; }

  void buildInstanceBuffers(unsigned render_mark);

  std::shared_ptr<BoundsCuller> _culler;
  std::vector<Shape> _shapes;
  unsigned _free_list = kFreeListEnd;
  Magnum::GL::Mesh _mesh;
  Magnum::Matrix4 _mesh_transform = {};
  Magnum::Vector3 _half_extents;
  /// Transformation matrix applied to the shape before rendering. This allows the Magnum primitives to be transformed
  /// to suit the 3rd Eye Scene rendering.
  std::vector<InstanceBuffer> _instance_buffers;
  std::array<ShapeInstance, 2048> _marshal_buffer;
  std::unique_ptr<ShapeCacheShaderFlat> _shader;
  BoundsCalculator _bounds_calculator = ShapeCache::defaultCalcBounds;
  Type _type;
};
}  // namespace tes

#endif  // TES_VIEWER_SHAPE_CACHE_H
