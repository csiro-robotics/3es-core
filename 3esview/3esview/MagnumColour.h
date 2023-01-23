#ifndef TES_VIEWER_MAGNUM_COLOUR_H
#define TES_VIEWER_MAGNUM_COLOUR_H

#include "3esview/Viewer.h"

#include <3escore/Colour.h>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>

namespace tes::viewer
{
inline tes::Colour convert(const Magnum::Color3 &c)
{
  return tes::Colour(c.x(), c.y(), c.z());
}

inline tes::Colour convert(const Magnum::Color4 &c)
{
  return tes::Colour(c.x(), c.y(), c.z(), c.w());
}

inline Magnum::Color4 convert(const tes::Colour &c)
{
  return Magnum::Color4(Magnum::Float(c.rf()), Magnum::Float(c.gf()), Magnum::Float(c.bf()), Magnum::Float(c.af()));
}
};  // namespace tes::viewer

#endif  // TES_VIEWER_MAGNUM_COLOUR_H
