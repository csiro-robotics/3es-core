//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SHAPES_POINTS_H
#define TES_CORE_SHAPES_POINTS_H

#include <3escore/CoreConfig.h>

#include "MeshSet.h"
#include "Shape.h"

//
#include <3escore/IntArg.h>

namespace tes
{
/// A @c Shape which renders a set of points as in a point cloud.
///
/// The points are contained in a @c MeshResource (e.g., @c PointCloud)
/// and may be shared between @c PointCloudShape shapes. The @c MeshResource should
/// have a @c MeshResource.drawType() of @c DtPoints or the behaviour may
/// be undefined.
///
/// The @c PointCloudShape shape supports limiting the view into the @c MeshResource
/// by having its own set of indices (see @c setIndices()).
///
/// @deprecated This shape has been deprecated. Use @c MeshSet with the @c PointCloud specialisation
/// of @c MeshResource or use a @c MeshShape with @c DtPoints rendering. This has been deprecated
/// because of the amount of work required by the viewer in order to duplicate resources with
/// a different set of indices and the functionality is sufficiently covered by the aforementioned
/// alternative classes.
class TES_CORE_API TES_CORE_DEPRECATED PointCloudShape : public Shape
{
public:
  /// Construct a point cloud shape object.
  /// mesh The mesh resource to render point data from. See class comments.
  /// @param id The shape ID, unique among @c Arrow objects, or zero for a transient shape.
  /// @param category The category grouping for the shape used for filtering.
  /// @param point_scale Desired point render scale. Use zero or one for the default scale.
  PointCloudShape(const MeshResource *mesh = nullptr, const Id &id = Id(),
                  float point_scale = 0.0f);

  /// Destructor.
  ~PointCloudShape() override;

  [[nodiscard]] const char *type() const override { return "pointCloudShape"; }

  /// Colour points by height.
  ///
  /// This sets the shape colour to zero (black, with zero alpha).
  /// @param colour_by_height True to colour by height.
  PointCloudShape &setColourByHeight(bool colour_by_height);

  /// Check if colouring points by height.
  /// @return True if set to colour by height.
  [[nodiscard]] bool colourByHeight() const;

  /// Set the desired point render scale. Zero or one for default.
  /// @param size The desired point scale for
  /// @return @c *this
  PointCloudShape &setPointScale(float scale)
  {
    _point_scale = scale;
    return *this;
  }
  /// Get the point render scale.
  /// @return The current point render scale.
  [[nodiscard]] float pointScale() const { return _point_scale; }

  /// Return the number of @c indices().
  ///
  /// Only non-zero when referencing a subset of @c mesh() vertices.
  ///
  /// @return Zero when using all @c mesh() vertices, non-zero when referencing a subset of @c
  /// mesh().
  [[nodiscard]] unsigned indexCount() const { return _index_count; }

  /// Return the index array when a subset of @c mesh() vertices.
  ///
  /// Indices are only set when overriding indexing from @c mesh().
  ///
  /// @return An array of indices, length @c indexCount(), or null when referencing all vertices
  /// from @c mesh().
  [[nodiscard]] const unsigned *indices() const { return _indices; }

  /// Sets the (optional) indices for this @c PointCloudShape @c Shape.
  /// This shape will only visualise the indexed points from its @c PointSource.
  /// This allows multiple @c PointCloudShape shapes to reference the same cloud, but reveal
  /// sub-sets of the cloud.
  ///
  /// This method is designed to copy any iterable sequence between @p begin and @p end,
  /// however the number of elements must be provided in @p index_count.
  ///
  /// @tparam I An iterable item. Must support dereferencing to an unsigned integer and
  ///   an increment operator.
  /// @param iter The index iterator.
  /// @param index_count The number of elements to copy from @p iter.
  /// @return This.
  template <typename I>
  PointCloudShape &setIndices(I iter, const UIntArg &index_count);

  /// Get the mesh resource containing the point data to render.
  /// @return The point cloud mesh resource.
  [[nodiscard]] const MeshResource *mesh() const { return _mesh; }

  /// Writes the standard create message and appends the point cloud ID (@c uint32_t).
  /// @param stream The stream to write to.
  /// @return True on success.
  bool writeCreate(PacketWriter &stream) const override;

  /// Write index data set in @c setIndices() if any.
  /// @param stream The data stream to write to.
  /// @param[in,out] progress_marker Indicates data transfer progress.
  ///   Initially zero, the @c Shape manages its own semantics.
  /// @return Indicates completion progress. 0 indicates completion,
  ///   1 indicates more data are available and more calls should be made.
  ///   -1 indicates an error. No more calls should be made.
  int writeData(PacketWriter &stream, unsigned &progress_marker) const override;

  bool readCreate(PacketReader &stream) override;

  bool readData(PacketReader &stream) override;

  /// Defines this class as a complex shape. See Shape::isComplex().
  /// @return @c true
  [[nodiscard]] bool isComplex() const override { return true; }

  /// Enumerates the mesh resource given on construction. See @c Shape::enumerateResources().
  /// @param resources Resource output array.
  /// @param capacity of @p resources.
  /// @param fetch_offset Indexing offset for the resources in this object.
  unsigned enumerateResources(const Resource **resources, unsigned capacity,
                              unsigned fetch_offset) const override;

  /// Deep copy clone. The source is only cloned if @c ownSource() is true.
  /// It is shared otherwise.
  /// @return A deep copy.
  [[nodiscard]] Shape *clone() const override;

private:
  void onClone(PointCloudShape *copy) const;

  /// Reallocate the index array preserving current data.
  /// @param count The new element size for the array.
  void reallocateIndices(uint32_t count);
  [[nodiscard]] static uint32_t *allocateIndices(uint32_t count);
  static void freeIndices(const uint32_t *indices);

  const MeshResource *_mesh = nullptr;
  uint32_t *_indices = nullptr;
  uint32_t _index_count = 0;
  float _point_scale = 0.0f;
  bool _own_mesh = false;
};


inline PointCloudShape::PointCloudShape(const MeshResource *mesh, const Id &id, float point_scale)
  : Shape(SIdPointCloud, id)
  , _mesh(mesh)
  , _point_scale(point_scale)
{
  setColourByHeight(true);
}


inline PointCloudShape &PointCloudShape::setColourByHeight(bool colour_by_height)
{
  if (colour_by_height)
  {
    _attributes.colour = 0;
  }
  else if (_attributes.colour == 0)
  {
    _attributes.colour = 0xFFFFFFFFu;
  }
  return *this;
}


inline bool PointCloudShape::colourByHeight() const
{
  return _attributes.colour == 0;
}


template <typename I>
PointCloudShape &PointCloudShape::setIndices(I iter, const UIntArg &index_count)
{
  freeIndices(_indices);
  _indices = nullptr;
  _index_count = index_count;
  if (index_count)
  {
    _indices = allocateIndices(index_count);
    uint32_t *ind = _indices;
    for (uint32_t i = 0; i < index_count; ++i, ++ind, ++iter)
    {
      *ind = *iter;
    }
  }

  return *this;
}
}  // namespace tes

#endif  // TES_CORE_SHAPES_POINTS_H
