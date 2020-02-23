//
// author: Kazys Stepanas
//
#ifndef _3ESMULTISHAPE_H_
#define _3ESMULTISHAPE_H_

#include "3es-core.h"

#include "3esintarg.h"
#include "3esquaternionarg.h"
#include "3esshape.h"
#include "3esv3arg.h"

#include <algorithm>

namespace tes
{
class _3es_coreAPI MultiShape : public Shape
{
public:
  /// Maximum number of shapes in a multi shape. Packet is too large otherwise.
  const uint16_t ShapeCountLimit = 1024u;

  MultiShape(Shape **shapes, const IntArgT<uint16_t> &shapeCount, const V3Arg &position = Vector3f::zero,
             const QuaternionArg &rotation = Quaternionf::identity, const V3Arg &scale = Vector3f::one);
  ~MultiShape();

  bool writeCreate(PacketWriter &stream) const override;

  MultiShape &takeOwnership();

private:
  Shape **_shapes = nullptr;
  uint16_t _itemCount = 0;
  bool _ownShapes = false;
};

inline MultiShape::MultiShape(Shape **shapes, const IntArgT<uint16_t> &shapeCount, const V3Arg &position,
                              const QuaternionArg &rotation, const V3Arg &scale)
: Shape(shapes[0]->routingId(), shapes[0]->id(), shapes[0]->category())
, _shapes(shapes)
, _itemCount(std::min(static_cast<uint16_t>(shapeCount), ShapeCountLimit))
{
  setPosition(position);
  setRotation(rotation);
  setScale(scale);
  _data.flags |= OFMultiShape;
}
}

#endif // _3ESMULTISHAPE_H_