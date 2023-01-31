//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SHAPES_MULTI_SHAPE_H
#define TES_CORE_SHAPES_MULTI_SHAPE_H

#include <3escore/CoreConfig.h>

#include "Shape.h"

#include <3escore/IntArg.h>
#include <3escore/QuaternionArg.h>
#include <3escore/V3Arg.h>

#include <algorithm>

namespace tes
{
/// The @c MultiShape allows groups of shapes of the same type to be created and managed using a single shape ID.
///
/// The @c MultiShape is tailored to primitive shapes only. All the shapes provided @c MultiShape must be of the same
/// type and must not be complex shapes (see @c Shape::isComplex() ). The @c MultiShape supports a maximum of
/// @c ShapeCountLimit items.
///
/// It is up to the user to respect the constraints above or undefine behaviour will occur.
///
/// The @c MultiShape may optionally take ownership of the shape memory via @c takeOwnership(). In this case, the
/// @c MultiShape copies the original array and takes ownership for the contained pointers. This means the original
/// array may be disposed of, but not the shapes in the array - these will be cleaned up by the @c MultiShape.
class TES_CORE_API MultiShape : public Shape
{
public:
  /// Maximum number of shapes in a multi shape packet using single precision. Halve for double precision. Packet is too
  /// large otherwise.
  static const unsigned BlockCountLimitSingle;  // = 1024u;
  /// Maximum number of shapes in a multi shape.
  static const unsigned ShapeCountLimit;  // = 0xffffu;

  /// Create a new multi-shape with the given set of @p shapes. The @c routingId(), @c id() and @c category() for the
  /// shape set is taken from the first item in the array.
  ///
  /// @param shapes The shape array. The array and all members must not be null.
  /// @param shapeCount Number of items in @p shapes. Truncated to @c ShapeCountLimit .
  /// @param position A translation to apply to all shapes in the collection.
  /// @param rotation A rotation transformation to apply to all shapes in the collection.
  /// @param scale Scaling to apply to all shapes in the collection.
  MultiShape(Shape **shapes, const UIntArg &shapeCount, const Transform &transform = Transform());

  /// Move constructor
  /// @param other Object to move.
  MultiShape(MultiShape &&other);

  /// Destructor.
  ~MultiShape();

  /// Complex to support large shape counts.
  /// @return @c true
  bool isComplex() const override;

  /// Override to effect the multi-shape creation.
  /// @param stream Packet stream.
  bool writeCreate(PacketWriter &stream) const override;

  int writeData(PacketWriter &stream, unsigned &progressMarker) const override;

  /// Take ownership of the shape array.
  ///
  /// The @c MultiShape copies the original array and takes ownership for deleting each shape in the array.
  ///
  /// @return @c *this
  MultiShape &takeOwnership();

  unsigned blockCountLimit() const;

private:
  Shape **_shapes = nullptr;  ///< The shape array. Pointer ownership is defined by @c _ownShapes .
  uint32_t _itemCount = 0;    ///< Number of items in @c _shapes.
  bool _ownShapes = false;    ///< True if _shapes is internally allocated and elements are to be deleted.
};

inline MultiShape::MultiShape(Shape **shapes, const UIntArg &shapeCount, const Transform &transform)
  : Shape(shapes[0]->routingId(), Id(shapes[0]->id(), shapes[0]->category()), transform)
  , _shapes(shapes)
  , _itemCount(std::min(static_cast<uint32_t>(shapeCount), ShapeCountLimit))
{
  _data.flags |= OFMultiShape;
  setDoublePrecision(shapes[0]->doublePrecision());
}


inline MultiShape::MultiShape(MultiShape &&other)
  : Shape(other)
  , _shapes(std::exchange(other._shapes, nullptr))
  , _itemCount(std::exchange(other._itemCount, 0))
  , _ownShapes(std::exchange(other._ownShapes, false))
{}


inline unsigned MultiShape::blockCountLimit() const
{
  return (doublePrecision()) ? BlockCountLimitSingle / 2 : BlockCountLimitSingle;
}
}  // namespace tes

#endif  // TES_CORE_SHAPES_MULTI_SHAPE_H
