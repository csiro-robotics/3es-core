#ifndef TES_VIEWER_STREAM_THREAD_H
#define TES_VIEWER_STREAM_THREAD_H

#include "3es-viewer.h"

#include "3esdatathread.h"

#include "3es-viewer/3esframestamp.h"

#include <3esmessages.h>

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iosfwd>
#include <optional>
#include <memory>
#include <mutex>

namespace tes
{
class CollatedPacketDecoder;
class PacketBuffer;
class PacketReader;
class PacketStreamReader;
}  // namespace tes

namespace tes::viewer
{
class ThirdEyeScene;

/// A @c DataThread implementation which reads and processes packets form a file.
class TES_VIEWER_API StreamThread : public DataThread
{
public:
  using Clock = std::chrono::steady_clock;

  StreamThread(std::shared_ptr<std::istream> stream, std::shared_ptr<ThirdEyeScene> tes);

  /// Reports whether the current stream is a live connection or a replay.
  ///
  /// Live streams do not support playback controls such as pausing and stepping.
  /// @return True if this is a live stream.
  bool isLiveStream() const override;

  /// Set the target frame to update to. This represents a frame jump.
  ///
  /// Threadsafe.
  /// @param frame The frame to jump, skip or step to.
  void setTargetFrame(FrameNumber frame) override;

  /// Get the target frame to jump to. Zero the current frame is up to date; i.e., this is zero once the current frame
  /// reaches the target frame.
  /// @return The target frame to jump to.
  FrameNumber targetFrame() const override;

  /// Get the current frame number.
  inline FrameNumber currentFrame() const override { return _currentFrame; }

  void setLooping(bool loop) override;
  bool looping() const override;

  /// Request the thread to quit. The thread may then be joined.
  inline void stop() override
  {
    _quitFlag = true;
    unpause();
  }

  /// Check if a quit has been requested.
  /// @return True when a quit has been requested.
  inline bool stopping() const { return _quitFlag; }

  /// Check if playback is paused.
  /// @return True if playback is paused.
  inline bool paused() const override { return _paused; }
  /// Pause playback.
  void pause() override;
  /// Unpause and resume playback.
  void unpause() override;

  /// Wait for this thread to finish.
  void join() override;

protected:
  /// Thread entry point.
  void run();

  void skipBack(FrameNumber targetFrame);

private:
  /// Block if paused until unpaused.
  /// @return True if we were paused and had to wait.
  bool blockOnPause();

  /// Process a control packet.
  /// This covers end of frame events, so the return value indicates how long to delay before the next
  /// frame.
  /// @param packet The packet to control. The routing Id is always @c MtControl.
  /// @return The time to delay before the next frame, or a zero duration when not a frame message.
  Clock::duration processControlMessage(PacketReader &packet);

  void processServerInfo(PacketReader &reader);

  mutable std::mutex _data_mutex;
  std::mutex _notify_mutex;
  std::condition_variable _notify;
  std::atomic_bool _quitFlag;
  std::atomic_bool _paused;
  bool _catchingUp = false;
  bool _looping = false;
  std::optional<FrameNumber> _target_frame;
  FrameNumberAtomic _currentFrame = 0;
  /// The total number of frames in the stream, if know. Zero when unknown.
  FrameNumber _total_frames = 0;
  std::unique_ptr<PacketStreamReader> _stream_reader;
  /// The scene manager.
  std::shared_ptr<ThirdEyeScene> _tes;
  std::thread _thread;
  ServerInfoMessage _server_info = {};
};
}  // namespace tes::viewer

#endif  // TES_VIEWER_STREAM_THREAD_H
