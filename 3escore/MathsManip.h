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
/// Accepted values for @c ::tesMatrixMode.
enum class MatrixMode : int
{
  /// Display all matrix elements inline.
  Inline,
  /// Insert newlines after every row.
  Block
};

/// @ingroup tesiostream
/// Display mode W component in @c Vector4 and @c Quaternion types.
enum class WMode : int
{
  /// W is displayed last to match memory layout (default).
  Last,
  /// W component is displayed first.
  First
};

/// Get the current matrix streaming display mode using @p stream.
/// @param stream The stream of interest.
/// @return The current matrix display mode for @p stream.
MatrixMode TES_CORE_API getMatMode(std::ostream &stream);
/// Get the current @c Quaternion W component streaming display mode using @p stream.
/// @param stream The stream of interest.
/// @return The current @c Quaternion W display mode for @p stream.
WMode TES_CORE_API getQuatWMode(std::ostream &stream);
/// Get the current @c Vector4 W component streaming display mode using @p stream.
/// @param stream The stream of interest.
/// @return The current @c Vector4 W display mode for @p stream.
WMode TES_CORE_API getV4WMode(std::ostream &stream);
}  // namespace tes

/// @ingroup tesiostream
/// Set the @c tes::MatMode for a stream affecting @c tes::Matrix3 and @c tes::Matrix4 output.
/// @param stream The stream to set the mode for.
/// @param mode The mode to set. See @c tes::MatrixMode
/// @return @c stream
std::ostream TES_CORE_API &tesMatrixMode(std::ostream &stream, tes::MatrixMode mode);
/// @ingroup tesiostream
/// Set the @c tes::WMode used to display @c tes::Vector4 in a stream.
/// @param stream The stream to set the mode for.
/// @param mode The mode to set. See @c tes::WMode
/// @return @c stream
std::ostream TES_CORE_API &tesV4WMode(std::ostream &stream, tes::WMode mode);
/// @ingroup tesiostream
/// Set the @c tes::WMode used to display @c tes::Quaternion in a stream.
/// @param stream The stream to set the mode for.
/// @param mode The mode to set. See @c tes::WMode
/// @return @c stream
std::ostream TES_CORE_API &tesQuatWMode(std::ostream &stream, tes::WMode mode);

#endif  // TES_CORE_MATHS_4MANIP_H
