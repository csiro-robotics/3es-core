#ifndef TES_VIEWER_STREAM_THREAD_H
#define TES_VIEWER_STREAM_THREAD_H

#include "3es-viewer.h"

#include "3esdatathread.h"

#include "3es-viewer/3esframestamp.h"

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iosfwd>
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
/// A @c DataThread implementation which reads and processes packets form a file.
class StreamThread : public DataThread
{
public:
  using Clock = std::chrono::steady_clock;

  /// Reports whether the current stream is a live connection or a replay.
  ///
  /// Live streams do not support playback controls such as pausing and stepping.
  /// @return True if this is a live stream.
  bool isLiveStream() const override;

protected:
  void run();

private:
  /// Block if paused until unpaused.
  /// @return True if we were paused and had to wait.
  bool blockOnPause();

  /// Process a control packet.
  /// This covers end of frame events, so the return value indicates how long to delay before the next
  /// frame.
  /// @param packet The packet to control. The routing Id is always @c MtControl.
  /// @return The time to delay before the next frame, or a zero duration when not a frame message.
  Clock::duration processControlPacket(PacketReader &packet);

  /// Process the given packet, routing it to the appropriate handler.
  ///
  /// Never called for packets where the routing Id is @c MtControl.
  /// @param packet The packet to process.
  void processPacket(PacketReader &packet);

  std::mutex _data_mutex;
  std::mutex _notify_mutex;
  std::condition_variable _notify;
  std::atomic_bool _quit_flag;
  std::atomic_bool _paused;
  FrameNumber _target_frame = 0;
  FrameNumber _current_frame = 0;
  std::unique_ptr<PacketStreamReader> _stream_reader;
  std::shared_ptr<tes::CollatedPacketDecoder> _collated_packet_decoder;
  ServerInfoMessage _server_info = {};
  std::array<uint8_t, 64 * 1024> _data_window;
};
}  // namespace tes::viewer

#endif  // TES_VIEWER_STREAM_THREAD_H
