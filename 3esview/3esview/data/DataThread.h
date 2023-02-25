#ifndef TES_VIEW_DATA_THREAD_H
#define TES_VIEW_DATA_THREAD_H

#include <3esview/ViewConfig.h>

#include <3esview/FrameStamp.h>

namespace tes
{
class PacketReader;
struct ServerInfoMessage;
}  // namespace tes

namespace tes::view
{
/// Base class TES_VIEWER_API for thread objects used as message sources.
///
/// A data thread is responsible for reading incoming data, generally over a network connection or
/// from file, decoding data packages and routing them to the appropriate handlers. Note this
/// implies the message handlers must be thread safe in their message handling.
///
/// For recorded streams, it is up to the @c DataThread implementation to maintain the correct
/// packet timing.
class TES_VIEWER_API DataThread
{
public:
  /// Virtual destructor.
  virtual ~DataThread();

  /// Reports whether the current stream is a live connection or a replay.
  ///
  /// Live streams do not support playback controls such as pausing and stepping.
  /// @return True if this is a live stream.
  virtual bool isLiveStream() const = 0;

  /// Stop or disconnect, marking the thread to finish. @c join() may then be called.
  virtual void stop() = 0;

  /// Set the target frame to update to. This represents a frame jump.
  ///
  /// Threadsafe.
  /// @param frame The frame to jump, skip or step to.
  virtual void setTargetFrame(FrameNumber frame) = 0;

  /// Get the target frame to jump to. Zero the current frame is up to date; i.e., this is zero once
  /// the current frame reaches the target frame.
  /// @return The target frame to jump to.
  virtual FrameNumber targetFrame() const = 0;

  /// Get the current frame number.
  virtual FrameNumber currentFrame() const = 0;

  /// Get the total number of frames.
  /// @return The total frame count.
  virtual FrameNumber totalFrames() const = 0;

  /// Set playback mode to looping. Only applicable when @c isLiveStream() is false.
  /// @param loop True to loop.
  virtual void setLooping(bool loop) = 0;
  /// Query looping playback mode. Only applicable when @c isLiveStream() is false.
  /// @return True when looping playback.
  virtual bool looping() const = 0;

  /// Set playback speed factor. Only applicable when @c isLiveStream() is false.
  /// @param speed The playback speed multiplier. Default to 1, must be greater than zero.
  virtual void setPlaybackSpeed(float speed) = 0;
  /// Query the playback factor. Only applicable when @c isLiveStream() is false.
  /// @return The playback speed multiplier.
  virtual float playbackSpeed() const = 0;

  virtual bool paused() const = 0;
  /// Pause playback.
  virtual void pause() = 0;
  /// Unpause and resume playback.
  virtual void unpause() = 0;

  virtual void join() = 0;

protected:
  /// Process a server info message and load into @p server_info.
  /// @param reader Reader containing a server info message.
  /// @param server_info Structure to read into.
  /// @return True if successfully read.
  bool processServerInfo(PacketReader &reader, ServerInfoMessage &server_info);

private:
};
}  // namespace tes::view

#endif  // TES_VIEW_DATA_THREAD_H
