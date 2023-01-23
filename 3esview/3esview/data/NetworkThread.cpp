#include "NetworkThread.h"

#include <3esview/ThirdEyeScene.h>

#include <3escore/CollatedPacketDecoder.h>
#include <3escore/Log.h>
#include <3escore/PacketBuffer.h>
#include <3escore/PacketReader.h>
#include <3escore/PacketStreamReader.h>
#include <3escore/TcpSocket.h>

#include <cinttypes>
#include <vector>

namespace tes::view
{
NetworkThread::NetworkThread(std::shared_ptr<ThirdEyeScene> tes, const std::string &host, uint16_t port,
                             bool allow_reconnect)
  : _host(host)
  , _port(port)
  , _allow_reconnect(allow_reconnect)
{
  _tes = std::exchange(tes, nullptr);
  _thread = std::thread([this] { run(); });
}


bool NetworkThread::isLiveStream() const
{
  return true;
}


void NetworkThread::setTargetFrame(FrameNumber frame)
{
  // Not supported.
  (void)frame;
}


FrameNumber NetworkThread::targetFrame() const
{
  return 0;
}


void NetworkThread::setLooping(bool loop)
{
  // Not supported.
  (void)loop;
}


bool NetworkThread::looping() const
{
  return false;
}


void NetworkThread::pause()
{
  // Not supported.
}


void NetworkThread::unpause()
{
  // Not supported.
}


void NetworkThread::join()
{
  _quitFlag = true;
  _allow_reconnect = false;
  _thread.join();
}


void NetworkThread::run()
{
  Clock::time_point next_frame_start = Clock::now();
  auto socket = std::make_unique<TcpSocket>();

  do
  {
    const bool connected = socket->open(_host.c_str(), _port);
    _connected = connected;
    _connection_attempted = true;
    if (!connected)
    {
      if (_allow_reconnect)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      }
      continue;
    }

    // Connected.
    configureSocket(*socket);
    runWith(*socket);
    socket->close();
  } while (_allow_reconnect || !_quitFlag);
}


void NetworkThread::configureSocket(TcpSocket &socket)
{
  socket.setNoDelay(true);
  socket.setReadTimeout(0);
  socket.setWriteTimeout(0);
  socket.setReadBufferSize(0xffff);
  socket.setSendBufferSize(4 * 1024);
}


void NetworkThread::runWith(TcpSocket &socket)
{
  CollatedPacketDecoder packet_decoder;
  bool have_server_info = false;
  // We have two buffers here -> redundant.
  // TODO(KS): change the PacketBuffer interface so we can read directly into it's buffer.
  PacketBuffer packet_buffer;
  std::vector<uint8_t> read_buffer(2 * 1024u);

  _currentFrame = 0;
  _total_frames = 0;

  // Make sure we reset from any previous connection.
  _tes->reset();

  while (socket.isConnected() && !_quitFlag)
  {
    auto bytes_read = socket.readAvailable(read_buffer.data(), int(read_buffer.size()));
    if (bytes_read <= 0)
    {
      continue;
    }

    packet_buffer.addBytes(read_buffer.data(), bytes_read);

    if (const auto *packet_header = packet_buffer.extractPacket(read_buffer))
    {
      packet_decoder.setPacket(packet_header);

      while (packet_header = packet_decoder.next())
      {
        PacketReader packet(packet_header);
        // Lock for frame control messages as these tell us to advance the frame and how long to wait.
        switch (packet.routingId())
        {
        case MtControl:
          processControlMessage(packet);
          break;
        case MtServerInfo:
          if (processServerInfo(packet, _server_info))
          {
            _tes->updateServerInfo(_server_info);
          }
          if (!have_server_info)
          {
            have_server_info = true;
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

void NetworkThread::processControlMessage(PacketReader &packet)
{
  ControlMessage msg;
  if (!msg.read(packet))
  {
    log::error("Failed to decode control packet: ", packet.messageId());
    return;
  }

  switch (packet.messageId())
  {
  case CIdNull:
    break;
  case CIdFrame: {
    const auto current_frame = ++_currentFrame;
    // Frame ending.
    _tes->updateToFrame(current_frame);
    _total_frames = std::max(current_frame, _total_frames);
    break;
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
    break;
  case CIdReset:
    // This doesn't seem right any more. Need to check what the Unity viewer did with this. It may be an artifact of
    // the main thread needing to do so much work in Unity.
    _currentFrame = msg.value32;
    _tes->reset();
    break;
  case CIdKeyframe:
    break;
  case CIdEnd:
    break;
  default:
    log::error("Unknown control message id: ", packet.messageId());
    break;
  }
}
}  // namespace tes::view
