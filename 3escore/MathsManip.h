// Copyright (c) 2018
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Kazys Stepanas
#ifndef TES_CORE_MATHS_MANIP_H
#define TES_CORE_MATHS_MANIP_H

#include "CoreConfig.h"

#include <iomanip>
#include <iostream>

// IO stream manipulators supporting maths type streaming.

namespace tes
{
/// @ingroup tesiostream
/// Accepted values for @c ::matmode.
enum MatMode
{
  /// Display all matrix elements inline.
  MM_Inline,
  /// Insert newlines after every row.
  MM_Block
};

/// @ingroup tesiostream
/// Display mode W component in @c Vector4 and @c Quaternion types.
enum WMode
{
  /// W is displayed last to match memory layout (default).
  WM_Last,
  /// W component is displayed first.
  WM_First
};

int TES_CORE_API getMatMode(std::ostream &o);
int TES_CORE_API getQuatWMode(std::ostream &o);
int TES_CORE_API getV4WMode(std::ostream &o);
}  // namespace tes

/// @ingroup tesiostream
/// Set the @c tes::MatMode for a stream affecting @c tes::Matrix3 and @c tes::Matrix4 output.
/// @param o The stream to set the mode for.
/// @param mode The mode to set. See @c tes::MatMode
/// @return @c o
std::ostream TES_CORE_API &matmode(std::ostream &o, int mode);
/// @ingroup tesiostream
/// Set the @c tes::WMode used to display @c tes::Vector4 in a stream.
/// @param o The stream to set the mode for.
/// @param mode The mode to set. See @c tes::WMode
/// @return @c o
std::ostream TES_CORE_API &v4wmode(std::ostream &o, int mode);
/// @ingroup tesiostream
/// Set the @c tes::WMode used to display @c tes::Quaternion in a stream.
/// @param o The stream to set the mode for.
/// @param mode The mode to set. See @c tes::WMode
/// @return @c o
std::ostream TES_CORE_API &quatwmode(std::ostream &o, int mode);

#endif  // TES_CORE_MATHS_4MANIP_H
