#include "3esstreamthread.h"

#include "3esthirdeyescene.h"

#include <3escollatedpacketdecoder.h>
#include <3espacketreader.h>
#include <3espacketstreamreader.h>
#include <3eslog.h>

#include <cinttypes>
#include <fstream>

namespace tes::viewer
{
StreamThread::StreamThread(std::shared_ptr<std::istream> stream, std::shared_ptr<ThirdEyeScene> tes)
{
  _stream_reader = std::make_unique<PacketStreamReader>(std::exchange(stream, nullptr));
  _collated_packet_decoder = std::make_unique<CollatedPacketDecoder>();
  _tes = std::exchange(tes, nullptr);
  _thread = std::thread([this] { run(); });
}


bool StreamThread::isLiveStream() const
{
  return false;
}


void StreamThread::setTargetFrame(FrameNumber frame)
{
  std::lock_guard guard(_data_mutex);
  _target_frame = frame;
  if (_target_frame < _currentFrame)
  {
    // Reset and seek back.
    _tes->reset();
    _stream_reader->seek(0);
  }
}


FrameNumber StreamThread::targetFrame() const
{
  std::lock_guard guard(_data_mutex);
  return _target_frame;
}


void StreamThread::pause()
{
  _paused = false;
  _notify.notify_all();
}


void StreamThread::unpause()
{
  if (_paused)
  {
    _paused = false;
  }
}


void StreamThread::join()
{
  _quitFlag = true;
  unpause();
  _thread.join();
}


void StreamThread::run()
{
  Clock::time_point next_frame_start = Clock::now();
  // Last position in the stream we can seek to.
  std::istream::pos_type last_seekable_position = 0;
  std::istream::pos_type last_keyframe_position = 0;
  uint64_t bytes_read = 0;
  bool allow_yield = false;
  bool was_paused = _paused;
  // HACK: when restoring keyframes to precise frames we don't do the main update. Needs to be cleaned up.
  bool skip_update = false;
  CollatedPacketDecoder packer_decoder;

  while (!_quitFlag)
  {
    if (blockOnPause())
    {
      continue;
    }

    auto current_frame = _currentFrame.load();
    auto target_frame = targetFrame();
    if (target_frame == 0)
    {
      // Not stepping. Check time elapsed.
      _catchingUp = false;
      std::this_thread::sleep_until(next_frame_start);
    }
    else if (target_frame < current_frame)
    {
      skipBack(target_frame);
    }
    else if (target_frame > current_frame)
    {
      _catchingUp = true;
    }
    else if (_target_frame == current_frame)
    {
      // Reached the target frame.
      _target_frame = 0;
      next_frame_start = Clock::now();
    }

    allow_yield = skip_update;
    while (!allow_yield && _stream_reader->isOk() && !_stream_reader->isEof())
    {
      auto packet_header = _stream_reader->extractPacket();
      if (packet_header)
      {
        // Handle collated packets by wrapping the header.
        // This is fine for normal packets too.
        packer_decoder.setPacket(packet_header);

        // Iterate packets while we decode. These do not need to be released.
        while (auto *header = _collated_packet_decoder->next())
        {
          PacketReader packet(header);
          // Lock for frame control messages as these tell us to advance the frame and how long to wait.
          if (packet.routingId() == MtControl)
          {
            next_frame_start += processControlMessage(packet);
          }
          else
          {
            _tes->processMessage(packet);
          }
        }
      }
    }
  }
}


void StreamThread::skipBack(FrameNumber targetFrame)
{
  // Simple implementation until we get keyframes.
  setTargetFrame(targetFrame);
}


bool StreamThread::blockOnPause()
{
  if (_paused && _target_frame == 0)
  {
    std::unique_lock lock(_notify_mutex);
    // Wait for unpause.
    _notify.wait(lock, [this] { return !_paused; });
    return true;
  }
  return false;
}


StreamThread::Clock::duration StreamThread::processControlMessage(PacketReader &packet)
{
  ControlMessage msg;
  if (!msg.read(packet))
  {
    log::error("Failed to decode control packet: ", packet.messageId());
    return {};
  }
  switch (packet.messageId())
  {
  case CIdNull:
    break;
  case CIdFrame: {
    // Frame ending.
    _tes->updateToFrame(++_currentFrame);
    // Work out how long to the next frame.
    const auto dt = (msg.value32) ? msg.value32 : _server_info.defaultFrameTime;
    return std::chrono::microseconds(_server_info.timeUnit * dt);
  }
  case CIdCoordinateFrame:
    if (msg.value32 < CFCount)
    {
      _server_info.coordinateFrame = CoordinateFrame(msg.value32);
      _tes->updateServerInfo(_server_info);
    }
    else
    {
      log::error("Invalid coordinate frame value: ", msg.value32);
    }
    break;
  case CIdFrameCount:
    _total_frames = msg.value32;
    break;
  case CIdForceFrameFlush:
    _tes->updateToFrame(_currentFrame);
    return std::chrono::microseconds(_server_info.timeUnit * _server_info.defaultFrameTime);
  case CIdReset:
    // This doesn't seem right any more. Need to check what the Unity viewer did with this. It may be an artifact of
    // the main thread needing to do so much work in Unity.
    _currentFrame = msg.value32;
    _tes->reset();
    break;
  case CIdKeyframe:
    // NYI
    log::warn("Keyframe control message handling not implemented.");
    break;
  case CIdEnd:
    log::warn("End control message handling not implemented.");
    break;
  default:
    log::error("Unknown control message id: ", packet.messageId());
    break;
  }
  return {};
}
}  // namespace tes::viewer
