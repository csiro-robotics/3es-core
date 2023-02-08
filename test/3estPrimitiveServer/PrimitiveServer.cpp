//
// author: Kazys Stepanas
//

// This test program creates a 3es server and publishes various shapes. It outputs a JSON
// representation of each published item to standard output. The program should be paired with a
// similar client application which logs received data in JSON format. The JSON may be parsed and
// compared to validate equivalents of what is sent and what is received.

#include <3escore/Connection.h>
#include <3escore/ConnectionMonitor.h>
#include <3escore/CoordinateFrame.h>
#include <3escore/Feature.h>
#include <3escore/Maths.h>
#include <3escore/Server.h>
#include <3escore/shapes/Shapes.h>
#include <3escore/tessellate/Sphere.h>

#define TES_ENABLE
#include <3escore/ServerMacros.h>

#include <3escore/Timer.h>
#include <3escore/Vector3.h>
#include <3escore/shapes/PointCloud.h>
#include <3escore/shapes/SimpleMesh.h>

#include <chrono>
#include <cmath>
#include <csignal>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

using namespace tes;

namespace
{
bool quit = false;

void onSignal(int arg)
{
  if (arg == SIGINT || arg == SIGTERM)
  {
    quit = true;
  }
}
}  // namespace


MeshShape *createPointsMesh(unsigned id, const std::vector<Vector3f> &vertices)
{
  MeshShape *shape = new MeshShape(DtPoints, tes::Id(id), tes::DataBuffer(vertices));
  return shape;
}


MeshShape *createLinesMesh(unsigned id, const std::vector<Vector3f> &vertices,
                           const std::vector<unsigned> &indices)
{
  std::vector<unsigned> lineIndices;

  // Many duplicate lines generated, but we are validating transfer, not rendering.
  for (size_t i = 0; i < indices.size(); i += 3)
  {
    lineIndices.push_back(indices[i + 0]);
    lineIndices.push_back(indices[i + 1]);
    lineIndices.push_back(indices[i + 1]);
    lineIndices.push_back(indices[i + 2]);
    lineIndices.push_back(indices[i + 2]);
    lineIndices.push_back(indices[i + 0]);
  }

  MeshShape *shape = new MeshShape(DtLines, Id(id), DataBuffer(vertices), DataBuffer(lineIndices));
  return shape;
}


MeshShape *createTrianglesMesh(unsigned id, const std::vector<Vector3f> &vertices,
                               const std::vector<unsigned> &indices)
{
  MeshShape *shape = new MeshShape(DtTriangles, Id(id), DataBuffer(vertices), DataBuffer(indices));
  return shape;
}


MeshShape *createVoxelsMesh(unsigned id)
{
  const float voxelScale = 0.1f;
  std::vector<Vector3f> vertices;

  Vector3f v;
  for (int z = -8; z < 8; ++z)
  {
    v.z() = float(z) * voxelScale;
    for (int y = -8; y < 8; ++y)
    {
      v.y() = float(y) * voxelScale;
      for (int x = -8; x < 8; ++x)
      {
        v.x() = float(x) * voxelScale;
        vertices.push_back(v);
      }
    }
  }

  MeshShape *shape = new MeshShape(DtVoxels, Id(id), DataBuffer(vertices));
  shape->setUniformNormal(Vector3f(voxelScale));
  return shape;
}


PointCloudShape *createCloud(unsigned id, const std::vector<Vector3f> &vertices,
                             std::vector<MeshResource *> &resources)
{
  PointCloud *mesh = new PointCloud(id * 100);
  resources.push_back(mesh);

  mesh->addPoints(vertices.data(), unsigned(vertices.size()));
  PointCloudShape *shape = new PointCloudShape(mesh, id);

  return shape;
}


MeshSet *createMeshSet(unsigned id, const std::vector<Vector3f> &vertices,
                       const std::vector<unsigned> &indices, std::vector<MeshResource *> &resources)
{
  const unsigned partCount = 5;

  MeshSet *shape = new MeshSet(id, partCount);

  for (unsigned i = 0; i < partCount; ++i)
  {
    SimpleMesh *mesh =
      new SimpleMesh(id * 100 + i, unsigned(vertices.size()), unsigned(indices.size()));
    mesh->addComponents(SimpleMesh::Normal);

    mesh->setTransform(Matrix4f::translation(Vector3f(float(i) * 2.0f, float(i) * 2.0f, 0)));

    mesh->addVertices(vertices.data(), unsigned(vertices.size()));
    mesh->addIndices(indices.data(), unsigned(indices.size()));

    for (unsigned v = 0; v < unsigned(vertices.size()); ++v)
    {
      // Assume sphere around origin.
      mesh->setNormal(v, vertices[v].normalised());
    }

    resources.push_back(mesh);
    shape->setPart(i, mesh, Matrix4f::Identity);
  }

  return shape;
}


const char *drawTypeString(DrawType type)
{
  switch (type)
  {
  case DtPoints:
    return "points";
  case DtLines:
    return "lines";
  case DtTriangles:
    return "triangles";
  case DtVoxels:
    return "voxels";
  default:
    break;
  }

  return "unknown";
}


bool haveOption(const char *opt, int argc, const char **argv)
{
  for (int i = 1; i < argc; ++i)
  {
    if (strcmp(opt, argv[i]) == 0)
    {
      return true;
    }
  }

  return false;
}


void defineCategory(const std::shared_ptr<Server> &server, const char *name, uint16_t id,
                    uint16_t parent_id, bool active)
{
  CategoryNameMessage msg;
  msg.category_id = id;
  msg.parent_id = parent_id;
  msg.default_active = (active) ? 1 : 0;
  const size_t nameLen = (name) ? strlen(name) : 0u;
  msg.name_length = (uint16_t)((nameLen <= 0xffffu) ? nameLen : 0xffffu);
  msg.name = name;
  sendMessage(*(server), MtCategory, CategoryNameMessage::MessageId, msg);
  std::cout << "  \"category-" << name << "\" : {\n"
            << "    \"category_id\" : " << id << ",\n"
            << "    \"parent_id\" : " << parent_id << ",\n"
            << "    \"default_active\" : " << (active ? "true" : "false") << ",\n"
            << "    \"name_length\" : " << msg.name_length << ",\n"
            << "    \"name\" : \"" << msg.name << "\"\n"
            << "  },\n";
}


/// Initialises a shape by setting a position and colour dependent on its @c id().
/// @param shape The shape to initialised.
/// @return @p shape
template <class T>
T *initShape(T *shape)
{
  shape->setPosition(
    Vector3f(1.0f * float(shape->id()), 0.1f * float(shape->id()), -0.75f * float(shape->id())));
  shape->setColour(ColourSet::predefined(ColourSet::Standard).cycle(shape->id()));
  return shape;
}

template <class T>
std::ostream &logShapeExtensions(std::ostream &o, const T &shape, const std::string &indent)
{
  TES_UNUSED(o);
  TES_UNUSED(shape);
  TES_UNUSED(indent);
  return o;
}


std::ostream &logShapeExtensions(std::ostream &o, const Text2D &shape, const std::string &indent)
{
  o << ",\n";
  o << indent << "\"textLength\" : " << shape.textLength() << ",\n";
  std::string text(shape.text(), shape.textLength());
  o << indent << "\"text\" : \"" << text << "\"";
  return o;
}


std::ostream &logShapeExtensions(std::ostream &o, const Text3D &shape, const std::string &indent)
{
  o << ",\n";
  o << indent << "\"textLength\" : " << shape.textLength() << ",\n";
  std::string text(shape.text(), shape.textLength());
  o << indent << "\"text\" : \"" << text << "\"";
  return o;
}


std::ostream &logShapeExtensions(std::ostream &o, const MeshShape &shape, const std::string &indent)
{
  o << ",\n";
  o << indent << "\"drawType\" : \"" << drawTypeString(shape.drawType()) << "\",\n";

  bool dangling = false;
  auto closeDangling = [&o](bool &dangling) {
    if (dangling)
    {
      o << ",\n";
      dangling = false;
    }
  };

  o << indent << "\"vertices\" : [";

  const float *verts = shape.vertices().ptr<float>(0);
  for (unsigned v = 0; v < shape.vertices().count(); ++v, verts += shape.vertices().elementStride())
  {
    if (v > 0)
    {
      o << ',';
    }
    o << '\n' << indent << "  " << verts[v + 0] << ", " << verts[v + 1] << ", " << verts[v + 2];
  }
  o << '\n' << indent << "]";
  dangling = true;

  if (shape.indices().count())
  {
    closeDangling(dangling);
    o << indent << "\"indices\" : [";
    const uint32_t *inds = shape.indices().ptr<uint32_t>(0);
    for (unsigned i = 0; i < shape.indices().count(); ++i)
    {
      if (i > 0)
      {
        o << ", ";
      }

      if (i % 16 == 0)
      {
        o << '\n' << indent << "  ";
      }

      o << inds[i];
    }
    o << '\n' << indent << "]";
    dangling = true;
  }

  if (shape.normals().count())
  {
    closeDangling(dangling);
    o << indent << "\"normals\" : [";

    const float *normals = shape.normals().ptr<float>(0);
    for (unsigned n = 0; n < shape.normals().count();
         ++n, normals += shape.normals().elementStride())
    {
      if (n > 0)
      {
        o << ',';
      }
      o << '\n'
        << indent << "  " << normals[n + 0] << ',' << normals[n + 1] << ',' << normals[n + 2];
    }
    o << '\n' << indent << "]";
    dangling = true;
  }

  return o;
}


std::ostream &operator<<(std::ostream &o, const Matrix4f &transform)
{
  o << "[\n";

  for (int i = 0; i < 4; ++i)
  {
    if (i > 0)
    {
      o << ",\n";
    }
    o << transform(i, 0) << ", " << transform(i, 1) << ", " << transform(i, 2) << ", "
      << transform(i, 3);
  }

  o << " ]";
  return o;
}


std::ostream &logMeshResource(std::ostream &o, const MeshResource &mesh, const std::string &indent,
                              bool vertexOnly = false)
{
  std::string indent2 = indent + "  ";
  o << indent << "\"mesh\" : {\n"
    << indent2 << "\"id\" : " << mesh.id() << ",\n"
    << indent2 << "\"typeId\" : " << mesh.typeId() << ",\n"
    << indent2 << "\"uniqueKey\" : " << mesh.uniqueKey() << ",\n"
    << indent2 << "\"drawType\" : " << '"' << drawTypeString((DrawType)mesh.drawType()) << '"'
    << ",\n"
    << indent2 << "\"tint\" : " << mesh.tint() << ",\n"
    << indent2 << "\"transform\" : " << mesh.transform() << ",\n";

  bool dangling = false;
  auto closeDangling = [&o](bool &dangling) {
    if (dangling)
    {
      dangling = false;
      o << ",\n";
    }
  };

  if (mesh.vertexCount())
  {
    closeDangling(dangling);
    o << indent2 << "\"vertices\" : [";
    DataBuffer verts = mesh.vertices();
    for (unsigned v = 0; v < verts.count(); ++v)
    {
      if (v > 0)
      {
        o << ',';
      }
      o << '\n'
        << indent2 << "  " << verts.get<float>(v, 0) << ", " << verts.get<float>(v, 2) << ", "
        << verts.get<float>(v, 2);
    }
    o << '\n' << indent2 << "]";
    dangling = true;
  }

  if (!vertexOnly && mesh.indexCount())
  {
    closeDangling(dangling);
    o << indent2 << "\"indices\" : [";
    DataBuffer indices = mesh.indices();

    for (unsigned i = 0; i < indices.count(); ++i)
    {
      if (i > 0)
      {
        o << ", ";
      }

      if (i % 20 == 0)
      {
        o << '\n' << indent2 << "  ";
      }

      o << indices.get<uint32_t>(i);
    }
    o << '\n' << indent2 << "]";
    dangling = true;
  }

  if (!vertexOnly && mesh.normals().isValid())
  {
    closeDangling(dangling);
    o << indent2 << "\"normals\" : [";
    DataBuffer normals = mesh.normals();
    for (unsigned n = 0; n < normals.count(); ++n)
    {
      if (n > 0)
      {
        o << ',';
      }
      o << '\n'
        << indent2 << "  " << normals.get<float>(n, 0) << ", " << normals.get<float>(n, 1) << ", "
        << normals.get<float>(n, 2);
    }
    o << '\n' << indent2 << "]";
    dangling = true;
  }

  if (!vertexOnly && mesh.uvs().isValid())
  {
    closeDangling(dangling);
    o << indent2 << "\"uvs\" : [";
    DataBuffer uvs = mesh.uvs();
    for (unsigned u = 0; u < uvs.count(); ++u)
    {
      if (u > 0)
      {
        o << ',';
      }
      o << '\n' << indent2 << "  " << uvs.get<float>(u, 0) << ", " << uvs.get<float>(u, 1);
    }
    o << '\n' << indent2 << "]";
    dangling = true;
  }

  if (mesh.colours().isValid())
  {
    closeDangling(dangling);
    o << indent << "\"colours\" : [";
    DataBuffer colours = mesh.colours();
    for (unsigned c = 0; c < colours.count(); ++c)
    {
      if (c > 0)
      {
        o << ',';
      }

      if (c % 20 == 0)
      {
        o << '\n' << indent2 << "  ";
      }
      o << colours.get<uint32_t>(c);
    }
    o << '\n' << indent2 << "]";
    dangling = true;
  }

  o << '\n' << indent << "}";

  return o;
}


std::ostream &logShapeExtensions(std::ostream &o, const PointCloudShape &shape,
                                 const std::string &indent)
{
  o << ",\n";
  o << indent << "\"pointScale\" : " << int(shape.pointScale()) << ",\n";

  logMeshResource(o, *shape.mesh(), indent, true);

  if (shape.indexCount())
  {
    o << ",\n";
    o << indent << "\"indices\" : [";
    for (unsigned i = 0; i < shape.indexCount(); ++i)
    {
      if (i % 20 == 0)
      {
        o << '\n' << indent;
      }

      if (i > 0)
      {
        o << ',';
      }

      o << shape.indices()[i];
    }
    o << indent << "\n]";
  }

  return o;
}


std::ostream &logShapeExtensions(std::ostream &o, const MeshSet &shape, const std::string &indent)
{
  o << ",\n";
  o << indent << "\"parts\" : {\n";

  std::string indent2 = indent + "  ";
  std::string indent3 = indent2 + "  ";
  for (unsigned i = 0; i < shape.partCount(); ++i)
  {
    if (i > 0)
    {
      o << ",\n";
    }
    o << indent2 << "\"part-" << i << "\" : {\n";
    o << indent3 << "\"transform\" : " << shape.partTransform(i) << ",\n";
    logMeshResource(o, *shape.partResource(i), indent3) << '\n';
    o << indent2 << "}";
  }

  o << '\n' << indent << "}";

  return o;
}


template <typename T>
std::ostream &logShape(std::ostream &o, const T &shape, const char *suffix)
{
  auto precision = o.precision(20);

  o << "  \"" << shape.type() << suffix << "\" : {\n"
    << "    \"routingId\" : " << shape.routingId() << ",\n"
    << "    \"id\" : " << shape.data().id << ",\n"
    << "    \"category\" : " << shape.data().category << ",\n"
    << "    \"flags\" : " << shape.data().flags << ",\n"
    << "    \"reserved\" : " << shape.data().reserved << ",\n"
    << "    \"attributes\" : {\n"
    << "      \"colour\" : " << shape.attributes().colour << ",\n"
    << "      \"position\" : [\n"
    << "        " << shape.attributes().position[0] << ", " << shape.attributes().position[1]
    << ", " << shape.attributes().position[2] << "\n"
    << "      ],\n"
    << "      \"rotation\" : [\n"
    << "        " << shape.attributes().rotation[0] << ", " << shape.attributes().rotation[1]
    << ", " << shape.attributes().rotation[2] << ", " << shape.attributes().rotation[3] << "\n"
    << "      ],\n"
    << "      \"scale\" : [\n"
    << "        " << shape.attributes().scale[0] << ", " << shape.attributes().scale[1] << ", "
    << shape.attributes().scale[2] << "\n"
    << "      ]\n"
    << "    }";

  std::string indent = "    ";
  logShapeExtensions(o, shape, indent) << ",\n";

  o << "    \"isComplex\" : " << (shape.isComplex() ? "true" : "false") << "\n";

  o << "  }";

  o.precision(precision);

  return o;
}


const char *coordinateFrameString(uint8_t frame)
{
  const char *frames[] = { "xyz",  "xz-y", "yx-z", "yzx",  "zxy",  "zy-x",
                           "xy-z", "xzy",  "yxz",  "yz-x", "zx-y", "zyx" };

  if (frame < sizeof(frames) / sizeof(frames[0]))
  {
    return frames[frame];
  }

  return "unknown";
}


std::ostream &operator<<(std::ostream &o, const ServerInfoMessage &info)
{
  o << "  \"server\" : {\n"
    << "    \"time_unit\" : " << info.time_unit << ",\n"
    << "    \"default_frame_time\" : " << info.default_frame_time << ",\n"
    << "    \"coordinate_frame\" : \"" << coordinateFrameString(info.coordinate_frame) << "\"\n"
    << "  },";
  return o;
}


/// Add @p shape to the @p server and @p shapes, printing it's attributes in JSON to @c stdout.
template <class T>
void addShape(T *shape, const std::shared_ptr<Server> &server, std::vector<Shape *> &shapes,
              const char *suffix = "")
{
  server->create(*shape);
  logShape(std::cout, *shape, suffix) << ",\n";
  shapes.push_back(shape);
}


void showUsage(int argc, char **argv)
{
  TES_UNUSED(argc);
  std::cout << "Usage:\n";
  std::cout << argv[0] << " [options] [shapes]\n";
  std::cout << "\nValid options:\n";
  std::cout << "  help: show this message\n";
  if (checkFeature(TFeatureCompression))
  {
    std::cout << "  compress: write collated and compressed packets\n";
  }
  std::cout.flush();
}


int main(int argc, char **argvNonConst)
{
  const char **argv = const_cast<const char **>(argvNonConst);
  signal(SIGINT, &onSignal);
  signal(SIGTERM, &onSignal);

  if (haveOption("help", argc, argv))
  {
    showUsage(argc, argvNonConst);
    return 0;
  }

  ServerInfoMessage info;
  initDefaultServerInfo(&info);
  info.coordinate_frame = XYZ;
  unsigned serverFlags = SFDefaultNoCompression;
  if (haveOption("compress", argc, argv))
  {
    serverFlags |= SFCompress;
  }
  auto server = Server::create(ServerSettings(serverFlags), &info);

  std::cout << "{\n";
  std::cout << info << std::endl;

  if (server->connectionMonitor()->waitForConnection(20000U) > 0)
  {
    server->connectionMonitor()->commitConnections();
  }

  server->updateTransfers(0);
  server->updateFrame(0.0f, true);
  auto connection_monitor = server->connectionMonitor();
  if (connection_monitor->mode() == ConnectionMonitor::Synchronous)
  {
    connection_monitor->monitorConnections();
  }
  connection_monitor->commitConnections();

  defineCategory(server, "Root", 0, 0, true);
  defineCategory(server, "Branch1", 1, 0, true);
  defineCategory(server, "Branch2", 2, 0, true);
  defineCategory(server, "Branch3", 3, 0, true);
  defineCategory(server, "Branch4-hidden", 4, 0, false);

  defineCategory(server, "Child1", 101, 1, true);
  defineCategory(server, "Child2", 102, 1, true);
  defineCategory(server, "Child3", 103, 1, true);
  defineCategory(server, "Child4", 104, 1, true);

  unsigned nextId = 1u;
  std::vector<Shape *> shapes;
  std::vector<MeshResource *> resources;

  addShape(
    initShape(new Arrow(nextId++, Directional(Vector3f(0.0f), Vector3f(1, 0, 0), 0.25f, 1.0f))),
    server, shapes);
  addShape(
    initShape(new Box(nextId++, Transform(Vector3f(0.0f),
                                          rotationToQuaternion(Matrix3f::rotation(
                                            degToRad(15.0f), degToRad(25.0f), degToRad(-9.0f))),
                                          Vector3f(0.1f, 0.2f, 0.23f)))),
    server, shapes);
  addShape(initShape(new Capsule(
             nextId++, Directional(Vector3f(0.0f), Vector3f(1, 2, 0).normalised(), 0.3f, 2.0f))),
           server, shapes);
  addShape(initShape(new Cone(
             nextId++, Directional(Vector3f(0.0f), Vector3f(0, 2, 1).normalised(), 0.4f, 2.25f))),
           server, shapes);
  addShape(
    initShape(new Cylinder(
      nextId++, Directional(Vector3f(0.0f), Vector3f(2, -1.4f, 1).normalised(), 0.15f, 1.2f))),
    server, shapes);
  addShape(
    initShape(new Plane(nextId++, Directional(Vector3f(0.0f), Vector3f(-1, -1, 1).normalised()))),
    server, shapes);
  addShape(initShape(new Sphere(nextId++, Spherical(Vector3f(0.0f), 1.15f))), server, shapes);
  addShape(initShape(new Star(nextId++, Spherical(Vector3f(0.0f), 0.15f))), server, shapes);
  addShape(initShape(new Text2D("Hello Text2D", nextId++)), server, shapes);
  addShape(initShape(new Text3D("Hello Text3D", nextId++)), server, shapes);

  std::vector<Vector3f> sphereVerts;
  std::vector<unsigned> sphereIndices;

  // Use a large sphere to ensure we need multiple data packets to transfer the vertices.
  sphere::solid(sphereVerts, sphereIndices, 2.1f, 4);

  addShape(createPointsMesh(nextId++, sphereVerts), server, shapes, "-points");
  addShape(createLinesMesh(nextId++, sphereVerts, sphereIndices), server, shapes, "-lines");
  addShape(createTrianglesMesh(nextId++, sphereVerts, sphereIndices), server, shapes, "-triangles");
  addShape(createVoxelsMesh(nextId++), server, shapes, "-voxels");
  addShape(createCloud(nextId++, sphereVerts, resources), server, shapes);
  addShape(createMeshSet(nextId++, sphereVerts, sphereIndices, resources), server, shapes);

  server->updateTransfers(0);
  server->updateFrame(0.0f, true);

  server->close();

  server->connectionMonitor()->stop();
  server->connectionMonitor()->join();

  server.reset();

  for (Shape *shape : shapes)
  {
    delete shape;
  }
  shapes.clear();

  for (Resource *resource : resources)
  {
    delete resource;
  }

  // Next line is partly to keep well formed JSON.
  std::cout << "  \"success\" : true\n";
  std::cout << "}\n";

  return 0;
}
