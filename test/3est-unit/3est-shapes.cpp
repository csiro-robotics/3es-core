//
// author: Kazys Stepanas
//

#include "3est-common.h"

#include <3escore/CollatedPacketDecoder.h>
#include <3escore/ConnectionMonitor.h>
#include <3escore/CoordinateFrame.h>
#include <3escore/Maths.h>
#include <3escore/MathsStream.h>
#include <3escore/Messages.h>
#include <3escore/PacketBuffer.h>
#include <3escore/PacketReader.h>
#include <3escore/PacketWriter.h>
#include <3escore/Server.h>
#include <3escore/ServerUtil.h>
#include <3escore/shapes/Pointcloud.h>
#include <3escore/shapes/Shapes.h>
#include <3escore/shapes/SimpleMesh.h>
#include <3escore/TcpSocket.h>

#include <gtest/gtest.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>

namespace tes
{
template <class T>
void handleShapeMessage(PacketReader &reader, T &shape, const T &referenceShape)
{
  // Shape message the shape.
  uint32_t shapeId = 0;

  // Peek the shape ID.
  reader.peek((uint8_t *)&shapeId, sizeof(shapeId));

  EXPECT_EQ(shapeId, referenceShape.id());

  switch (reader.messageId())
  {
  case OIdCreate:
    EXPECT_TRUE(shape.readCreate(reader));
    break;

  case OIdUpdate:
    EXPECT_TRUE(shape.readUpdate(reader));
    break;

  case OIdData:
    EXPECT_TRUE(shape.readData(reader));
    break;
  }
}

void handleMeshMessage(PacketReader &reader, ResourceMap &resources)
{
  uint32_t meshId = 0;
  reader.peek((uint8_t *)&meshId, sizeof(meshId));
  auto resIter = resources.find(MeshPlaceholder(meshId).uniqueKey());
  SimpleMesh *mesh = nullptr;

  // If it exists, make sure it's a mesh.
  if (resIter != resources.end())
  {
    ASSERT_TRUE(resIter->second->typeId() == MtMesh);
    mesh = static_cast<SimpleMesh *>(resIter->second);
  }

  switch (reader.messageId())
  {
  case MmtInvalid:
    EXPECT_TRUE(false) << "Invalid mesh message sent";
    break;

  case MmtDestroy:
    delete mesh;
    if (resIter != resources.end())
    {
      resources.erase(resIter);
    }
    break;

  case MmtCreate:
    // Create message. Should not already exists.
    EXPECT_EQ(mesh, nullptr) << "Recreating exiting mesh.";
    delete mesh;
    mesh = new SimpleMesh(meshId);
    EXPECT_TRUE(mesh->readCreate(reader));
    resources.insert(std::make_pair(mesh->uniqueKey(), mesh));
    break;

  // Not handling these messages.
  case MmtRedefine:
  case MmtFinalise:
    break;

  default:
    EXPECT_NE(mesh, nullptr);
    if (mesh)
    {
      EXPECT_TRUE(mesh->readTransfer(reader.messageId(), reader));
    }
    break;
  }
}

typedef std::function<int(uint8_t *buffer, int bufferLength)> DataReadFunc;

template <class T>
void validateDataRead(const DataReadFunc &dataRead, const T &referenceShape, const ServerInfoMessage &serverInfo,
                      unsigned timeoutSec = 10)
{
  typedef std::chrono::steady_clock Clock;
  ServerInfoMessage readServerInfo;
  std::vector<uint8_t> readBuffer(0xffffu);
  std::vector<uint8_t> decodeBuffer(0xffffu);
  ResourceMap resources;
  PacketBuffer packetBuffer;
  CollatedPacketDecoder decoder;
  T shape;
  auto startTime = Clock::now();
  int readCount = 0;
  bool endMsgReceived = false;
  bool serverInfoRead = false;
  bool shapeMsgRead = false;

  memset(&readServerInfo, 0, sizeof(readServerInfo));

  // Keep looping until we get a CIdEnd ControlMessage or timeoutSec elapses.
  while (!endMsgReceived &&
         std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - startTime).count() < timeoutSec)
  {
    readCount = dataRead(readBuffer.data(), int(readBuffer.size()));
    // Assert no read errors.
    ASSERT_TRUE(readCount >= 0);
    if (readCount < 0)
    {
      break;
    }

    if (readCount == 0)
    {
      // Nothing read. Wait.
      std::this_thread::yield();
      continue;
    }

    packetBuffer.addBytes(readBuffer.data(), unsigned(readCount));

    while (const PacketHeader *primaryPacket = packetBuffer.extractPacket(decodeBuffer))
    {
      decoder.setPacket(primaryPacket);

      while (const PacketHeader *packetHeader = decoder.next())
      {
        PacketReader reader(packetHeader);

        EXPECT_EQ(reader.marker(), PacketMarker);
        EXPECT_EQ(reader.versionMajor(), PacketVersionMajor);
        EXPECT_EQ(reader.versionMinor(), PacketVersionMinor);

        switch (reader.routingId())
        {
        case MtServerInfo:
          serverInfoRead = true;
          readServerInfo.read(reader);

          // Validate server info.
          EXPECT_EQ(readServerInfo.timeUnit, serverInfo.timeUnit);
          EXPECT_EQ(readServerInfo.defaultFrameTime, serverInfo.defaultFrameTime);
          EXPECT_EQ(readServerInfo.coordinateFrame, serverInfo.coordinateFrame);

          for (int i = 0; i < int(sizeof(readServerInfo.reserved) / sizeof(readServerInfo.reserved[0])); ++i)
          {
            EXPECT_EQ(readServerInfo.reserved[i], serverInfo.reserved[i]);
          }
          break;

        case MtControl: {
          // Only interested in the CIdEnd message to mark the end of the
          // stream.
          ControlMessage msg;
          ASSERT_TRUE(msg.read(reader));

          if (reader.messageId() == CIdEnd)
          {
            endMsgReceived = true;
          }
          break;
        }

        case MtMesh:
          handleMeshMessage(reader, resources);
          break;

        default:
          if (reader.routingId() == referenceShape.routingId())
          {
            shapeMsgRead = true;
            handleShapeMessage(reader, shape, referenceShape);
          }
          break;
        }
      }
    }
    // else fail?
  }

  EXPECT_GT(readCount, -1);
  EXPECT_TRUE(serverInfoRead);
  EXPECT_TRUE(shapeMsgRead);
  EXPECT_TRUE(endMsgReceived);

  // Validate the shape state.
  if (shapeMsgRead)
  {
    validateShape(shape, referenceShape, resources);
  }

  for (auto &&resource : resources)
  {
    delete resource.second;
  }
}

template <class T>
void validateClient(TcpSocket &socket, const T &referenceShape, const ServerInfoMessage &serverInfo,
                    unsigned timeoutSec = 10)
{
  const DataReadFunc socketRead = [&socket](uint8_t *buffer, int bufferLength) {
    return socket.readAvailable(buffer, bufferLength);
  };
  validateDataRead(socketRead, referenceShape, serverInfo, timeoutSec);
}

template <class T>
void testShape(const T &shape, ServerInfoMessage *infoOut = nullptr, const char *saveFilePath = nullptr)
{
  // Initialise server.
  ServerInfoMessage info;
  initDefaultServerInfo(&info);
  info.coordinateFrame = XYZ;

  unsigned serverFlags = SF_Default | SF_CollateAndCompress;
  ServerSettings serverSettings(serverFlags);
  serverSettings.portRange = 1000;
  Server *server = Server::create(serverSettings, &info);

  if (infoOut)
  {
    *infoOut = info;
  }

  // std::cout << "Start on port " << serverSettings.listenPort << std::endl;
  ASSERT_TRUE(server->connectionMonitor()->start(tes::ConnectionMonitor::Asynchronous));
  // std::cout << "Server listening on port " <<
  // server->connectionMonitor()->port() << std::endl;;

  // Create client and connect.
  TcpSocket client;
  client.open("127.0.0.1", server->connectionMonitor()->port());

  // Wait for connection.
  if (server->connectionMonitor()->waitForConnection(5000U) > 0)
  {
    server->connectionMonitor()->commitConnections();
  }

  // Setup saving to file.
  if (saveFilePath && saveFilePath[0])
  {
    Connection *fileConnection = server->connectionMonitor()->openFileStream(saveFilePath);
    ASSERT_NE(fileConnection, nullptr);
    server->connectionMonitor()->commitConnections();
  }

  EXPECT_GT(server->connectionCount(), 0u);
  EXPECT_TRUE(client.isConnected());

  // Send server messages from another thread. Otherwise large packets may
  // block.
  std::thread sendThread([server, &shape]() {
    server->create(shape);
    server->updateTransfers(0);
    server->updateFrame(0.0f, true);

    // Send end message.
    ControlMessage ctrlMsg;
    memset(&ctrlMsg, 0, sizeof(ctrlMsg));
    sendMessage(*server, MtControl, CIdEnd, ctrlMsg, false);
  });

  // Process client messages.
  validateClient(client, shape, info);

  client.close();

  sendThread.join();
  server->close();

  server->connectionMonitor()->stop();
  server->connectionMonitor()->join();

  server->dispose();
  server = nullptr;
}

template <class T>
void validateFileStream(const char *fileName, const T &referenceShape, const ServerInfoMessage &serverInfo)
{
  // Load a file stream and validate the shape it generates.
  std::ifstream inFile(fileName, std::ios::binary);

  ASSERT_TRUE(inFile.is_open()) << "Failed to read file '" << fileName << "'";
  const DataReadFunc fileRead = [&inFile](uint8_t *buffer, int bufferLength) {
#if WIN32
    inFile.read(reinterpret_cast<char *>(buffer), bufferLength);
    const auto readBytes = inFile.gcount();
#else   // WIN32
    const auto readBytes = inFile.readsome(reinterpret_cast<char *>(buffer), bufferLength);
#endif  // WIN32
    if (inFile.bad())
    {
      return -1;
    }
    return int(readBytes);
  };

  validateDataRead(fileRead, referenceShape, serverInfo);
}

TEST(Shapes, Arrow)
{
  testShape(Arrow(42u, Directional(Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), 0.05f, 2.0f)));
  testShape(Arrow(Id(42u, 1), Directional(Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), 0.05f, 2.0f)));
}

TEST(Shapes, Box)
{
  testShape(Box(
    42u, Transform(Vector3f(1.2f, 2.3f, 3.4f),
                   Quaternionf().setAxisAngle(Vector3f(1, 1, 1).normalised(), degToRad(18.0f)), Vector3f(1, 3, 2))));
  testShape(Box(Id(42u, 1), Transform(Vector3f(1.2f, 2.3f, 3.4f),
                                      Quaternionf().setAxisAngle(Vector3f(1, 1, 1).normalised(), degToRad(18.0f)),
                                      Vector3f(1, 3, 2))));
}

TEST(Shapes, Capsule)
{
  testShape(Capsule(42u, Directional(Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), 0.3f, 2.05f)));
  testShape(Capsule(Id(42u, 1), Directional(Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), 0.3f, 2.05f)));
}

TEST(Shapes, Cone)
{
  testShape(Cone(Id(42u), Directional(Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), 0.35f, 3.0f)));
  testShape(Cone(Id(42u, 1), Directional(Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), 0.35f, 3.0f)));
}

TEST(Shapes, Cylinder)
{
  testShape(Cylinder(Id(42u), Directional(Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), 0.25f, 1.05f)));
  testShape(
    Cylinder(Id(42u, 1), Directional(Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), 0.25f, 1.05f)));
}

TEST(Shapes, MeshSet)
{
  std::vector<Vector3f> vertices;
  std::vector<unsigned> indices;
  std::vector<unsigned> wireIndices;
  std::vector<Vector3f> normals;
  std::vector<uint32_t> colours;
  std::vector<SimpleMesh *> meshes;
  makeHiResSphere(vertices, indices, &normals);

  // Build per vertex colours by colour cycling.
  colours.resize(vertices.size());
  for (size_t i = 0; i < vertices.size(); ++i)
  {
    colours[i] = Colour::cycle(unsigned(i)).c;
  }

  // Build a line based indexing scheme for a wireframe sphere.
  for (size_t i = 0; i < indices.size(); i += 3)
  {
    wireIndices.push_back(indices[i + 0]);
    wireIndices.push_back(indices[i + 1]);
    wireIndices.push_back(indices[i + 1]);
    wireIndices.push_back(indices[i + 2]);
    wireIndices.push_back(indices[i + 2]);
    wireIndices.push_back(indices[i + 0]);
  }

  // Build a number of meshes to include in the mesh set.
  SimpleMesh *mesh = nullptr;
  unsigned nextMeshId = 1;

  // Vertices and indices only.
  mesh = new SimpleMesh(nextMeshId++, unsigned(vertices.size()), unsigned(indices.size()), DtTriangles);
  mesh->setVertices(0, vertices.data(), unsigned(vertices.size()));
  mesh->setIndices(0, indices.data(), unsigned(indices.size()));
  meshes.push_back(mesh);

  // Vertices, indices and colours
  mesh = new SimpleMesh(nextMeshId++, unsigned(vertices.size()), unsigned(indices.size()), DtTriangles,
                        SimpleMesh::Vertex | SimpleMesh::Index | SimpleMesh::Colour);
  mesh->setVertices(0, vertices.data(), unsigned(vertices.size()));
  mesh->setNormals(0, normals.data(), unsigned(normals.size()));
  mesh->setColours(0, colours.data(), unsigned(colours.size()));
  mesh->setIndices(0, indices.data(), unsigned(indices.size()));
  meshes.push_back(mesh);

  // Points and colours only (essentially a point cloud)
  mesh = new SimpleMesh(nextMeshId++, unsigned(vertices.size()), unsigned(indices.size()), DtPoints,
                        SimpleMesh::Vertex | SimpleMesh::Colour);
  mesh->setVertices(0, vertices.data(), unsigned(vertices.size()));
  mesh->setColours(0, colours.data(), unsigned(colours.size()));
  meshes.push_back(mesh);

  // Lines.
  mesh = new SimpleMesh(nextMeshId++, unsigned(vertices.size()), unsigned(wireIndices.size()), DtLines);
  mesh->setVertices(0, vertices.data(), unsigned(vertices.size()));
  mesh->setIndices(0, wireIndices.data(), unsigned(wireIndices.size()));
  meshes.push_back(mesh);

  // One with the lot.
  mesh = new SimpleMesh(nextMeshId++, unsigned(vertices.size()), unsigned(indices.size()), DtTriangles,
                        SimpleMesh::Vertex | SimpleMesh::Index | SimpleMesh::Normal | SimpleMesh::Colour);
  mesh->setVertices(0, vertices.data(), unsigned(vertices.size()));
  mesh->setNormals(0, normals.data(), unsigned(normals.size()));
  mesh->setColours(0, colours.data(), unsigned(colours.size()));
  mesh->setIndices(0, indices.data(), unsigned(indices.size()));
  meshes.push_back(mesh);

  // First do a single part MeshSet.
  testShape(MeshSet(meshes[0], 42u));

  // Now a multi-part MeshSet.
  {
    MeshSet set(Id(42u, 1), meshes.size());

    Matrix4f transform = Matrix4f::identity;
    for (unsigned i = 0; i < meshes.size(); ++i)
    {
      const float fi = float(i);
      transform = prsTransform(Vector3f(fi * 1.0f, fi - 3.2f, 1.5f * fi),
                               Quaternionf().setAxisAngle(Vector3f(fi * 1.0f, fi + 1.0f, fi - 3.0f).normalised(),
                                                          degToRad((fi + 1.0f) * 6.0f)),
                               Vector3f(0.75f, 0.75f, 0.75f));
      set.setPart(i, meshes[i], transform);
    }
    testShape(set);
  }

  for (auto &&mesh : meshes)
  {
    delete mesh;
  }
}

TEST(Shapes, Mesh)
{
  std::vector<Vector3f> vertices;
  std::vector<unsigned> indices;
  std::vector<Vector3f> normals;
  makeHiResSphere(vertices, indices, &normals);

  // Build a colour cycle for per-vertex colours.
  std::vector<uint32_t> colours(vertices.size());
  for (unsigned i = 0; i < unsigned(colours.size()); ++i)
  {
    colours[i] = tes::Colour::cycle(i).c;
  }

  // I> Test each constructor.
  // 1. drawType, verts, vcount, vstrideBytes, pos, rot, scale
  testShape(
    MeshShape(DtPoints, Id(), DataBuffer(vertices),
              Transform(Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f(1, 1, 1), degToRad(18.0f)),
                        Vector3f(1.0f, 1.2f, 0.8f))));
  // 2. drawType, verts, vcount, vstrideBytes, indices, icount, pos, rot, scale
  testShape(
    MeshShape(DtTriangles, Id(), DataBuffer(vertices), DataBuffer(indices),
              Transform(Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f(1, 1, 1), degToRad(18.0f)),
                        Vector3f(1.0f, 1.2f, 0.8f))));
  // 3. drawType, verts, vcount, vstrideBytes, id, pos, rot, scale
  testShape(
    MeshShape(DtPoints, Id(42u), DataBuffer(vertices),
              Transform(Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f(1, 1, 1), degToRad(18.0f)),
                        Vector3f(1.0f, 1.2f, 0.8f))));
  // 4. drawType, verts, vcount, vstrideBytes, indices, icount, id, pos, rot,
  // scale
  testShape(
    MeshShape(DtTriangles, Id(42u), DataBuffer(vertices), DataBuffer(indices),
              Transform(Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f(1, 1, 1), degToRad(18.0f)),
                        Vector3f(1.0f, 1.2f, 0.8f))));
  // 5. drawType, verts, vcount, vstrideBytes, indices, icount, id, cat, pos,
  // rot, scale
  testShape(
    MeshShape(DtTriangles, Id(42u), DataBuffer(vertices), DataBuffer(indices),
              Transform(Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f(1, 1, 1), degToRad(18.0f)),
                        Vector3f(1.0f, 1.2f, 0.8f))));

  // II> Test with uniform normal.
  testShape(
    MeshShape(DtVoxels, Id(42u), DataBuffer(vertices), DataBuffer(indices),
              Transform(Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f(1, 1, 1), degToRad(18.0f)),
                        Vector3f(1.0f, 1.2f, 0.8f)))
      .setUniformNormal(Vector3f(0.1f, 0.1f, 0.1f)));

  // III> Test will many normals.
  testShape(
    MeshShape(DtTriangles, Id(42u, 1), DataBuffer(vertices), DataBuffer(indices),
              Transform(Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f(1, 1, 1), degToRad(18.0f)),
                        Vector3f(1.0f, 1.2f, 0.8f)))
      .setNormals(DataBuffer(normals)));

  // IV> Test with colours.
  testShape(
    MeshShape(DtTriangles, Id(), DataBuffer(vertices), DataBuffer(indices),
              Transform(Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f(1, 1, 1), degToRad(18.0f)),
                        Vector3f(1.0f, 1.2f, 0.8f)))
      .setColours(colours.data()));
}

TEST(Shapes, Plane)
{
  testShape(Plane(Id(42u), Directional(Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), 5.0f, 0.75f)));
  testShape(Plane(Id(42u, 1), Directional(Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), 5.0f, 0.75f)));
}

TEST(Shapes, PointCloud)
{
  std::vector<Vector3f> vertices;
  std::vector<unsigned> indices;
  std::vector<Vector3f> normals;
  makeHiResSphere(vertices, indices, &normals);

  PointCloud cloud(42);
  cloud.addPoints(vertices.data(), unsigned(vertices.size()));

  // Full res cloud.
  testShape(PointCloudShape(&cloud, Id(42u), 8));

  // Indexed (sub-sampled) cloud. Just use half the points.
  indices.clear();
  for (unsigned i = 0; i < unsigned(vertices.size() / 2); ++i)
  {
    indices.push_back(i);
  }
  testShape(PointCloudShape(&cloud, Id(42u, 1), 8).setIndices(indices.data(), unsigned(indices.size())));
}

TEST(Shapes, Pose)
{
  testShape(
    Pose(Id(42u), Transform(Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f::axisz, float(0.25 * M_PI)),
                            Vector3f(0.25f, 0.5f, 1.5f))));
  testShape(Pose(Id(42u, 1),
                 Transform(Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f::axisz, float(0.25 * M_PI)),
                           Vector3f(0.25f, 0.5f, 1.5f))));
}

TEST(Shapes, Sphere)
{
  testShape(Sphere(Id(42u), Spherical(Vector3f(1.2f, 2.3f, 3.4f), 1.26f)));
  testShape(Sphere(Id(42u, 1), Spherical(Vector3f(1.2f, 2.3f, 3.4f), 1.26f)));
}

TEST(Shapes, Star)
{
  testShape(Star(Id(42u), Spherical(Vector3f(1.2f, 2.3f, 3.4f), 1.26f)));
  testShape(Star(Id(42u, 1), Spherical(Vector3f(1.2f, 2.3f, 3.4f), 1.26f)));
}

TEST(Shapes, Text2D)
{
  testShape(Text2D("Transient Text2D", Id(), Spherical(Vector3f(1.2f, 2.3f, 3.4f))));
  testShape(Text2D("Persistent Text2D", 42u, Spherical(Vector3f(1.2f, 2.3f, 3.4f))));
  testShape(Text2D("Persistent, categorised Text2D", Id(42u, 1), Spherical(Vector3f(1.2f, 2.3f, 3.4f))));
}

TEST(Shapes, Text3D)
{
  // Validate all the constructors.
  testShape(Text3D("Transient Text3D", Id(), Directional(Vector3f(1.2f, 2.3f, 3.4f), 14)));
  testShape(Text3D("Transient oriented Text3D", Id(),
                   Directional(Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 2, 3).normalised(), 8)));
  testShape(Text3D("Persistent Text3D", Id(42u), Directional(Vector3f(1.2f, 2.3f, 3.4f), 23)));
  testShape(Text3D("Persistent oriented Text3D", Id(42u),
                   Directional(Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 2, 3).normalised(), 12)));
  testShape(Text3D("Persistent, categorised, oriented Text3D", Id(42u, 1),
                   Directional(Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 2, 3).normalised(), 15)));
}

TEST(Shapes, FileStream)
{
  const char *fileName = "sphere-stream.3es";
  ServerInfoMessage serverInfo;
  Sphere shape(Id(42u), Spherical(Vector3f(1.2f, 2.3f, 3.4f), 1.26f));
  testShape(shape, &serverInfo, fileName);
  validateFileStream(fileName, shape, serverInfo);
}
}  // namespace tes
