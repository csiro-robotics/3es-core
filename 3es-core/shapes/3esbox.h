//
// author: Kazys Stepanas
//
#ifndef _3ESBOX_H_
#define _3ESBOX_H_

#include "3es-core.h"
#include "3esshape.h"

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
class _3es_coreAPI Box : public Shape
{
public:
  /// Construct a box object.
  /// @param id The shape id and category, with unique id among @c Box objects, or zero for a transient shape.
  /// @param transform The box transformation matrix. The position is the box centre, while a unit scale denotes a unit
  /// box.
  Box(const IdCat &id = IdCat(), const Transform &transform = Transform());

  inline const char *type() const override { return "box"; }
};


inline Box::Box(const IdCat &id, const Transform &transform)
  : Shape(SIdBox, id, transform)
{}
}  // namespace tes

#endif  // _3ESBOX_H_
