#include "3esfilethread.h"

#include <3es-core/3esmessages.h>
#include <3es-core/3espacketreader.h>
#include <3es-core/3espacketstreamreader.h>

#include <cinttypes>
#include <fstream>

namespace tes::viewer
{
bool StreamThread::isLiveStream() const
{
  return false;
}


void StreamThread::run()
{
  Clock::time_point next_frame_start = Clock::now();
  // Last position in the stream we can seek to.
  std::istream::pos_type last_seekable_position = 0;
  std::istream::pos_type last_keyframe_position = 0;
  uint64_t bytes_read = 0;
  bool allow_yield = false;
  bool failed_keyframe = false;
  bool at_seekable_position = false;
  bool was_paused = _paused;
  // HACK: when restoring keyframes to precise frames we don't do the main update. Needs to be cleaned up.
  bool skip_update = false;
  FrameNumber last_keyframe_frame = 0;
  FrameNumber last_seekable_frame = 0;
  CollatedPacketDecoder packer_decoder;

  stopwatch.Start();
  while (!quit_flag)
  {
    if (blockOnPause())
    {
      continue;
    }

    if (_target_frame == 0)
    {
      // Not stepping. Check time elapsed.
      _catchingUp = false;
      std::this_thread::sleep_until(next_frame_start);
    }
    else if (_target_frame < _current_frame)
    {
      skipBack(_target_frame);
    }
    else if (_target_frame == _current_frame)
    {
      // Reached the target frame.
      _target_frame = 0;
    }

    // else
    // {
    //   lock(this)
    //   {
    //     if (_targetFrame < _currentFrame)
    //     {
    //       // Stepping back.
    //       lastKeyframeFrame = 0;
    //       lastSeekablePosition = 0;
    //       lastSeekableFrame = 0;
    //       bool restoredKeyframe = false;
    //       if (AllowKeyframes && !failedKeyframe)
    //       {
    //         Keyframe keyframe;
    //         if (TryRestoreKeyframe(out keyframe, _targetFrame))
    //         {
    //           // No failures. Does not meek we have a valid keyframe.
    //           if (keyframe != null)
    //           {
    //             lastKeyframeFrame = _currentFrame = keyframe.FrameNumber;
    //             restoredKeyframe = true;
    //             skipUpdate = _currentFrame == keyframe.FrameNumber;
    //           }
    //         }
    //         else
    //         {
    //           // Failed a keyframe. Disallow further keyframes.
    //           // TODO: consider just invalidating the failed keyframe.
    //           failedKeyframe = true;
    //         }
    //       }

    //       // Not available, not allowed or failed keyframe.
    //       if (!restoredKeyframe)
    //       {
    //         _packetStream.Reset();
    //         ResetQueue(0);
    //       }
    //       _catchingUp = _currentFrame + 1 < _targetFrame;
    //       stopwatch.Reset();
    //       stopwatch.Start();
    //     }
    //     else if (_targetFrame > _currentFrame + KeyframeSkipForwardFrames)
    //     {
    //       // Skipping forward a fair number of frames. Try for a keyframe.
    //       if (AllowKeyframes && !failedKeyframe)
    //       {
    //         // Ok to try for a keyframe.
    //         Keyframe keyframe;
    //         if (TryRestoreKeyframe(out keyframe, _targetFrame, _currentFrame))
    //         {
    //           // No failure. Check if we have a keyframe.
    //           if (keyframe != null)
    //           {
    //             lastKeyframeFrame = _currentFrame = keyframe.FrameNumber;
    //             _catchingUp = _currentFrame + 1 < _targetFrame;
    //             skipUpdate = _currentFrame == keyframe.FrameNumber;
    //           }
    //         }
    //         else
    //         {
    //           // Failed. Stream has been reset.
    //           failedKeyframe = true;
    //         }
    //       }
    //     }
    //   }  // lock(this)
    // }

    allow_yield = skip_update;
    while (!allow_yield && _stream && !_stream->eof() && _stream->good())
    {
      auto packet_header = _stream_reader->extractPacket();
      if (packet_header)
      {
        // Handle collated packets by wrapping the header.
        // This is fine for normal packets too.
        packer_decoder.setPacket(packet_header);

        // Iterate packets while we decode. These do not need to be released.
        while (auto *header = packet_decoder.next())
        {
          PacketReader packet(header);
          // Lock for frame control messages as these tell us to advance the frame and how long to wait.
          if (packet.routingId() == MtControl)
          {
            next_frame_start += processControlPacket(packet);
          }
          else
          {
            processPacket(packet);
          }
        }
      }
    }
  }
}


bool StreamThread::blockOnPause()
{
  if (_paused && target_frame == 0)
  {
    was_paused = true;
    std::unique_lock lock(_notify_mutex);
    // Wait for unpause.
    _notify.wait(lock, [this] { return !_paused; });
    return true;
  }
  return false;
}


Clock::duration StreamThread::processControlPacket(PacketReader &packet)
{
  return {};
}


void StreamThread::processPacket(PacketReader &packet)
{}
}  // namespace tes::viewer
