//
// author: Kazys Stepanas
//

#include "3est-common.h"

#include <3escollatedpacketdecoder.h>
#include <3escoordinateframe.h>
#include <3esconnectionmonitor.h>
#include <3esmaths.h>
#include <3esmathsstream.h>
#include <3esmessages.h>
#include <3espacketbuffer.h>
#include <3espacketreader.h>
#include <3espacketwriter.h>
#include <3estcpsocket.h>
#include <3esserver.h>
#include <3esserverutil.h>
#include <shapes/3espointcloud.h>
#include <shapes/3esshapes.h>
#include <shapes/3essimplemesh.h>

#include <gtest/gtest.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <unordered_map>

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


  template <class T>
  void validateClient(TcpSocket &socket, const T &referenceShape, const ServerInfoMessage &serverInfo,
                      unsigned timeoutSec = 10)
  {
    typedef std::chrono::steady_clock Clock;
    ServerInfoMessage readServerInfo;
    std::vector<uint8_t> readBuffer(0xffffu);
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
    while (!endMsgReceived && std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - startTime).count() < timeoutSec)
    {
      readCount = socket.readAvailable(readBuffer.data(), int(readBuffer.size()));
      // Assert no read errors.
      ASSERT_TRUE(readCount >= 0);
      if (readCount < 0)
      {
        break;
      }

      if  (readCount == 0)
      {
        // Nothing read. Wait.
        std::this_thread::yield();
        continue;
      }

      packetBuffer.addBytes(readBuffer.data(), readCount);

      while (const PacketHeader *primaryPacket = packetBuffer.extractPacket())
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

            for (int i = 0; i < sizeof(readServerInfo.reserved) / sizeof(readServerInfo.reserved[0]); ++i)
            {
              EXPECT_EQ(readServerInfo.reserved[i], serverInfo.reserved[i]);
            }
            break;

          case MtControl:
          {
            // Only interested in the CIdEnd message to mark the end of the stream.
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

        packetBuffer.releasePacket(primaryPacket);
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
  void testShape(const T &shape)
  {
    // Initialise server.
    ServerInfoMessage info;
    initDefaultServerInfo(&info);
    info.coordinateFrame = XYZ;

    unsigned serverFlags = SF_Default | SF_CollateAndCompress;
    ServerSettings serverSettings(serverFlags);
    serverSettings.portRange = 1000;
    Server *server = Server::create(serverSettings, &info);

    // std::cout << "Start on port " << serverSettings.listenPort << std::endl;
    ASSERT_TRUE(server->connectionMonitor()->start(tes::ConnectionMonitor::Asynchronous));
    // std::cout << "Server listening on port " << server->connectionMonitor()->port() << std::endl;;

    // Create client and connect.
    TcpSocket client;
    client.open("127.0.0.1", server->connectionMonitor()->port());

    // Wait for connection.
    if (server->connectionMonitor()->waitForConnection(5000U) > 0)
    {
      server->connectionMonitor()->commitConnections();
    }

    EXPECT_GT(server->connectionCount(), 0u);
    EXPECT_TRUE(client.isConnected());

    // Send server messages from another thread. Otherwise large packets may block.
    std::thread sendThread([server, &shape] ()
    {
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

  TEST(Shapes, Arrow)
  {
    testShape(Arrow(42, Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), 2.0f, 0.05f));
    testShape(Arrow(42, 1, Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), 2.0f, 0.05f));
  }

  TEST(Shapes, Box)
  {
    testShape(Box(42, Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 3, 2), Quaternionf().setAxisAngle(Vector3f(1, 1, 1).normalised(), degToRad(18.0f))));
    testShape(Box(42, 1, Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 3, 2), Quaternionf().setAxisAngle(Vector3f(1, 1, 1).normalised(), degToRad(18.0f))));
  }

  TEST(Shapes, Capsule)
  {
    testShape(Capsule(42, Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), 0.3f, 2.05f));
    testShape(Capsule(42, 1, Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), 0.3f, 2.05f));
  }

  TEST(Shapes, Cone)
  {
    testShape(Cone(42, Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), degToRad(35.0f), 3.0f));
    testShape(Cone(42, 1, Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), degToRad(35.0f), 3.0f));
  }

  TEST(Shapes, Cylinder)
  {
    testShape(Cylinder(42, Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), 0.25f, 1.05f));
    testShape(Cylinder(42, 1, Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), 0.25f, 1.05f));
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
    testShape(MeshSet(meshes[0], 42));

    // Now a multi-part MeshSet.
    {
      MeshSet set(42, 1, int(meshes.size()));

      Matrix4f transform = Matrix4f::identity;
      for (int i = 0; i < int(meshes.size()); ++i)
      {
        transform = prsTransform(Vector3f(i * 1.0f, i - 3.2f, 1.5f * i),
                                 Quaternionf().setAxisAngle(Vector3f(i * 1.0f, i + 1.0f, i - 3.0f).normalised(), degToRad((i + 1) * 6.0f)),
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
    testShape(MeshShape(DtPoints, vertices.data()->v, unsigned(vertices.size()), sizeof(*vertices.data()),
              Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f(1, 1, 1), degToRad(18.0f)),
              Vector3f(1.0f, 1.2f, 0.8f)));
    // 2. drawType, verts, vcount, vstrideBytes, indices, icount, pos, rot, scale
    testShape(MeshShape(DtTriangles, vertices.data()->v, unsigned(vertices.size()), sizeof(*vertices.data()),
              indices.data(), unsigned(indices.size()),
              Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f(1, 1, 1), degToRad(18.0f)),
              Vector3f(1.0f, 1.2f, 0.8f)));
    // 3. drawType, verts, vcount, vstrideBytes, id, pos, rot, scale
    testShape(MeshShape(DtPoints, vertices.data()->v, unsigned(vertices.size()), sizeof(*vertices.data()),
              42,
              Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f(1, 1, 1), degToRad(18.0f)),
              Vector3f(1.0f, 1.2f, 0.8f)));
    // 4. drawType, verts, vcount, vstrideBytes, indices, icount, id, pos, rot, scale
    testShape(MeshShape(DtTriangles, vertices.data()->v, unsigned(vertices.size()), sizeof(*vertices.data()),
              indices.data(), unsigned(indices.size()),
              42,
              Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f(1, 1, 1), degToRad(18.0f)),
              Vector3f(1.0f, 1.2f, 0.8f)));
    // 5. drawType, verts, vcount, vstrideBytes, indices, icount, id, cat, pos, rot, scale
    testShape(MeshShape(DtTriangles, vertices.data()->v, unsigned(vertices.size()), sizeof(*vertices.data()),
              indices.data(), unsigned(indices.size()),
              42, 1,
              Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f(1, 1, 1), degToRad(18.0f)),
              Vector3f(1.0f, 1.2f, 0.8f)));

    // II> Test with uniform normal.
    testShape(MeshShape(DtVoxels, vertices.data()->v, unsigned(vertices.size()), sizeof(*vertices.data()),
              42,
              Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f(1, 1, 1), degToRad(18.0f)),
              Vector3f(1.0f, 1.2f, 0.8f)).setUniformNormal(Vector3f(0.1f, 0.1f, 0.1f)));

    // III> Test will many normals.
    testShape(MeshShape(DtTriangles, vertices.data()->v, unsigned(vertices.size()), sizeof(*vertices.data()),
              indices.data(), unsigned(indices.size()),
              42, 1,
              Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f(1, 1, 1), degToRad(18.0f)),
              Vector3f(1.0f, 1.2f, 0.8f)).setNormals(normals.data()->v, sizeof(*normals.data())));

    // IV> Test with colours.
    testShape(MeshShape(DtTriangles, vertices.data()->v, unsigned(vertices.size()), sizeof(*vertices.data()),
              indices.data(), unsigned(indices.size()),
              Vector3f(1.2f, 2.3f, 3.4f), Quaternionf().setAxisAngle(Vector3f(1, 1, 1), degToRad(18.0f)),
              Vector3f(1.0f, 1.2f, 0.8f)).setColours(colours.data()));
  }

  TEST(Shapes, Plane)
  {
    testShape(Plane(42, Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), 5.0f, 0.75f));
    testShape(Plane(42, 1, Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 1, 1).normalised(), 5.0f, 0.75f));
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
    testShape(PointCloudShape(&cloud, 42, 0, 8));

    // Indexed (sub-sampled) cloud. Just use half the points.
    indices.resize(0);
    for (unsigned i = 0; i < unsigned(vertices.size()/2); ++i)
    {
      indices.push_back(i);
    }
    testShape(PointCloudShape(&cloud, 42, 0, 8).setIndices(indices.data(), unsigned(indices.size())));
  }

  TEST(Shapes, Sphere)
  {
    testShape(Sphere(42, Vector3f(1.2f, 2.3f, 3.4f), 1.26f));
    testShape(Sphere(42, 1, Vector3f(1.2f, 2.3f, 3.4f), 1.26f));
  }

  TEST(Shapes, Star)
  {
    testShape(Star(42, Vector3f(1.2f, 2.3f, 3.4f), 1.26f));
    testShape(Star(42, 1, Vector3f(1.2f, 2.3f, 3.4f), 1.26f));
  }

  TEST(Shapes, Text2D)
  {
    testShape(Text2D("Transient Text2D", Vector3f(1.2f, 2.3f, 3.4f)));
    testShape(Text2D("Persistent Text2D", 42, Vector3f(1.2f, 2.3f, 3.4f)));
    testShape(Text2D("Persistent, categorised Text2D", 42, 1, Vector3f(1.2f, 2.3f, 3.4f)));
  }

  TEST(Shapes, Text3D)
  {
    // Validate all the constructors.
    testShape(Text3D("Transient Text3D", Vector3f(1.2f, 2.3f, 3.4f), 14));
    testShape(Text3D("Transient oriented Text3D", Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 2, 3).normalised(), 8));
    testShape(Text3D("Persistent Text3D", 42, Vector3f(1.2f, 2.3f, 3.4f), 23));
    testShape(Text3D("Persistent oriented Text3D", 42, Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 2, 3).normalised(), 12));
    testShape(Text3D("Persistent, categorised, oriented Text3D", 42, 1, Vector3f(1.2f, 2.3f, 3.4f), Vector3f(1, 2, 3).normalised(), 15));
  }
}
