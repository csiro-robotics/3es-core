#include "StreamThread.h"

#include <3esview/ThirdEyeScene.h>

#include <3escore/CollatedPacketDecoder.h>
#include <3escore/Log.h>
#include <3escore/PacketReader.h>
#include <3escore/PacketStreamReader.h>

#include <cinttypes>
#include <fstream>

namespace tes::view
{
StreamThread::StreamThread(std::shared_ptr<ThirdEyeScene> tes, std::shared_ptr<std::istream> stream)
  : _stream_reader(std::make_unique<PacketStreamReader>(std::exchange(stream, nullptr)))
  , _tes(std::exchange(tes, nullptr))
{
  _thread = std::thread([this] { run(); });
}


StreamThread::~StreamThread() = default;


bool StreamThread::isLiveStream() const
{
  return false;
}


void StreamThread::setTargetFrame(FrameNumber frame)
{
  {
    std::scoped_lock guard(_data_mutex);
    _target_frame = frame;
    if (_target_frame < _currentFrame)
    {
      // Reset and seek back.
      _tes->reset();
      _stream_reader->seek(0);
      _currentFrame = 0;
    }
  }
  // Ensure the thread wakes up to step the frame.
  // Note we have unlocked the mutex before the notify call.
  _notify.notify_all();
}


FrameNumber StreamThread::targetFrame() const
{
  std::scoped_lock guard(_data_mutex);
  return _target_frame.has_value() ? *_target_frame : 0;
}


void StreamThread::setLooping(bool loop)
{
  std::scoped_lock guard(_data_mutex);
  _looping = loop;
}


bool StreamThread::looping() const
{
  std::scoped_lock guard(_data_mutex);
  return _looping;
}


void StreamThread::pause()
{
  _paused = true;
}


void StreamThread::unpause()
{
  _paused = false;
  _notify.notify_all();
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
  bool at_frame_boundary = false;
  bool have_server_info = false;
  CollatedPacketDecoder packer_decoder;

  while (!_quitFlag)
  {
    // Before anything else, check for the target frame being set. This affects catchup and
    // can trigger updates even when paused.
    FrameNumber target_frame = 0;
    switch (checkTargetFrameState(target_frame))
    {
    case TargetFrameState::NotSet:  // Nothing special to do
    default:
      _catchingUp = false;
      std::this_thread::sleep_until(next_frame_start);
      break;
    case TargetFrameState::Behind:  // Go back.
      skipBack(target_frame);
      break;
    case TargetFrameState::Ahead:  // Catch up.
      _catchingUp = true;
      break;
    case TargetFrameState::Reached:  // Result normal playback.
      _catchingUp = false;
      next_frame_start = Clock::now();
      break;
    }

    if (blockOnPause())
    {
      continue;
    }

    if (_stream_reader->isEof())
    {
      if (_looping)
      {
        setTargetFrame(0);
        have_server_info = false;
      }
    }

    at_frame_boundary = false;  // Tracks when we reach a frame boundary.
    while (!at_frame_boundary && _stream_reader->isOk() && !_stream_reader->isEof())
    {
      auto packet_header = _stream_reader->extractPacket();
      if (packet_header)
      {
        // Handle collated packets by wrapping the header.
        // This is fine for normal packets too.
        packer_decoder.setPacket(packet_header);

        // Iterate packets while we decode. These do not need to be released.
        while (auto *header = packer_decoder.next())
        {
          PacketReader packet(header);
          // Lock for frame control messages as these tell us to advance the frame and how long to wait.
          switch (packet.routingId())
          {
          case MtControl:
            next_frame_start = Clock::now() + processControlMessage(packet);
            at_frame_boundary = packet.messageId() == CIdFrame;
            break;
          case MtServerInfo:
            if (processServerInfo(packet, _server_info))
            {
              _tes->updateServerInfo(_server_info);
            }
            if (!have_server_info)
            {
              have_server_info = true;
              next_frame_start = Clock::now();
            }
            break;
          default:
            _tes->processMessage(packet);
            break;
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
  if (_paused && targetFrame() == 0)
  {
    std::unique_lock lock(_notify_mutex);
    // Wait for unpause.
    _notify.wait(lock, [this] {
      if (!_paused || targetFrame() != 0)
      {
        return true;
      }
      return false;
    });
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


StreamThread::TargetFrameState StreamThread::checkTargetFrameState(FrameNumber &target_frame)
{
  // Mutex lock to check teh
  std::scoped_lock guard(_data_mutex);

  if (!_target_frame.has_value())
  {
    target_frame = 0;
    return TargetFrameState::NotSet;
  }

  target_frame = *_target_frame;
  const auto current_frame = _currentFrame.load();

  if (target_frame < current_frame)
  {
    return TargetFrameState::Behind;
  }

  if (target_frame > current_frame)
  {
    return TargetFrameState::Behind;
  }

  _target_frame.reset();
  return TargetFrameState::Reached;
}
}  // namespace tes::view
