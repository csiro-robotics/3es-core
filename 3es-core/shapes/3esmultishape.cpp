#include "3esmultishape.h"

using namespace tes;

MultiShape::~MultiShape()
{
  if (_ownShapes)
  {
    for (unsigned i = 0; i < _itemCount; ++i)
    {
      delete _shapes[i];
    }

    delete [] _shapes;
  }
}

bool MultiShape::writeCreate(PacketWriter &stream) const
{
  if (!Shape::writeCreate(stream))
  {
    return false;
  }

  bool ok = true;
  stream.writeElement(_itemCount) == sizeof(_itemCount) && ok;

  for (unsigned i = 0; i < _itemCount; ++i)
  {
    ok = _shapes[i]->data().attributes.write(stream) && ok;
  }

  return ok;
}


MultiShape &MultiShape::takeOwnership()
{
  if (!_ownShapes)
  {
    Shape **shapes = new Shape*[_itemCount];
    for (unsigned i = 0; i < _itemCount; ++i)
    {
      shapes[i] = _shapes[i];
    }
    _shapes = shapes;
    _ownShapes = true;
  }

  return *this;
}
