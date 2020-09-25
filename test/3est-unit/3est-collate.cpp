//
// author: Kazys Stepanas
//

#include "3est-common.h"

#include <3esconnectionmonitor.h>
#include <3escoordinateframe.h>
#include <3esmaths.h>
#include <3esmathsstream.h>
#include <3esmessages.h>
#include <3espacketbuffer.h>
#include <3espacketreader.h>
#include <3espacketwriter.h>
#include <3esserver.h>
#include <3esserverutil.h>
#include <3estcplistensocket.h>
#include <3estcpsocket.h>
#include <shapes/3espointcloud.h>
#include <shapes/3esshapes.h>
#include <shapes/3essimplemesh.h>
#include <tessellate/3essphere.h>

#include <3escollatedpacket.h>
#include <3escollatedpacketdecoder.h>

#include <gtest/gtest.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>
#include "3escollatedpacketdecoder.h"

namespace tes
{
void collationTest(bool compress, CollatedPacketDecoder *decoderOverride = nullptr)
{
  // Allocate an excessively large packet (not for network transfer).
  CollatedPacket encoder(compress);
  CollatedPacketDecoder localDecoder;
  CollatedPacketDecoder &decoder = (decoderOverride) ? *decoderOverride : localDecoder;

  // Create a mesh object to generate some messages.
  std::vector<Vector3f> vertices;
  std::vector<unsigned> indices;
  std::vector<Vector3f> normals;
  // Use a low res sphere to ensure we fit the data in a single data packet.
  makeLowResSphere(vertices, indices, &normals);

  // I> Test each constructor.
  // 1. drawType, verts, vcount, vstrideBytes, pos, rot, scale
  MeshShape referenceMesh(
    DtTriangles, ShapeId(42u, 1), DataBuffer(vertices), DataBuffer(indices),
    Transform(Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f(1, 1, 1), degToRad(18.0f)),
              Vector3f(1.0f, 1.2f, 0.8f)));
  referenceMesh.setNormals(DataBuffer(normals));

  // Use the encoder as a connection.
  // The create() call will pack the mesh create message and multiple data messages.
  ASSERT_GT(encoder.create(referenceMesh), 0);
  ASSERT_TRUE(encoder.finalise());

  unsigned byteCount = 0;
  const PacketHeader *encoded = reinterpret_cast<const PacketHeader *>(encoder.buffer(byteCount));

  // Decode the packet into a new mesh.
  MeshShape readMesh;
  ASSERT_TRUE(decoder.setPacket(encoded));
  EXPECT_TRUE(decoder.decoding());
  EXPECT_EQ(decoder.targetBytes(), encoder.collatedBytes());
  EXPECT_EQ(decoder.decodedBytes(), 0);

  uint32_t shapeId = 0;
  while (const PacketHeader *packet = decoder.next())
  {
    PacketReader reader(packet);

    EXPECT_EQ(reader.marker(), PacketMarker);
    EXPECT_EQ(reader.versionMajor(), PacketVersionMajor);
    EXPECT_EQ(reader.versionMinor(), PacketVersionMinor);

    ASSERT_EQ(reader.routingId(), referenceMesh.routingId());

    // Peek the shape ID.
    reader.peek((uint8_t *)&shapeId, sizeof(shapeId));
    EXPECT_EQ(shapeId, referenceMesh.id());

    switch (reader.messageId())
    {
    case OIdCreate:
      EXPECT_TRUE(readMesh.readCreate(reader));
      break;

    case OIdUpdate:
      EXPECT_TRUE(readMesh.readUpdate(reader));
      break;

    case OIdData:
      EXPECT_TRUE(readMesh.readData(reader));
      break;
    }
  }

  EXPECT_FALSE(decoder.decoding());
  EXPECT_GT(decoder.decodedBytes(), 0u);
  EXPECT_EQ(decoder.decodedBytes(), decoder.targetBytes());

  // Validate we've read what we wrote.
  ResourceMap resources;
  validateShape(readMesh, referenceMesh, resources);
}


void singlePacketTest(CollatedPacketDecoder *decoderOverride = nullptr)
{
  std::vector<uint8_t> buffer(16 * 1024);
  PacketWriter writer(buffer.data(), (uint16_t)buffer.size());

  // Create a single packet message.
  ControlMessage ctrlMsg;
  memset(&ctrlMsg, 0, sizeof(ctrlMsg));
  ctrlMsg.value32 = 42;
  ctrlMsg.value64 = 42;
  writer.reset(MtControl, CIdEnd);
  ASSERT_TRUE(ctrlMsg.write(writer));

  // Give the packet to a decoder.
  CollatedPacketDecoder localDecoder(&writer.packet());
  CollatedPacketDecoder &decoder = (decoderOverride) ? *decoderOverride : localDecoder;
  if (decoderOverride)
  {
    decoderOverride->setPacket(&writer.packet());
  }

  ASSERT_TRUE(decoder.decoding());
  EXPECT_EQ(decoder.decodedBytes(), 0u);
  EXPECT_EQ(decoder.targetBytes(), writer.payloadSize());

  const PacketHeader *packet = decoder.next();
  ASSERT_NE(packet, nullptr);
  EXPECT_FALSE(decoder.decoding());
  EXPECT_EQ(packet, &writer.packet());
  EXPECT_GT(decoder.decodedBytes(), 0u);
  EXPECT_EQ(decoder.decodedBytes(), decoder.targetBytes());

  // Reader scope.
  {
    PacketReader reader(packet);
    EXPECT_EQ(reader.marker(), PacketMarker);
    EXPECT_EQ(reader.versionMajor(), PacketVersionMajor);
    EXPECT_EQ(reader.versionMinor(), PacketVersionMinor);

    EXPECT_EQ(reader.routingId(), MtControl);
    EXPECT_EQ(reader.messageId(), CIdEnd);

    memset(&ctrlMsg, 0, sizeof(ctrlMsg));
    EXPECT_TRUE(ctrlMsg.read(reader));

    EXPECT_EQ(ctrlMsg.value32, 42);
    EXPECT_EQ(ctrlMsg.value64, 42);
  }

  packet = decoder.next();
  EXPECT_EQ(packet, nullptr);
  EXPECT_FALSE(decoder.decoding());
  EXPECT_EQ(decoder.decodedBytes(), writer.payloadSize());
  EXPECT_EQ(decoder.targetBytes(), writer.payloadSize());
}


TEST(Collate, Uncompressed)
{
  collationTest(false);
}


TEST(Collate, Compressed)
{
  collationTest(true);
}

TEST(Collate, Reuse)
{
  CollatedPacketDecoder decoder;
  singlePacketTest(&decoder);
  collationTest(false, &decoder);
  singlePacketTest(&decoder);
}

TEST(Collate, DecodeSinglePacket)
{
  singlePacketTest();
}
}  // namespace tes
