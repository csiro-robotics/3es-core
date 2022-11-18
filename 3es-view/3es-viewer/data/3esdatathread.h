#ifndef TES_VIEWER_DATA_THREAD_H
#define TES_VIEWER_DATA_THREAD_H

#include "3es-viewer.h"

namespace tes::viewer
{
/// Base class for thread objects used as message sources.
///
/// A data thread is responsible for reading incoming data, generally over a network connection or from file, decoding
/// data packages and routing them to the appropriate handlers. Note this implies the message handlers must be thread
/// safe in their message handling.
///
/// For recorded streams, it is up to the @c DataThread implementation to maintain the correct packet timing.
class DataThread
{
public:
  /// Reports whether the current stream is a live connection or a replay.
  ///
  /// Live streams do not support playback controls such as pausing and stepping.
  /// @return True if this is a live stream.
  virtual bool isLiveStream() const = 0;

protected:
private:
};
}  // namespace tes::viewer

#endif  // TES_VIEWER_DATA_THREAD_H
