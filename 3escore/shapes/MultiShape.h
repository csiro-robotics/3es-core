//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SHAPES_MULTI_SHAPE_H
#define TES_CORE_SHAPES_MULTI_SHAPE_H

#include <3escore/CoreConfig.h>

#include "Shape.h"

#include <3escore/IntArg.h>
#include <3escore/Ptr.h>
#include <3escore/QuaternionArg.h>
#include <3escore/V3Arg.h>

#include <algorithm>

namespace tes
{
/// The @c MultiShape allows groups of shapes of the same type to be created and managed using a
/// single shape ID.
///
/// The @c MultiShape is tailored to primitive shapes only. All the shapes provided @c MultiShape
/// must be of the same type and must not be complex shapes (see @c Shape::isComplex() ).
///
/// It is up to the user to respect the constraints above or undefine behaviour will occur.
///
/// The @c MultiShape accepts either borrowed pointer or @c shared_ptr shape addressing, via @c Ptr
/// objects. It may may optionally ensure ownership (potentially shared) of all it's constituent
/// shapes via @c takeOwnership(). In this case, the @c MultiShape creates copies of all the shapes
/// addressed by borrowed pointers.
class TES_CORE_API MultiShape : public Shape
{
public:
  /// Maximum number of shapes in a multi shape packet using single precision. Halve for double
  /// precision. Packet is too large otherwise.
  static const unsigned BlockCountLimitSingle;  // = 1024u;
  using ShapePtr = Ptr<const Shape>;

  /// Create a new multi-shape with the given set of @p shapes. The @c routingId(), @c id() and @c
  /// category() for the shape set is taken from the first item in the array.
  ///
  /// @param shapes The shape array. The array and all members must not be null.
  /// @param shape_count Number of items in @p shapes. Truncated to @c ShapeCountLimit .
  /// @param position A translation to apply to all shapes in the collection.
  /// @param rotation A rotation transformation to apply to all shapes in the collection.
  /// @param scale Scaling to apply to all shapes in the collection.
  template <typename Iter>
  MultiShape(Iter shapes_begin, Iter shapes_end, const Transform &transform = Transform());

  /// Move constructor
  /// @param other Object to move.
  MultiShape(MultiShape &&other) noexcept;

  /// Destructor.
  ~MultiShape() override;

  /// Complex to support large shape counts.
  /// @return @c true
  [[nodiscard]] bool isComplex() const override;

  /// Override to effect the multi-shape creation.
  /// @param stream Packet stream.
  bool writeCreate(PacketWriter &stream) const override;

  int writeData(PacketWriter &stream, unsigned &progress_marker) const override;

  /// Take ownership of the shape array.
  ///
  /// The @c MultiShape copies the original array and takes ownership for deleting each shape in the
  /// array.
  ///
  /// @return @c *this
  MultiShape &takeOwnership();

  [[nodiscard]] unsigned blockCountLimit() const;

private:
  /// The shape array. Pointer ownership is defined by @c _own_shapes .
  std::vector<ShapePtr> _shapes;
  /// True if _shapes is internally allocated and elements are to be deleted.
  bool _own_shapes = false;
};


template <typename Iter>
inline MultiShape::MultiShape(Iter shapes_begin, Iter shapes_end, const Transform &transform)
  : Shape(MtNull)  // Note(KS): constructed with an invalid routing ID.
{
  setTransform(transform);
  bool first = true;
  for (auto iter = shapes_begin; iter != shapes_end; ++iter)
  {
    // Use the first shape to set the routing ID and shape ID.
    if (first)
    {
      first = false;
      _routing_id = (*iter)->routingId();
      setId((*iter)->id());
      setCategory((*iter)->category());
    }

    _shapes.emplace_back(Ptr<Shape>(*iter));
  }
}


inline MultiShape::MultiShape(MultiShape &&other) noexcept
  : Shape(other)
  , _shapes(std::move(other._shapes))
  , _own_shapes(std::exchange(other._own_shapes, false))
{}


inline unsigned MultiShape::blockCountLimit() const
{
  return (doublePrecision()) ? BlockCountLimitSingle / 2 : BlockCountLimitSingle;
}
}  // namespace tes

#endif  // TES_CORE_SHAPES_MULTI_SHAPE_H
