//
// author: Kazys Stepanas
//
#ifndef _3ESPOINTS_H_
#define _3ESPOINTS_H_

#include <3escore/CoreConfig.h>

#include "3esintarg.h"
#include "3esmeshset.h"
#include "3esshape.h"

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
class TES_CORE_API PointCloudShape : public Shape
{
public:
  /// Construct a point cloud shape object.
  /// mesh The mesh resource to render point data from. See class comments.
  /// @param id The shape ID, unique among @c Arrow objects, or zero for a transient shape.
  /// @param category The category grouping for the shape used for filtering.
  /// @param pointScale Desired point render scale. Use zero or one for the default scale.
  PointCloudShape(const MeshResource *mesh = nullptr, const Id &id = Id(), float pointScale = 0.0f);

  /// Destructor.
  ~PointCloudShape();

  inline const char *type() const override { return "pointCloudShape"; }

  /// Colour points by height.
  ///
  /// This sets the shape colour to zero (black, with zero alpha).
  /// @param colourByHeight True to colour by height.
  PointCloudShape &setColourByHeight(bool colourByHeight);

  /// Check if colouring points by height.
  /// @return True if set to colour by height.
  bool colourByHeight() const;

  /// Set the desired point render scale. Zero or one for default.
  /// @param size The desired point scale for
  /// @return @c *this
  inline PointCloudShape &setPointScale(float scale)
  {
    _pointScale = scale;
    return *this;
  }
  /// Get the point render scale.
  /// @return The current point render scale.
  inline float pointScale() const { return _pointScale; }

  /// Return the number of @c indices().
  ///
  /// Only non-zero when referencing a subset of @c mesh() vertices.
  ///
  /// @return Zero when using all @c mesh() vertices, non-zero when referencing a subset of @c mesh().
  unsigned indexCount() const { return _indexCount; }

  /// Return the index array when a subset of @c mesh() vertices.
  ///
  /// Indices are only set when overriding indexing from @c mesh().
  ///
  /// @return An array of indices, length @c indexCount(), or null when referencing all vertices from @c mesh().
  const unsigned *indices() const { return _indices; }

  /// Sets the (optional) indices for this @c PointCloudShape @c Shape.
  /// This shape will only visualise the indexed points from its @c PointSource.
  /// This allows multiple @c PointCloudShape shapes to reference the same cloud, but reveal
  /// sub-sets of the cloud.
  ///
  /// This method is designed to copy any iterable sequence between @p begin and @p end,
  /// however the number of elements must be provided in @p indexCount.
  ///
  /// @tparam I An iterable item. Must support dereferencing to an unsigned integer and
  ///   an increment operator.
  /// @param iter The index iterator.
  /// @param indexCount The number of elements to copy from @p iter.
  /// @return This.
  template <typename I>
  PointCloudShape &setIndices(I begin, const UIntArg &indexCount);

  /// Get the mesh resource containing the point data to render.
  /// @return The point cloud mesh resource.
  inline const MeshResource *mesh() const { return _mesh; }

  /// Writes the standard create message and appends the point cloud ID (@c uint32_t).
  /// @param stream The stream to write to.
  /// @return True on success.
  bool writeCreate(PacketWriter &stream) const override;

  /// Write index data set in @c setIndices() if any.
  /// @param stream The data stream to write to.
  /// @param[in,out] progressMarker Indicates data transfer progress.
  ///   Initially zero, the @c Shape manages its own semantics.
  /// @return Indicates completion progress. 0 indicates completion,
  ///   1 indicates more data are available and more calls should be made.
  ///   -1 indicates an error. No more calls should be made.
  int writeData(PacketWriter &stream, unsigned &progressMarker) const override;

  bool readCreate(PacketReader &stream) override;

  bool readData(PacketReader &stream) override;

  /// Defines this class as a complex shape. See Shape::isComplex().
  /// @return @c true
  virtual inline bool isComplex() const override { return true; }

  /// Enumerates the mesh resource given on construction. See @c Shape::enumerateResources().
  /// @param resources Resource output array.
  /// @param capacity of @p resources.
  /// @param fetchOffset Indexing offset for the resources in this object.
  unsigned enumerateResources(const Resource **resources, unsigned capacity, unsigned fetchOffset) const override;

  /// Deep copy clone. The source is only cloned if @c ownSource() is true.
  /// It is shared otherwise.
  /// @return A deep copy.
  Shape *clone() const override;

private:
  void onClone(PointCloudShape *copy) const;

  /// Reallocate the index array preserving current data.
  /// @param count The new element size for the array.
  void reallocateIndices(uint32_t count);
  uint32_t *allocateIndices(uint32_t count);
  void freeIndices(const uint32_t *indices);

  const MeshResource *_mesh;
  uint32_t *_indices;
  uint32_t _indexCount;
  float _pointScale;
  bool _ownMesh;
};


inline PointCloudShape::PointCloudShape(const MeshResource *mesh, const Id &id, float pointScale)
  : Shape(SIdPointCloud, id)
  , _mesh(mesh)
  , _indices(nullptr)
  , _indexCount(0)
  , _pointScale(pointScale)
  , _ownMesh(false)
{
  setColourByHeight(true);
}


inline PointCloudShape &PointCloudShape::setColourByHeight(bool colourByHeight)
{
  if (colourByHeight)
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
PointCloudShape &PointCloudShape::setIndices(I iter, const UIntArg &indexCount)
{
  freeIndices(_indices);
  _indices = nullptr;
  _indexCount = indexCount;
  if (indexCount)
  {
    _indices = allocateIndices(indexCount);
    uint32_t *ind = _indices;
    for (uint32_t i = 0; i < indexCount; ++i, ++ind, ++iter)
    {
      *ind = *iter;
    }
  }

  return *this;
}
}  // namespace tes

#endif  // _3ESPOINTS_H_
