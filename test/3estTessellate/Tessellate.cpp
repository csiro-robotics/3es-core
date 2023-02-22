//
// author: Kazys Stepanas
//

#include <3escore/Connection.h>
#include <3escore/ConnectionMonitor.h>
#include <3escore/CoordinateFrame.h>
#include <3escore/Feature.h>
#include <3escore/Server.h>
#include <3escore/shapes/Shapes.h>

#include <3escore/shapes/MeshShape.h>
#include <3escore/shapes/SimpleMesh.h>
#include <3escore/Timer.h>
#include <3escore/Vector3.h>

#include <3escore/tessellate/Arrow.h>
#include <3escore/tessellate/Box.h>
#include <3escore/tessellate/Cone.h>
#include <3escore/tessellate/Cylinder.h>
#include <3escore/tessellate/Sphere.h>

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


void createAxes(unsigned &nextId, std::vector<std::shared_ptr<Shape>> &shapes,
                std::vector<std::shared_ptr<const Resource>> &resources, int argc,
                const char **argv)
{
  TES_UNUSED(resources);
  if (!haveOption("noaxes", argc, argv))
  {
    const float arrowLength = 1.0f;
    const float arrowRadius = 0.025f;
    const Vector3f pos(0.0f);
    std::shared_ptr<Arrow> arrow;

    arrow = std::make_shared<Arrow>(nextId++,
                                    Directional(pos, Vector3f(1, 0, 0), arrowRadius, arrowLength));
    arrow->setColour(Colour(Colour::Red));
    shapes.push_back(arrow);

    arrow = std::make_shared<Arrow>(nextId++,
                                    Directional(pos, Vector3f(0, 1, 0), arrowRadius, arrowLength));
    arrow->setColour(Colour(Colour::ForestGreen));
    shapes.push_back(arrow);

    arrow = std::make_shared<Arrow>(nextId++,
                                    Directional(pos, Vector3f(0, 0, 1), arrowRadius, arrowLength));
    arrow->setColour(Colour(Colour::DodgerBlue));
    shapes.push_back(arrow);
  }
}


std::shared_ptr<MeshSet> createMeshShape(unsigned shapeId, unsigned mesh_id,
                                         const std::vector<Vector3f> &vertices,
                                         const std::vector<unsigned> &indices,
                                         const std::vector<Vector3f> *normals)
{
  unsigned components = SimpleMesh::Vertex | SimpleMesh::Index;
  if (normals)
  {
    components |= SimpleMesh::Normal;
  }
  auto resource =
    std::make_shared<SimpleMesh>(mesh_id, vertices.size(), indices.size(), DtTriangles, components);
  resource->setVertices(0, vertices.data(), vertices.size());
  resource->setIndices(0, indices.data(), indices.size());
  if (normals)
  {
    resource->setNormals(0, normals->data(), normals->size());
  }

  auto meshShape = std::make_shared<MeshSet>(resource, shapeId);
  return meshShape;
}


void createShapes(unsigned &nextId, std::vector<std::shared_ptr<Shape>> &shapes,
                  std::vector<std::shared_ptr<const Resource>> &resources, int argc,
                  const char **argv)
{
  bool allShapes = haveOption("all", argc, argv) || argc == 1;
  size_t initialShapeCount = shapes.size();

  std::vector<Vector3f> vertices;
  std::vector<Vector3f> normals;
  std::vector<unsigned> indices;

  if (allShapes || haveOption("arrow", argc, argv))
  {
    tes::arrow::solid(vertices, indices, normals, 16, 0.2f, 0.1f, 0.7f, 1.0f,
                      Vector3f(1, 0.8f, -0.2f).normalised());
    auto mesh =
      createMeshShape(nextId++, unsigned(resources.size() + 1u), vertices, indices, &normals);
    shapes.push_back(mesh);
    resources.push_back(mesh->partResource(0).shared());
  }

  vertices.clear();
  normals.clear();
  indices.clear();

  if (allShapes || haveOption("box", argc, argv))
  {
    tes::box::solid(vertices, indices, normals);
    auto mesh =
      createMeshShape(nextId++, unsigned(resources.size() + 1u), vertices, indices, &normals);
    shapes.push_back(mesh);
    resources.push_back(mesh->partResource(0).shared());
  }

  vertices.clear();
  normals.clear();
  indices.clear();

  if (allShapes || haveOption("cone", argc, argv))
  {
    tes::cone::solid(vertices, indices, normals, Vector3f(0.5), Vector3f(1, 1, 0).normalised(),
                     1.5f, float(M_PI / 6.0), 12);
    auto mesh =
      createMeshShape(nextId++, unsigned(resources.size() + 1u), vertices, indices, &normals);
    shapes.push_back(mesh);
    resources.push_back(mesh->partResource(0).shared());
  }

  vertices.clear();
  normals.clear();
  indices.clear();

  if (allShapes || haveOption("cylinder", argc, argv))
  {
    tes::cylinder::solid(vertices, indices, normals, Vector3f(0, 0, 1), 2.2f, 0.3f, 18, false);
    auto mesh =
      createMeshShape(nextId++, unsigned(resources.size() + 1u), vertices, indices, &normals);
    shapes.push_back(mesh);
    resources.push_back(mesh->partResource(0).shared());
  }

  vertices.clear();
  normals.clear();
  indices.clear();

  if (allShapes || haveOption("sphere", argc, argv))
  {
    tes::sphere::solid(vertices, indices, normals, 0.7f);
    auto mesh =
      createMeshShape(nextId++, unsigned(resources.size() + 1u), vertices, indices, &normals);
    shapes.push_back(mesh);
    resources.push_back(mesh->partResource(0).shared());
  }

  vertices.clear();
  normals.clear();
  indices.clear();

  // Position the shapes so they aren't all on top of one another.
  if (shapes.size() > initialShapeCount)
  {
    Vector3f pos(0.0f);
    const float spacing = 2.0f;
    pos.x() -= spacing * float((shapes.size() - initialShapeCount) / 2u);

    for (size_t i = initialShapeCount; i < shapes.size(); ++i)
    {
      shapes[i]->setPosition(pos);
      pos.x() += spacing;
    }
  }
}


void showUsage(int argc, char **argv)
{
  TES_UNUSED(argc);
  std::cout << "Usage:\n";
  std::cout << argv[0] << " [options] [shapes]\n";
  std::cout << "\nValid options:\n";
  std::cout << "  help: show this message\n";
  if (tes::checkFeature(tes::TFeatureCompression))
  {
    std::cout << "  compress: write collated and compressed packets\n";
  }
  std::cout << "  noaxes: Don't create axis arrow objects\n";
  std::cout << "  nomove: don't move objects (keep stationary)\n";
  std::cout << "  wire: Show wireframe shapes, not slide for relevant objects\n";
  std::cout << "\nValid shapes:\n";
  std::cout << "\tall: show all shapes\n";
  std::cout << "\tarrow\n";
  std::cout << "\tbox\n";
  std::cout << "\tcone\n";
  std::cout << "\tcylinder\n";
  std::cout << "\tsphere\n";
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

  std::vector<std::shared_ptr<Shape>> shapes;
  std::vector<std::shared_ptr<const Resource>> resources;

  unsigned nextId = 1;
  createAxes(nextId, shapes, resources, argc, argv);
  createShapes(nextId, shapes, resources, argc, argv);

  const unsigned targetFrameTimeMs = 1000 / 30;
  float time = 0;
  auto lastTime = std::chrono::system_clock::now();
  auto onNewConnection = [&shapes](Server & /*server*/, Connection &connection) {
    for (auto &shape : shapes)
    {
      connection.create(*shape);
    }
    connection.updateTransfers(0);
    connection.updateFrame(0.0f);
  };

  server->connectionMonitor()->setConnectionCallback(onNewConnection);

  if (!server->connectionMonitor()->start(tes::ConnectionMode::Asynchronous))
  {
    std::cerr << "Failed to start listening." << std::endl;
    return 1;
  }
  std::cout << "Listening on port " << server->connectionMonitor()->port() << std::endl;

  // Register shapes with server.
  for (auto &shape : shapes)
  {
    server->create(*shape);
  }
  server->updateTransfers(0);
  server->updateFrame(0.0f);

  while (!quit)
  {
    auto now = std::chrono::system_clock::now();
    auto elapsed = now - lastTime;

    lastTime = now;
    float dt =
      float(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()) * 1e-6f;
    time += dt;

    if (server->connectionMonitor()->mode() == tes::ConnectionMode::Synchronous)
    {
      server->connectionMonitor()->monitorConnections();
    }
    server->connectionMonitor()->commitConnections();

    now = std::chrono::system_clock::now();
    elapsed = now - lastTime;
    unsigned elapsedMs =
      unsigned(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
    unsigned sleepTimeMs = (elapsedMs <= targetFrameTimeMs) ? targetFrameTimeMs - elapsedMs : 0u;
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
  }


  for (auto &shape : shapes)
  {
    server->destroy(*shape);
  }
  shapes.clear();
  resources.clear();

  server->close();

  server->connectionMonitor()->stop();
  server->connectionMonitor()->join();

  server.reset();

  return 0;
}
