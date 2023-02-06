//
// author: Kazys Stepanas
//
#include "StreamUtil.h"

#include "CoreUtil.h"
#include "Endian.h"
#include "Messages.h"
#include "PacketWriter.h"

#include <array>
#include <iostream>
#include <type_traits>
#include <vector>

namespace tes::streamutil
{
namespace
{
using PosType = std::iostream::pos_type;

struct PacketInfo
{
  PosType stream_pos = 0;
  uint16_t routing_id = 0;
  uint16_t message_id = 0;
  bool with_crc = false;
};

PosType findPacketMarker(std::iostream &stream, size_t byte_read_limit = 1024u)
{
  // Read the marker into a byte array and ensure it's in network endian.
  std::array<char, sizeof(kPacketMarker)> marker_validation_bytes;
  std::memcpy(marker_validation_bytes.data(), &kPacketMarker, marker_validation_bytes.size());
  networkEndianSwap(marker_validation_bytes);
  std::array<char, sizeof(kPacketMarker)> marker_bytes;
  static_assert(marker_validation_bytes.size() == marker_bytes.size());
  unsigned marker_bytes_checked = {};
  unsigned marker_bytes_validated = {};

  // Limit the number of bytes we try read in each attempt.
  while (byte_read_limit > 0)
  {
    --byte_read_limit;
    // Record the potential marker start position.
    auto stream_pos = stream.tellg();

    marker_bytes_checked = 0;
    marker_bytes_validated = 0;

    for (size_t i = 0; i < marker_bytes.size() && stream.good(); ++i)
    {
      stream.read(&marker_bytes[i], 1);
      // We've checked another byte.
      ++marker_bytes_checked;
      // Check the result. Loop will end when this doesn't track marker_bytes_checked
      if (marker_bytes[i] == marker_validation_bytes[i])
      {
        ++marker_bytes_validated;
      }
      else
      {
        // Failed to read a valid byte. Rewind the stream one byte if we've read more than one byte
        // as this could be the start of another marker sequence.
        if (i > 0)
        {
          stream.clear();
          stream.seekg(-1, std::ios_base::cur);
        }
        break;
      }
    }

    if (marker_bytes_checked == marker_bytes_validated &&
        marker_bytes_checked == marker_bytes.size())
    {
      // We've matched all bytes available.
      if (marker_bytes_checked == marker_bytes.size())
      {
        // All bytes matched.
        return stream_pos;
      }
    }
  }

  return -1;
}

bool getPacketInfo(std::iostream &stream, PacketInfo &info, std::vector<uint8_t> &header_buffer)
{
  info.stream_pos = stream.tellg();
  stream.read(reinterpret_cast<char *>(header_buffer.data()), sizeof(PacketHeader));
  auto bytes_read = stream.tellg() - info.stream_pos;
  if (bytes_read != sizeof(PacketHeader))
  {
    return false;
  }

  const auto *network_endian_header = reinterpret_cast<const PacketHeader *>(header_buffer.data());
  info.routing_id = networkEndianSwapValue(network_endian_header->routing_id);
  info.message_id = networkEndianSwapValue(network_endian_header->message_id);
  info.with_crc = (networkEndianSwapValue(network_endian_header->flags) & PFNoCrc) == 0;

  return true;
}

bool findInfoMessagePositions(std::iostream &stream, PacketInfo &server_info,
                              PacketInfo &frame_count_info, std::vector<uint8_t> &header_buffer)
{
  int attempts_remaining = 5;
  server_info.stream_pos = frame_count_info.stream_pos = -1;

  while (attempts_remaining > 0 && stream.good() &&
         (server_info.stream_pos < 0 || frame_count_info.stream_pos < 0))
  {
    --attempts_remaining;
    auto packet_start_pos = findPacketMarker(stream);
    if (packet_start_pos < 0)
    {
      continue;
    }

    // Found a packet start. Check if the packet type matches either server info or a frame count
    // control message.
    stream.seekg(packet_start_pos);

    PacketInfo packet_info = {};
    if (!getPacketInfo(stream, packet_info, header_buffer))
    {
      continue;
    }

    if (packet_info.routing_id == MtServerInfo)
    {
      if (server_info.stream_pos < 0)
      {
        server_info = packet_info;
      }
    }
    else if (packet_info.routing_id == MtControl && packet_info.message_id == CIdFrameCount)
    {
      if (frame_count_info.stream_pos < 0)
      {
        frame_count_info = packet_info;
      }
    }
  }

  return server_info.stream_pos >= 0 || frame_count_info.stream_pos >= 0;
}

/// Finalise the @c ServerInfoMessage.
///
/// The @p server_info is written at @p server_info_message_pos in @p stream provided both
/// @p serve_info and @p server_info_message_pos are valid. This allows dummy info to be written
/// when the stream starts, and correct the information later.
///
/// Note after calling this function with valid arguments, the stream position will have changed
/// to just after the @c ServerInfoMessage. It is the caller's responsibility to manage restoring
/// the stream position.
///
/// @note The @c ServerInfoMessage must have originally been written with Crc enabled.
///
/// @param stream  The stream to write to.
/// @param server_info The server information to write. May be null, in which case this function
/// does nothing.
/// @param server_message_info Information about the location of the @c ServerInfoMessage.
/// The position is -1 if there is no such message present, in which case this function does
/// nothing.
/// @param header_buffer A buffer used to compose the message to write.
void finaliseServerInfo(std::iostream &stream, const ServerInfoMessage *server_info,
                        const PacketInfo &server_message_info, std::vector<uint8_t> &header_buffer)
{
  // Rewrite server info in case it was just a place holder which was written before.
  if (!server_info || server_message_info.stream_pos < 0)
  {
    return;
  }

  // Estimate buffer size with padding.
  const size_t min_buffer_size =
    sizeof(PacketHeader) + sizeof(ServerInfoMessage) + sizeof(PacketWriter::CrcType) + 128;
  if (header_buffer.size() < min_buffer_size)
  {
    header_buffer.resize(min_buffer_size);
  }

  // Found the correct location. Seek the stream to here and write a new MtServerInfo message.
  stream.clear();
  stream.seekp(server_message_info.stream_pos);
  PacketWriter packet(header_buffer.data(), int_cast<uint16_t>(header_buffer.size()), MtServerInfo);
  // Set PFNoCrc flag if required
  if (!server_message_info.with_crc)
  {
    auto *header = reinterpret_cast<PacketHeader *>(header_buffer.data());
    header->flags = networkEndianSwapValue<decltype(PacketHeader::flags)>(PFNoCrc);
  }

  server_info->write(packet);
  packet.finalise();
  stream.write(reinterpret_cast<const char *>(header_buffer.data()), packet.packetSize());
  stream.flush();
}

/// Write the final frame count.
///
/// The @p frame_count is written to @p frame_count_message_pos provided the latter is valid.
/// This allows a dummy frame count to be written at the beginning of the stream, and update it
/// when the actual count is known.
///
/// Note after calling this function with valid arguments, the stream position will have changed
/// to just after the @c MtControl, @c CIdFrameCount message. It is the caller's responsibility to
/// manage restoring the stream position.
///
/// @note The frame count @c ControlMessage must have originally been written with Crc enabled.
///
/// @param stream The stream to write to.
/// @param frame_count the frame count to write.
/// @param frame_count_info Information about the location of the frame count @c ControlMessage.
/// The position is -1 if there is no such message present, in which case this function does
/// nothing.
/// @param header_buffer A buffer used to compose the message to write.
void finaliseFrameCount(std::iostream &stream, uint32_t frame_count,
                        const PacketInfo &frame_count_info, std::vector<uint8_t> &header_buffer)
{
  if (frame_count_info.stream_pos < 0)
  {
    return;
  }

  // Estimate buffer size with padding.
  const size_t min_buffer_size =
    sizeof(PacketHeader) + sizeof(ControlMessage) + sizeof(PacketWriter::CrcType) + 128;
  if (header_buffer.size() < min_buffer_size)
  {
    header_buffer.resize(min_buffer_size);
  }

  // Found the correct location. Seek the stream to here and write a new FrameCount control
  // message.
  stream.clear();
  stream.seekp(frame_count_info.stream_pos);

  PacketWriter packet(header_buffer.data(), int_cast<uint16_t>(header_buffer.size()), MtControl,
                      CIdFrameCount);
  // Set PFNoCrc flag if required
  if (!frame_count_info.with_crc)
  {
    auto *header = reinterpret_cast<PacketHeader *>(header_buffer.data());
    header->flags = networkEndianSwapValue<decltype(PacketHeader::flags)>(PFNoCrc);
  }

  ControlMessage frame_count_msg;

  frame_count_msg.control_flags = 0;
  frame_count_msg.value32 = frame_count;
  frame_count_msg.value64 = 0;

  frame_count_msg.write(packet);
  packet.finalise();
  stream.write(reinterpret_cast<const char *>(header_buffer.data()), packet.packetSize());
  stream.flush();
}
}  // namespace

bool initialiseStream(std::ostream &stream, const ServerInfoMessage *server_info)
{
  const uint16_t packet_buffer_size = 256;
  std::array<uint8_t, packet_buffer_size> packet_buffer;
  PacketWriter packet(packet_buffer.data(), int_cast<uint16_t>(packet_buffer.size()));

  if (server_info)
  {
    packet.reset(MtServerInfo, 0);
    if (!server_info->write(packet))
    {
      return false;
    }

    if (!packet.finalise())
    {
      return false;
    }

    stream.write(reinterpret_cast<const char *>(packet.data()), packet.packetSize());
    if (!stream.good() || stream.fail())
    {
      return false;
    }
  }

  // Write a frame count control message place holder.
  packet.reset(MtControl, CIdFrameCount);
  ControlMessage msg;
  msg.control_flags = 0;
  msg.value32 = 0;
  msg.value64 = 0;

  if (!msg.write(packet))
  {
    return false;
  }

  if (!packet.finalise())
  {
    return false;
  }

  stream.write(reinterpret_cast<const char *>(packet.data()), packet.packetSize());
  return stream.good() && !stream.fail();
}


bool finaliseStream(std::iostream &stream, uint32_t frame_count,
                    const ServerInfoMessage *server_info)
{
  // Rewind the stream to the beginning and find the first RoutingID.ServerInfo message
  // and RoutingID.Control message with a ControlMessageID.FrameCount ID. These should be
  // the first and second messages in the stream.
  // We'll limit searching to the first 5 messages.
  using PosType = std::remove_reference_t<decltype(stream)>::pos_type;
  stream.flush();

  // Record the initial stream position to restore later.
  const PosType restore_pos = stream.tellp();

  // Set the read position to search for the relevant messages.
  stream.clear();
  stream.seekg(0);

  std::vector<uint8_t> header_buffer(1024);
  PacketInfo sever_info;
  PacketInfo frame_count_info;
  const bool found = findInfoMessagePositions(stream, sever_info, frame_count_info, header_buffer);
  if (found)
  {
    finaliseServerInfo(stream, server_info, sever_info, header_buffer);
    finaliseFrameCount(stream, frame_count, frame_count_info, header_buffer);
  }

  if (restore_pos > 0)
  {
    stream.seekp(restore_pos);
    stream.seekg(restore_pos);
    stream.flush();
  }

  return stream.good() && found;
}
}  // namespace tes::streamutil
