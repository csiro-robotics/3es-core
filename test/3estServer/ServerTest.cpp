//
// author: Kazys Stepanas
//

#include <3escore/Connection.h>
#include <3escore/ConnectionMonitor.h>
#include <3escore/CoordinateFrame.h>
#include <3escore/Feature.h>
#include <3escore/Server.h>
#include <3escore/shapes/Shapes.h>

#define TES_ENABLE
#include <3escore/ServerApi.h>

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


enum Categories
{
  CatRoot,
  Cat3D,
  CatText,
  CatSimple3D,
  CatComplex3D,
  CatArrow,
  CatBox,
  CatCapsule,
  CatCylinder,
  CatCone,
  CatLines,
  CatMesh,
  CatPlane,
  CatPoints,
  CatPose,
  CatSphere,
  CatStar,
  CatText2D,
  CatText3D,
  CatTriangles
};


class ShapeMover
{
public:
  ShapeMover(std::shared_ptr<Shape> shape)
    : _shape(std::move(shape))
  {}

  virtual inline ~ShapeMover() {}

  inline std::shared_ptr<Shape> shape() { return _shape; }
  inline std::shared_ptr<const Shape> shape() const { return _shape; }
  inline void setShape(std::shared_ptr<Shape> shape)
  {
    _shape = std::move(shape);
    onShapeChange();
  }

  virtual void reset() {}

  virtual void update(float time, float dt)
  {
    TES_UNUSED(time);
    TES_UNUSED(dt);
  }

protected:
  virtual void onShapeChange() {}

private:
  std::shared_ptr<Shape> _shape;
};


class Oscilator : public ShapeMover
{
public:
  inline Oscilator(std::shared_ptr<Shape> shape, float amplitude = 1.0f, float period = 5.0f,
                   const Vector3f &axis = Vector3f(0, 0, 1))
    : ShapeMover(std::move(shape))
    , _referencePos(Vector3f::Zero)
    , _axis(axis)
    , _amplitude(amplitude)
    , _period(period)
  {
    if (!this->shape())
    {
      _referencePos = Vector3f(this->shape()->position());
    }
    onShapeChange();
  }

  inline const Vector3f &axis() const { return _axis; }
  inline const Vector3f &referencePos() const { return _referencePos; }
  inline float amplitude() const { return _amplitude; }
  inline float period() const { return _period; }

  void reset() override { _referencePos = shape() ? Vector3f(shape()->position()) : _referencePos; }

  void update(float time, float dt) override
  {
    TES_UNUSED(dt);
    Vector3f pos = _referencePos + _amplitude * std::sin(time) * _axis;
    shape()->setPosition(pos);
  }

protected:
  void onShapeChange() override
  {
    _referencePos = (shape()) ? Vector3f(shape()->position()) : _referencePos;
  }

private:
  Vector3f _referencePos;
  Vector3f _axis;
  float _amplitude;
  float _period;
};


std::shared_ptr<MeshResource> createTestMesh()
{
  auto mesh = std::make_shared<SimpleMesh>(
    1, 4, 6, DtTriangles, SimpleMesh::Vertex | SimpleMesh::Index | SimpleMesh::Colour);
  mesh->setVertex(0, Vector3f(-0.5f, 0, -0.5f));
  mesh->setVertex(1, Vector3f(0.5f, 0, -0.5f));
  mesh->setVertex(2, Vector3f(0.5f, 0, 0.5f));
  mesh->setVertex(3, Vector3f(-0.5f, 0, 0.5f));

  mesh->setIndex(0, 0);
  mesh->setIndex(1, 1);
  mesh->setIndex(2, 2);
  mesh->setIndex(3, 0);
  mesh->setIndex(4, 2);
  mesh->setIndex(5, 3);

  mesh->setColour(0, 0xff0000ff);
  mesh->setColour(1, 0xffff00ff);
  mesh->setColour(2, 0xff00ffff);
  mesh->setColour(3, 0xffffffff);

  // mesh->setNormal(0, Vector3f(0, 1, 0));
  // mesh->setNormal(1, Vector3f(0, 1, 0));
  // mesh->setNormal(2, Vector3f(0, 1, 0));
  // mesh->setNormal(3, Vector3f(0, 1, 0));

  return mesh;
}


std::shared_ptr<MeshResource> createTestCloud()
{
  auto cloud = std::make_shared<PointCloud>(2);  // Considered a Mesh for ID purposes.
  cloud->resize(8);

  cloud->setPoint(0, Vector3f(0, 0, 0), Vector3f(0, 0, 1), Colour(0, 255, 255));
  cloud->setPoint(1, Vector3f(1, 0, 0), Vector3f(0, 0, 1), Colour(0, 255, 255));
  cloud->setPoint(2, Vector3f(0, 1, 0), Vector3f(0, 0, 1), Colour(255, 255, 255));
  cloud->setPoint(3, Vector3f(0, 0, 1), Vector3f(0, 0, 1), Colour(0, 255, 255));
  cloud->setPoint(4, Vector3f(1, 1, 0), Vector3f(0, 0, 1), Colour(0, 0, 0));
  cloud->setPoint(5, Vector3f(0, 1, 1), Vector3f(0, 0, 1), Colour(0, 255, 255));
  cloud->setPoint(6, Vector3f(1, 0, 1), Vector3f(0, 0, 1), Colour(0, 255, 255));
  cloud->setPoint(7, Vector3f(1, 1, 1), Vector3f(0, 0, 1), Colour(0, 255, 255));

  return cloud;
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


void createAxes(unsigned &nextId, std::vector<std::shared_ptr<Shape>> &shapes,
                std::vector<std::shared_ptr<ShapeMover>> &movers,
                std::vector<std::shared_ptr<Resource>> &resources, int argc, const char **argv)
{
  TES_UNUSED(movers);
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
    shapes.emplace_back(arrow);

    arrow = std::make_shared<Arrow>(nextId++,
                                    Directional(pos, Vector3f(0, 1, 0), arrowRadius, arrowLength));
    arrow->setColour(Colour(Colour::ForestGreen));
    shapes.emplace_back(arrow);

    arrow = std::make_shared<Arrow>(nextId++,
                                    Directional(pos, Vector3f(0, 0, 1), arrowRadius, arrowLength));
    arrow->setColour(Colour(Colour::DodgerBlue));
    shapes.emplace_back(arrow);
  }
}


void createShapes(unsigned &nextId, std::vector<std::shared_ptr<Shape>> &shapes,
                  std::vector<std::shared_ptr<ShapeMover>> &movers,
                  std::vector<std::shared_ptr<Resource>> &resources, int argc, const char **argv)
{
  bool allShapes = haveOption("all", argc, argv);
  bool noMove = haveOption("nomove", argc, argv);
  size_t initialShapeCount = shapes.size();

  if (allShapes || haveOption("arrow", argc, argv))
  {
    auto arrow = std::make_shared<Arrow>(Id(nextId++, CatArrow));
    arrow->setRadius(0.5f);
    arrow->setLength(1.0f);
    arrow->setColour(Colour(Colour::SeaGreen));
    shapes.emplace_back(arrow);
    if (!noMove)
    {
      movers.emplace_back(std::make_shared<Oscilator>(arrow, 2.0f, 2.5f));
    }
  }

  if (allShapes || haveOption("box", argc, argv))
  {
    auto box = std::make_shared<Box>(Id(nextId++, CatBox));
    box->setScale(Vector3f(0.45f));
    box->setColour(Colour(Colour::MediumSlateBlue));
    shapes.emplace_back(box);
    if (!noMove)
    {
      movers.emplace_back(std::make_shared<Oscilator>(box, 2.0f, 2.5f));
    }
  }

  if (allShapes || haveOption("capsule", argc, argv))
  {
    auto capsule = std::make_shared<Capsule>(Id(nextId++, CatCapsule), Directional::identity());
    capsule->setLength(2.0f);
    capsule->setRadius(0.3f);
    capsule->setColour(Colour(Colour::LavenderBlush));
    shapes.emplace_back(capsule);
    if (!noMove)
    {
      movers.emplace_back(std::make_shared<Oscilator>(capsule, 2.0f, 2.5f));
    }
  }

  if (allShapes || haveOption("cone", argc, argv))
  {
    auto cone = std::make_shared<Cone>(Id(nextId++, CatCone));
    cone->setLength(2.0f);
    cone->setRadius(0.25f);
    cone->setColour(Colour(Colour::SandyBrown));
    shapes.emplace_back(cone);
    if (!noMove)
    {
      movers.emplace_back(std::make_shared<Oscilator>(cone, 2.0f, 2.5f));
    }
  }

  if (allShapes || haveOption("cylinder", argc, argv))
  {
    auto cylinder = std::make_shared<Cylinder>(Id(nextId++, CatCylinder));
    cylinder->setScale(Vector3f(0.45f));
    cylinder->setColour(Colour(Colour::FireBrick));
    shapes.emplace_back(cylinder);
    if (!noMove)
    {
      movers.emplace_back(std::make_shared<Oscilator>(cylinder, 2.0f, 2.5f));
    }
  }

  if (allShapes || haveOption("plane", argc, argv))
  {
    auto plane = std::make_shared<Plane>(Id(nextId++, CatPlane));
    plane->setNormal(Vector3f(1.0f, 1.0f, 0.0f).normalised());
    plane->setScale(1.5f);
    plane->setNormalLength(0.5f);
    plane->setColour(Colour(Colour::LightSlateGrey));
    shapes.emplace_back(plane);
    if (!noMove)
    {
      movers.emplace_back(std::make_shared<Oscilator>(plane, 2.0f, 2.5f));
    }
  }

  if (allShapes || haveOption("pose", argc, argv))
  {
    auto pose = std::make_shared<Pose>(Id(nextId++, CatPose));
    pose->setRotation(Quaternionf().setAxisAngle(Vector3f::AxisZ, float(0.25 * M_PI)));
    shapes.emplace_back(pose);
    if (!noMove)
    {
      movers.emplace_back(std::make_shared<Oscilator>(pose, 2.0f, 2.5f));
    }
  }

  if (allShapes || haveOption("sphere", argc, argv))
  {
    auto sphere = std::make_shared<Sphere>(Id(nextId++, CatSphere));
    sphere->setRadius(0.75f);
    sphere->setColour(Colour(Colour::Coral));
    shapes.emplace_back(sphere);
    if (!noMove)
    {
      movers.emplace_back(std::make_shared<Oscilator>(sphere, 2.0f, 2.5f));
    }
  }

  if (allShapes || haveOption("star", argc, argv))
  {
    auto star = std::make_shared<Star>(Id(nextId++, CatStar));
    star->setRadius(0.75f);
    star->setColour(Colour(Colour::DarkGreen));
    shapes.emplace_back(star);
    if (!noMove)
    {
      movers.emplace_back(std::make_shared<Oscilator>(star, 2.0f, 2.5f));
    }
  }

  if (allShapes || haveOption("lines", argc, argv))
  {
    static const Vector3f lineSet[] = { Vector3f(0, 0, 0), Vector3f(0, 0, 1),
                                        Vector3f(0, 0, 1), Vector3f(0.25f, 0, 0.8f),
                                        Vector3f(0, 0, 1), Vector3f(-0.25f, 0, 0.8f) };
    const unsigned lineVertexCount = sizeof(lineSet) / sizeof(lineSet[0]);
    auto lines = std::make_shared<MeshShape>(DtLines, Id(nextId++, CatLines),
                                             DataBuffer(lineSet, lineVertexCount));
    shapes.emplace_back(lines);
    // if (!noMove)
    // {
    //   movers.emplace_back(std::make_shared<Oscilator>(mesh, 2.0f, 2.5f));
    // }
  }

  if (allShapes || haveOption("triangles", argc, argv))
  {
    static const Vector3f triangleSet[] = {
      Vector3f(0, 0, 0),      Vector3f(0, 0.25f, 1), Vector3f(0.25f, 0, 1), Vector3f(0, 0, 0),
      Vector3f(-0.25f, 0, 1), Vector3f(0, 0.25f, 1), Vector3f(0, 0, 0),     Vector3f(0, -0.25f, 1),
      Vector3f(-0.25f, 0, 1), Vector3f(0, 0, 0),     Vector3f(0.25f, 0, 1), Vector3f(0, -0.25f, 1)
    };
    const unsigned triVertexCount = sizeof(triangleSet) / sizeof(triangleSet[0]);
    static const uint32_t colours[] = {
      Colour(Colour::Red).colour32(),   Colour(Colour::Red).colour32(),
      Colour(Colour::Red).colour32(),   Colour(Colour::Green).colour32(),
      Colour(Colour::Green).colour32(), Colour(Colour::Green).colour32(),
      Colour(Colour::Blue).colour32(),  Colour(Colour::Blue).colour32(),
      Colour(Colour::Blue).colour32(),  Colour(Colour::White).colour32(),
      Colour(Colour::White).colour32(), Colour(Colour::White).colour32()
    };
    auto triangles = std::make_shared<MeshShape>(DtTriangles, Id(nextId++, CatTriangles),
                                                 DataBuffer(triangleSet, triVertexCount));
    triangles->setColours(colours);
    triangles->duplicateArrays();
    shapes.emplace_back(triangles);
    // if (!noMove)
    // {
    //   movers.emplace_back(std::make_shared<Oscilator>(mesh, 2.0f, 2.5f));
    // }
  }

  if (allShapes || haveOption("mesh", argc, argv))
  {
    auto meshRes = createTestMesh();
    resources.emplace_back(meshRes);
    const unsigned partCount = 2;
    auto mesh = std::make_shared<MeshSet>(Id(nextId++, CatMesh), partCount);
    mesh->setPart(0, meshRes, Matrix4f::Identity, Colour(Colour::YellowGreen));
    mesh->setPart(1, meshRes, Matrix4f::translation(Vector3f(0, 0, 1.5f)), Colour(Colour::SkyBlue));
    shapes.emplace_back(mesh);
    // if (!noMove)
    // {
    //   movers.emplace_back(std::make_shared<Oscilator>(mesh, 2.0f, 2.5f));
    // }
    std::cout << "make mesh" << std::endl;
  }

  if (allShapes || haveOption("points", argc, argv))
  {
    static const Vector3f pts[] = { Vector3f(0, 0.25f, 1), Vector3f(0.25f, 0, 1),
                                    Vector3f(-0.25f, 0, 1), Vector3f(0, -0.25f, 1),
                                    Vector3f(0, -0.25f, 1) };
    const unsigned pointsCount = sizeof(pts) / sizeof(pts[0]);
    static const uint32_t colours[] = { Colour(Colour::Black).colour32(),
                                        Colour(Colour::Red).colour32(),
                                        Colour(Colour::Green).colour32(),
                                        Colour(Colour::Blue).colour32(),
                                        Colour(Colour::White).colour32() };
    auto points =
      std::make_shared<MeshShape>(DtPoints, Id(nextId++, CatPoints), DataBuffer(pts, pointsCount));
    points->setColours(colours);
    points->setDrawScale(3.0f);
    shapes.emplace_back(points);
    // if (!noMove)
    // {
    //   movers.emplace_back(std::make_shared<Oscilator>(mesh, 2.0f, 2.5f));
    // }
  }

  if (allShapes || haveOption("voxels", argc, argv))
  {
    static const Vector3f pts[] = { Vector3f(0, 0.25f, 1), Vector3f(0.25f, 0, 1),
                                    Vector3f(-0.25f, 0, 1), Vector3f(0, -0.25f, 1),
                                    Vector3f(0, -0.25f, 1) };
    const unsigned pointsCount = sizeof(pts) / sizeof(pts[0]);
    static const uint32_t colours[] = { Colour(Colour::Black).colour32(),
                                        Colour(Colour::Red).colour32(),
                                        Colour(Colour::Green).colour32(),
                                        Colour(Colour::Blue).colour32(),
                                        Colour(Colour::White).colour32() };
    auto points =
      std::make_shared<MeshShape>(DtVoxels, Id(nextId++, CatPoints), DataBuffer(pts, pointsCount));
    points->setColours(colours);
    points->setDrawScale(0.2f);
    // points->setUniformNormal(tes::Vector3f(0.2f));
    shapes.emplace_back(points);
    // if (!noMove)
    // {
    //   movers.emplace_back(std::make_shared<Oscilator>(mesh, 2.0f, 2.5f));
    // }
  }

  if (allShapes || haveOption("cloud", argc, argv) || haveOption("cloudpart", argc, argv))
  {
    auto cloud = createTestCloud();
    auto points = std::make_shared<PointCloudShape>(cloud, Id(nextId++, CatPoints), 1.25f);
    if (haveOption("cloudpart", argc, argv))
    {
      // Partial indexing.
      std::vector<unsigned> partialIndices;
      partialIndices.resize((cloud->vertexCount() + 1) / 2);
      unsigned nextIndex = 0;
      for (size_t i = 0; i < partialIndices.size(); ++i)
      {
        partialIndices[i] = nextIndex;
        nextIndex += 2;
      }
      points->setIndices(partialIndices.begin(), partialIndices.end());
    }
    shapes.emplace_back(points);
    resources.emplace_back(cloud);
    // if (!noMove)
    // {
    //   movers.emplace_back(std::make_shared<Oscilator>(points, 2.0f, 2.5f));
    // }
  }

  if (haveOption("multi", argc, argv))
  {
    // Set the block size to ensure we are larger than the multi-shape packet size.
    const int blockSize = 15;
    const int manyCount = blockSize * blockSize * blockSize;
    const float separation = 0.3f;
    const float blockOffset = -0.5f * blockSize * separation;

    std::vector<std::shared_ptr<Shape>> manyShapes(manyCount);
    unsigned id = nextId++;
    unsigned i = 0;
    for (int z = 0; z < blockSize; ++z)
    {
      for (int y = 0; y < blockSize; ++y)
      {
        for (int x = 0; x < blockSize; ++x)
        {
          Vector3f pos;
          pos.x() = blockOffset + float(x) * separation;
          pos.y() = blockOffset + float(y) * separation;
          pos.z() = blockOffset + float(z) * separation;

          auto capsule =
            std::make_shared<Capsule>(Id(id, CatCapsule), Directional::identity(false));
          capsule->setLength(0.4f);
          capsule->setRadius(0.15f);
          capsule->setColour(ColourSet::predefined(ColourSet::Standard).cycle(i));
          capsule->setPosition(pos);
          manyShapes[i++] = capsule;
        }
      }
    }

    auto shape =
      std::make_shared<MultiShape>(manyShapes.begin(), manyShapes.end(), Vector3f(0, 10.0f, 0));
    // shape->takeOwnership();
    shapes.emplace_back(shape);

    // Clone the array for a second set and change the ID.
    id = nextId++;
    for (size_t i = 0; i < manyShapes.size(); ++i)
    {
      manyShapes[i] = manyShapes[i]->clone();
      manyShapes[i]->setId(id);
    }
    shape =
      std::make_shared<MultiShape>(manyShapes.begin(), manyShapes.end(), Vector3f(-10.0f, 5.0f, 0));
    shape->takeOwnership();
    shapes.emplace_back(shape);
  }

  if (haveOption("wire", argc, argv))
  {
    for (size_t i = initialShapeCount; i < shapes.size(); ++i)
    {
      shapes[i]->setWireframe(true);
    }
  }

  // Position the shapes so they aren't all on top of one another.
  if (shapes.size() > initialShapeCount)
  {
    Vector3f pos(0.0f);
    const float spacing = 2.0f;
    pos.x() -= spacing * float((shapes.size() - initialShapeCount) / 2u);

    for (size_t i = initialShapeCount; i < shapes.size(); ++i)
    {
      // Set position if not already set.
      if (shapes[i]->position().isEqual(Vector3f::Zero))
      {
        shapes[i]->setPosition(pos);
        pos.x() += spacing;
      }
    }

    for (auto &mover : movers)
    {
      mover->reset();
    }
  }


  // Add text after positioning and mover changes to keep fixed positions.
  if (allShapes || haveOption("text2d", argc, argv))
  {
    std::shared_ptr<Text2D> text;
    text = std::make_shared<Text2D>("Hello Screen", Id(nextId++, CatText2D),
                                    Spherical(Vector3f(0.25f, 0.75f, 0.0f)));
    shapes.emplace_back(text);
    text = std::make_shared<Text2D>("Hello World 2D", Id(nextId++, CatText2D),
                                    Spherical(Vector3f(1.0f, 1.0f, 1.0f)));
    text->setInWorldSpace(true);
    shapes.emplace_back(text);
  }

  if (allShapes || haveOption("text3d", argc, argv))
  {
    std::shared_ptr<Text3D> text;
    text = std::make_shared<Text3D>(
      "Hello World 3D", Id(nextId++, CatText3D),
      Directional(Vector3f(-1.0f, -1.0f, 1.0f), Vector3f(0, 1, 0), 1.0f, 8.0f));
    shapes.emplace_back(text);
    text = std::make_shared<Text3D>("Hello World 3D Facing", Id(nextId++, CatText3D),
                                    Directional(Vector3f(-1.0f, -1.0f, 0.0f), 1.0f, 8.0f));
    text->setScreenFacing(true);
    shapes.emplace_back(text);
  }

  // Did we create anything?
  if (initialShapeCount == shapes.size())
  {
    // Nothing created. Create the default shape by providing some fake arguments.
    const char *defaultArgv[] = { "this arg is not read", "sphere" };

    createShapes(nextId, shapes, movers, resources, sizeof(defaultArgv) / sizeof(defaultArgv[0]),
                 defaultArgv);
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
  std::cout << "  file: Save a file stream to 'server-test.3es'\n";
  std::cout << "  noaxes: Don't create axis arrow objects\n";
  std::cout << "  nomove: don't move objects (keep stationary)\n";
  std::cout << "  wire: Show wireframe shapes, not slide for relevant objects\n";
  std::cout << "\nValid shapes:\n";
  std::cout << "\tall: show all shapes\n";
  std::cout << "\tarrow\n";
  std::cout << "\tbox\n";
  std::cout << "\tcapsule\n";
  std::cout << "\tcloud\n";
  std::cout << "\tcloudpart\n";
  std::cout << "\tcone\n";
  std::cout << "\tcylinder\n";
  std::cout << "\tlines\n";
  std::cout << "\tmesh\n";
  std::cout << "\tmulti (2000 capsules)\n";
  std::cout << "\tplane\n";
  std::cout << "\tpoints\n";
  std::cout << "\tsphere\n";
  std::cout << "\tstar\n";
  std::cout << "\ttext2d\n";
  std::cout << "\ttext3d\n";
  std::cout << "\ttriangles\n";
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
  std::vector<std::shared_ptr<ShapeMover>> movers;
  std::vector<std::shared_ptr<Resource>> resources;

  unsigned nextId = 1;
  createAxes(nextId, shapes, movers, resources, argc, argv);
  createShapes(nextId, shapes, movers, resources, argc, argv);

  const unsigned targetFrameTimeMs = 1000 / 30;
  float time = 0;
  auto lastTime = std::chrono::system_clock::now();
  auto onNewConnection = [&shapes](Server & /*server*/, Connection &connection) {
    // Test categories API.
    defineCategory(&connection, "3D", Cat3D, CatRoot, true);
    defineCategory(&connection, "Text", CatText, CatRoot, true);
    defineCategory(&connection, "Primitives", CatSimple3D, Cat3D, true);
    defineCategory(&connection, "Mesh Based", CatComplex3D, Cat3D, true);
    defineCategory(&connection, "Arrows", CatArrow, CatSimple3D, true);
    defineCategory(&connection, "Boxes", CatBox, CatSimple3D, true);
    defineCategory(&connection, "Capsules", CatCapsule, CatSimple3D, true);
    defineCategory(&connection, "Cylinders", CatCylinder, CatSimple3D, true);
    defineCategory(&connection, "Cones", CatCone, CatSimple3D, true);
    defineCategory(&connection, "Lines", CatLines, CatComplex3D, true);
    defineCategory(&connection, "Meshes", CatMesh, CatComplex3D, true);
    defineCategory(&connection, "Planes", CatPlane, CatSimple3D, true);
    defineCategory(&connection, "Points", CatPoints, CatComplex3D, true);
    defineCategory(&connection, "Pose", CatPose, CatSimple3D, true);
    defineCategory(&connection, "Spheres", CatSphere, CatSimple3D, true);
    defineCategory(&connection, "Stars", CatStar, CatSimple3D, true);
    defineCategory(&connection, "Text2D", CatText2D, CatText, true);
    defineCategory(&connection, "Text3D", CatText3D, CatText, true);
    defineCategory(&connection, "Triangles", CatTriangles, CatComplex3D, true);
    for (auto &shape : shapes)
    {
      connection.create(*shape);
    }
  };

  server->connectionMonitor()->setConnectionCallback(onNewConnection);

  if (!server->connectionMonitor()->start(tes::ConnectionMode::Asynchronous))
  {
    std::cerr << "Failed to start listening." << std::endl;
    return 1;
  }
  std::cout << "Listening on port " << server->connectionMonitor()->port() << std::endl;

  if (haveOption("file", argc, argv))
  {
    // Record to file stream.
    server->connectionMonitor()->openFileStream("server-test.3es");
  }

  // Register shapes with server.
  for (auto &shape : shapes)
  {
    server->create(*shape);
  }

  while (!quit)
  {
    auto now = std::chrono::system_clock::now();
    auto elapsed = now - lastTime;

    lastTime = now;
    float dt =
      float(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()) * 1e-6f;
    time += dt;

    for (auto &mover : movers)
    {
      mover->update(time, dt);
      server->update(*mover->shape());
    }

    server->updateFrame(dt);
    if (server->connectionMonitor()->mode() == tes::ConnectionMode::Synchronous)
    {
      server->connectionMonitor()->monitorConnections();
    }
    server->connectionMonitor()->commitConnections();
    server->updateTransfers(64 * 1024);

    printf("\rFrame %f: %u connection(s)    ", dt, server->connectionCount());
    fflush(stdout);

    now = std::chrono::system_clock::now();
    elapsed = now - lastTime;
    unsigned elapsedMs =
      unsigned(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
    unsigned sleepTimeMs = (elapsedMs <= targetFrameTimeMs) ? targetFrameTimeMs - elapsedMs : 0u;
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
  }

  movers.clear();
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
