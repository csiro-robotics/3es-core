#ifndef TES_VIEWER_MAGNUM_V3_H
#define TES_VIEWER_MAGNUM_V3_H

#include "3es-viewer.h"

#include <3esvector3.h>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

namespace tes::viewer
{
inline tes::Vector3<Magnum::Float> convert(const Magnum::Vector3 &v)
{
  return tes::Vector3<Magnum::Float>(v.x(), v.y(), v.z());
}

template <typename T>
inline Magnum::Vector3 convert(const tes::Vector3<T> &v)
{
  return Magnum::Vector3(Magnum::Float(v.x), Magnum::Float(v.y), Magnum::Float(v.z));
}
};  // namespace tes::viewer

#endif  // TES_VIEWER_MAGNUM_V3_H
