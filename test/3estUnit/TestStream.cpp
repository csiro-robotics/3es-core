//
// author: Kazys Stepanas
//

#include "TestCommon.h"

#include <3escore/CoordinateFrame.h>
#include <3escore/CoreUtil.h>
#include <3escore/Messages.h>
#include <3escore/PacketBuffer.h>
#include <3escore/PacketReader.h>
#include <3escore/PacketStreamReader.h>
#include <3escore/StreamUtil.h>

#include <gtest/gtest.h>

#include <array>
#include <iterator>
#include <memory>
#include <sstream>

namespace tes
{
/// Read a @c ServerInfoMessage and @p fame_count from the current position in @p stream.
/// @param stream The stream to read from.
/// @param server_info Structure to read into.
/// @param frame_count Frame count to read into.
/// @return true on success.
bool readStreamInfo(std::shared_ptr<std::iostream> stream, ServerInfoMessage &server_info,
                    uint32_t &frame_count)
{
  PacketStreamReader stream_reader(stream);
  const PacketHeader *header = stream_reader.extractPacket();

  if (!header)
  {
    return false;
  }

  PacketReader reader(header);

  if (reader.routingId() != MtServerInfo && reader.messageId() != 0)
  {
    return false;
  }

  if (!server_info.read(reader))
  {
    return false;
  }

  // Next read frame count.
  header = stream_reader.extractPacket();

  if (!header)
  {
    return false;
  }

  reader = PacketReader(header);

  if (reader.routingId() != MtControl && reader.messageId() != CIdFrameCount)
  {
    return false;
  }

  ControlMessage msg;
  if (!msg.read(reader))
  {
    return false;
  }

  frame_count = msg.value32;
  return true;
}


TEST(Stream, Util)
{
  // Test stream init/finalisation in the tes::streamutil namespace.
  std::shared_ptr<std::stringstream> stream_ptr = std::make_shared<std::stringstream>();
  std::stringstream &stream = *stream_ptr;

  const uint32_t expect_frame_count = 42u;
  ServerInfoMessage expected_info = { 101, 202, ZYX, { 0u } };

  // First write some rubbish to the stream in order to set prime it. We'll include writing
  // part of the packet marker at the start, but not complete the packet.
  std::array<char, sizeof(kPacketMarker)> marker_bytes;
  std::memcpy(marker_bytes.data(), &kPacketMarker, sizeof(kPacketMarker));
  networkEndianSwap(marker_bytes);  // Ensure network endian.
  // Write all but one marker byte.
  static_assert(sizeof(kPacketMarker) > 1, "Expecting at least 2 marker bytes.");
  // Replace the last marker byte with an invalid value.
  marker_bytes.back() += 1;
  stream.write(marker_bytes.data(), marker_bytes.size());

  const auto server_info_pos = stream.tellp();

  // Now initialise the stream.
  streamutil::initialiseStream(stream, &expected_info);
  stream.flush();

  // Check we've written placeholder messages correctly.
  const auto stream_end = stream.tellp();

  ServerInfoMessage server_info = {};
  uint32_t frame_count = ~0u;  // Initialise to non zero value. We'll validate for zero later.

  stream.clear();
  // For the first read we'll seek to the expected position and start from there.
  stream.seekg(server_info_pos);
  ASSERT_TRUE(readStreamInfo(stream_ptr, server_info, frame_count));

  EXPECT_EQ(server_info.time_unit, expected_info.time_unit);
  EXPECT_EQ(server_info.default_frame_time, expected_info.default_frame_time);
  EXPECT_EQ(server_info.coordinate_frame, expected_info.coordinate_frame);
  EXPECT_EQ(frame_count, 0);

  // Change the server info before we finalise.
  initDefaultServerInfo(&expected_info);

  // Finalise the stream with a new frame count.
  ASSERT_TRUE(streamutil::finaliseStream(stream, expect_frame_count, &expected_info));
  stream.clear();
  // This time we'll seek to the stream start where we have a partial packet marker.
  // We expect the PacketStreamReader to correctly skip this section.
  stream.seekg(0);
  ASSERT_TRUE(readStreamInfo(stream_ptr, server_info, frame_count));

  EXPECT_EQ(server_info.time_unit, expected_info.time_unit);
  EXPECT_EQ(server_info.default_frame_time, expected_info.default_frame_time);
  EXPECT_EQ(server_info.coordinate_frame, expected_info.coordinate_frame);
  EXPECT_EQ(frame_count, expect_frame_count);
}

TEST(Stream, PacketBuffer)
{
  // Setup a stream.
  std::shared_ptr<std::stringstream> stream_ptr = std::make_shared<std::stringstream>();
  std::stringstream &stream = *stream_ptr;
  ServerInfoMessage expected_server_info = {};
  const uint32_t expected_frame_count = 42u;
  initDefaultServerInfo(&expected_server_info);
  streamutil::initialiseStream(stream, &expected_server_info);
  ASSERT_TRUE(streamutil::finaliseStream(stream, expected_frame_count, &expected_server_info));
  stream.flush();

  std::vector<uint8_t> buffer;
  std::vector<uint8_t> secondary_buffer;
  const auto str = stream.str();
  std::copy(str.begin(), str.end(), back_inserter(buffer));

  ServerInfoMessage restored_info = {};
  uint32_t final_frame_count = 0;
  const auto handle_packet = [this, &restored_info,
                              &final_frame_count](const PacketHeader *header) {
    PacketReader reader(header);
    switch (reader.routingId())
    {
    case MtServerInfo:
      EXPECT_TRUE(restored_info.read(reader));
      break;
    case MtControl:
      if (reader.messageId() == CIdFrameCount)
      {
        ControlMessage msg;
        EXPECT_TRUE(msg.read(reader));
        final_frame_count = msg.value32;
        break;
      }
      TES_FALLTHROUGH;
    default:
      FAIL() << "Unexpected message type.";
      break;
    }
  };

  // We have our memory buffer. Now start migrating data from this to a PacketBuffer.
  // We'll copy 16 byte blocks to simulate partial reads.
  PacketBuffer packet_buffer;

  for (size_t i = 0; i < buffer.size(); i += 16)
  {
    const size_t copy_count = std::min<size_t>(16u, buffer.size() - i);
    EXPECT_NE(packet_buffer.addBytes(buffer.data() + i, copy_count), -1);

    // Check for packet completion.
    secondary_buffer.clear();
    auto *packet = packet_buffer.extractPacket(secondary_buffer);
    if (packet)
    {
      handle_packet(packet);
    }
  }

  EXPECT_EQ(restored_info.time_unit, expected_server_info.time_unit);
  EXPECT_EQ(restored_info.default_frame_time, expected_server_info.default_frame_time);
  EXPECT_EQ(restored_info.coordinate_frame, expected_server_info.coordinate_frame);
  EXPECT_EQ(final_frame_count, expected_frame_count);
}
}  // namespace tes
