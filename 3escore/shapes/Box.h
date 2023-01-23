//
// author: Kazys Stepanas
//
#ifndef _3ESBOX_H_
#define _3ESBOX_H_

#include <3escore/CoreConfig.h>

#include "Shape.h"

namespace tes
{
/// Defines a rectangular prism shape.
///
/// The box is defined by its centre, scale and orientation. The scale defines the full extents from one corner to
/// another.
///
/// A box is defined by:
/// Component      | Description
/// -------------- | -----------------------------------------------------------------------------------------------
/// @c position()  | The box base position.
/// @c scale()     | The box size/scale, where (1, 1, 1) defines a unit box.
/// @c rotation()  | Quaternion rotation to apply to the box.
class TES_CORE_API Box : public Shape
{
public:
  /// Construct a box object.
  /// @param id The shape id and category, with unique id among @c Box objects, or zero for a transient shape.
  /// @param transform The box transformation matrix. The position is the box centre, while a unit scale denotes a unit
  /// box.
  Box(const Id &id = Id(), const Transform &transform = Transform());

  /// Copy constructor
  /// @param other Object to copy.
  Box(const Box &other);

  inline const char *type() const override { return "box"; }
};


inline Box::Box(const Id &id, const Transform &transform)
  : Shape(SIdBox, id, transform)
{}


inline Box::Box(const Box &other)
  : Shape(other)
{}
}  // namespace tes

#endif  // _3ESBOX_H_
