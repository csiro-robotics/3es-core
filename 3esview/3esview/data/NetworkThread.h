#ifndef TES_VIEW_NETWORK_THREAD_H
#define TES_VIEW_NETWORK_THREAD_H

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
class TcpSocket;
}  // namespace tes

namespace tes::view
{
class ThirdEyeScene;

/// A @c DataThread implementation which reads and processes packets form a live network connection.
class TES_VIEWER_API NetworkThread : public DataThread
{
public:
  using Clock = std::chrono::steady_clock;

  NetworkThread(std::shared_ptr<ThirdEyeScene> tes, const std::string &host, uint16_t port,
                bool allow_reconnect = true);
  ~NetworkThread() override;

  /// The host to which we'll be connecting.
  /// @return The target host.
  const std::string &host() const { return _host; }

  /// The port to try connect on.
  /// @return The socket port.
  uint16_t port() const { return _port; }

  /// Is the thread allowed keep trying to connect after a connection failure, timeout or loss?
  /// @return True if reconnection is allowed.
  bool allowReconnect() const { return _allow_reconnect; }

  /// Set whether the thread is allowed try reconnecting on connection failure, timeout or loss.
  /// @param allow True to allow reconnection.
  void setAllowReconnect(bool allow) { _allow_reconnect = allow; }

  /// Check if a connection is active.
  /// @return True when connected.
  bool connected() const { return _connected; }

  /// Check if at least one connection has been attempted.
  ///
  /// From this, coupled with @s connected(), we can infer when we've failed to connect, vs not
  /// tried yet.
  ///
  /// @return True if a connection has been attempted.
  bool connectionAttempted() const { return _connection_attempted; }

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

  /// Get the target frame to jump to. Zero the current frame is up to date; i.e., this is zero once
  /// the current frame reaches the target frame.
  /// @return The target frame to jump to.
  FrameNumber targetFrame() const override;

  /// Get the current frame number.
  FrameNumber currentFrame() const override { return _currentFrame; }

  FrameNumber totalFrames() const override { return _currentFrame; }

  void setLooping(bool loop) override { (void)loop; }
  bool looping() const override { return false; }

  void setPlaybackSpeed(float speed) override { (void)speed; }
  float playbackSpeed() const override { return 1.0f; }

  /// Request the thread to quit. The thread may then be joined.
  void stop() override
  {
    _quitFlag = true;
    _allow_reconnect = false;
    unpause();
  }

  /// Check if a quit has been requested.
  /// @return True when a quit has been requested.
  bool stopping() const { return _quitFlag; }

  /// Check if playback is paused.
  /// @return True if playback is paused.
  bool paused() const override { return false; }
  /// Pause playback.
  void pause() override;
  /// Unpause and resume playback.
  void unpause() override;

  /// Wait for this thread to finish.
  void join() override;

protected:
  /// Thread entry point.
  void run();

private:
  void configureSocket(TcpSocket &socket);
  void runWith(TcpSocket &socket);

  /// Process a control packet.
  ///
  /// This covers end of frame events, so the return value indicates how long to delay before the
  /// next frame.
  ///
  /// Handles the following messages:
  /// - @c CIdFrame increments @p _currentFrame and @c _total_frames if less than current, then
  /// calls
  ///   @c ThirdEyeScene::updateToFrame() .
  /// - @c CIdCoordinateFrame updates @c _server_info then calls @c
  /// ThirdEyeScene::updateServerInfo() .
  /// - @c CIdFrameCount updates @c _total_frames.
  /// - @c CIdForceFrameFlush calls @c ThirdEyeScene::updateToFrame() with the @p _currentFrame.
  /// - @c CIdReset resets the @c _currentFrame and calls @c ThirdEyeScene::reset() .
  /// - @c CIdKeyframe - irrelevant for a live stream.
  /// - @c CIdEnd - irrelevant for a live stream.
  ///
  /// @param packet The packet to control. The routing Id is always @c MtControl.
  void processControlMessage(PacketReader &packet);

  mutable std::mutex _data_mutex;
  std::mutex _notify_mutex;
  std::condition_variable _notify;
  std::atomic_bool _quitFlag = false;
  std::atomic_bool _connected = false;
  std::atomic_bool _connection_attempted = false;
  std::atomic_bool _allow_reconnect = true;
  FrameNumberAtomic _currentFrame = 0;
  /// The total number of frames in the stream, if know. Zero when unknown.
  FrameNumber _total_frames = 0;
  std::string _host;
  uint16_t _port = 0;
  /// The scene manager.
  std::shared_ptr<ThirdEyeScene> _tes;
  std::thread _thread;
  ServerInfoMessage _server_info = {};
};
}  // namespace tes::view

#endif  // TES_VIEW_NETWORK_THREAD_H
