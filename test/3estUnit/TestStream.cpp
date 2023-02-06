//
// author: Kazys Stepanas
//

#include "TestCommon.h"

#include <3escore/CoordinateFrame.h>
#include <3escore/Messages.h>
#include <3escore/StreamUtil.h>
#include <3escore/PacketStreamReader.h>

#include <gtest/gtest.h>

#include <array>
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
  ServerInfoMessage expected_info = {
    101,
    202,
    ZYX,
  };

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
}  // namespace tes
