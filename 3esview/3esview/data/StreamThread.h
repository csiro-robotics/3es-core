#ifndef TES_VIEW_STREAM_THREAD_H
#define TES_VIEW_STREAM_THREAD_H

#include <3esview/ViewConfig.h>

#include "DataThread.h"

#include <3esview/FrameStamp.h>

#include <3escore/Messages.h>

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iosfwd>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

namespace tes
{
class CollatedPacketDecoder;
class PacketBuffer;
class PacketReader;
class PacketStreamReader;
}  // namespace tes

namespace tes::view
{
class ThirdEyeScene;

/// A @c DataThread implementation which reads and processes packets form a file.
class TES_VIEWER_API StreamThread : public DataThread
{
public:
  using Clock = std::chrono::steady_clock;

  StreamThread(std::shared_ptr<ThirdEyeScene> tes, std::shared_ptr<std::istream> stream);
  ~StreamThread();

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

  FrameNumber totalFrames() const override
  {
    std::lock_guard guard(_data_mutex);
    return _total_frames;
  }

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
  ///
  /// This covers end of frame events, so the return value indicates how long to delay before the next
  /// frame.
  ///
  /// Handles the following messages:
  /// - @c CIdFrame increments @p _currentFrame then calls @c ThirdEyeScene::updateToFrame() .
  /// - @c CIdCoordinateFrame updates @c _server_info then calls @c ThirdEyeScene::updateServerInfo() .
  /// - @c CIdFrameCount updates @c _total_frames.
  /// - @c CIdForceFrameFlush calls @c ThirdEyeScene::updateToFrame() with the @p _currentFrame.
  /// - @c CIdReset resets the @c _currentFrame and calls @c ThirdEyeScene::reset() .
  /// - @c CIdKeyframe - NYI
  /// - @c CIdEnd - NYI
  ///
  /// @param packet The packet to control. The routing Id is always @c MtControl.
  /// @return The time to delay before the next frame, or a zero duration when not a frame message.
  Clock::duration processControlMessage(PacketReader &packet);

  /// Return values for @c checkTargetFrameState()
  enum class TargetFrameState
  {
    NotSet,  ///< No target frame set
    Behind,  ///< Target frame is set and behind the current frame. Requires keyframe or file reset.
    Ahead,   ///< The target frame is ahead of the current frame.
    Reached  ///< Target frame has just been reached.
  };

  /// Check the conditions around the target frame being set.
  ///
  /// This checks the @c targetFrame() value to see if we need to adjust playback to reach the
  /// target frame. The current state is indicated by the @c TargetFrameState return value.
  ///
  /// - @c TargetFrameState::NotSet : the target frame is not set and we use normal playback rules.
  /// - @c TargetFrameState::Behind : the target frame set behind the current frame. We must reset
  ///   to a keyframe (or the file start) and catch up to the desired frame. After the reset the
  ///   next check will be @c TargetFrameState::Ahead until the frame is
  ///   @c TargetFrameState::Reached
  /// - @c TargetFrameState::Ahead : the target frame is set ahead of the current frame and we need
  ///   to process messages to catch up to the target frame.
  /// - @c TargetFrameState::Reached : the target frame has been reached and we can result normal
  ///   playback. This also clears the @c targetFrame() value so the next call will return
  ///   @c TargetFrameState::NotSet
  ///
  /// @param[out] target_frame Set to the target frame value. Only useful when returning
  /// @c TargetFrameState::Valid
  /// @return Details of the @c targetFrameValue() - see comments.
  TargetFrameState checkTargetFrameState(FrameNumber &target_frame);

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
}  // namespace tes::view

#endif  // TES_VIEW_STREAM_THREAD_H
