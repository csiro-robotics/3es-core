//
// author: Kazys Stepanas
//
#ifndef TES_CORE_STREAM_UTIL_H
#define TES_CORE_STREAM_UTIL_H

#include "CoreConfig.h"

#include <cinttypes>
#include <iosfwd>

namespace tes
{
struct ServerInfoMessage;

namespace streamutil
{
/// Initialises the file stream ensuring the @p server_info and a preliminary @c CIdFrameCount @c
/// ControlMessage are written. The frame count is corrected on calling @c finaliseStream().
///
/// In some cases a @c ServerInfoMessage may already be written, such as in a @c FileConnection. For
/// this the
/// @p server_info argument is optional. When @c null, it is assumed to already have been written
/// and only the frame count placeholder is established.
///
/// @param stream The file stream to initialise. Must support writing.
/// @param server_info Optional server info to write. When null it is assumed that this has already
/// been written.
/// @return True on success, false due to any failure.
bool TES_CORE_API initialiseStream(std::ostream &stream,
                                   const ServerInfoMessage *server_info = nullptr);

/// Finalise a data stream previously initialised with @c initialiseStream(). The @p stream must be
/// seekable for read/write so that the initial @c CIdFrameCount @c ControlMessage can be found and
/// fixed.
///
/// @param stream The file stream to initialise. Must support reading and writing.
/// @param frame_count The final frame count which has been written to the data stream. This value
/// will be written to the @c CIdFrameCount @c ControlMessage which appears near the start of the
/// stream.
/// @param server_info Optional server info structure to rewrite to the stream. When given, this
/// structure is written over the existing @c ServerInfoMessage near the start of the stream. This
/// handles cases where the info may not be known at the start.
/// @return True on success, false due to any failure.
bool TES_CORE_API finaliseStream(std::iostream &stream, uint32_t frame_count,
                                 const ServerInfoMessage *server_info = nullptr);
}  // namespace streamutil
}  // namespace tes

#endif  // TES_CORE_STREAM_UTIL_H
