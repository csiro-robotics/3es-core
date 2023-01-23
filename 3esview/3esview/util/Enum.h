//
// Author: Kazys Stepanas
//
#ifndef TES_VIEWER_UTIL_ENUM_H
#define TES_VIEWER_UTIL_ENUM_H

#include <3esview/ViewConfig.h>

/// A helper which defines bitwise operations for an enum class which defines bit flag values.
#define TES_ENUM_FLAGS(Enum, IntType)      \
  inline Enum operator|(Enum a, Enum b)    \
  {                                        \
    return Enum(IntType(a) | IntType(b));  \
  }                                        \
                                           \
  inline Enum operator&(Enum a, Enum b)    \
  {                                        \
    return Enum(IntType(a) & IntType(b));  \
  }                                        \
                                           \
  inline Enum &operator|=(Enum &a, Enum b) \
  {                                        \
    a = a | b;                             \
    return a;                              \
  }                                        \
                                           \
  inline Enum &operator&=(Enum &a, Enum b) \
  {                                        \
    a = a & b;                             \
    return a;                              \
  }                                        \
  inline Enum operator~(Enum a)            \
  {                                        \
    return Enum(~IntType(a));              \
  }


#endif  // TES_VIEWER_UTIL_ENUM_H
