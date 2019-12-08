//
// author: Kazys Stepanas
//
#ifndef _3ESSTREAMUTIL_H_
#define _3ESSTREAMUTIL_H_

#include "3es-core.h"

#include <iosfwd>

namespace tes
{
struct ServerInfoMessage;

  namespace streamutil
  {
    /// Initialises the file stream ensuring the @p serverInfo and a preliminary @c CIdFrameCount @c ControlMessage are
    /// written. The frame count is corrected on calling @c finaliseStream().
    ///
    /// In some cases a @c ServerInfoMessage may already be written, such as in a @c FileConnection. For this the
    /// @p serverInfo argument is optional. When @c null, it is assumed to already have been written and only the frame
    /// count placeholder is established.
    ///
    /// @param stream The file stream to initialise. Must support writing.
    /// @param serverInfo Optional server info to write. When null it is assumed that this has already been written.
    /// @return True on success, false due to any failure.
    bool _3es_coreAPI initialiseStream(std::ostream &stream, const ServerInfoMessage *serverInfo);

    /// Finalise a data stream previously initialised with @c initialiseStream(). The @p stream must be seekable for
    /// read/write so that the initial @c CIdFrameCount @c ControlMessage can be found and fixed.
    ///
    /// @param stream The file stream to initialise. Must support reading and writing.
    /// @return True on success, false due to any failure.
    bool _3es_coreAPI finaliseStream(std::iostream &stream, unsigned frameCount);
  } // namespace streamutil
} // namespace tes

#endif // _3ESSTREAMUTIL_H_
