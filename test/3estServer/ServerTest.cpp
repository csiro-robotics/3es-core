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

// NOLINTBEGIN(readability-magic-numbers, readability-function-cognitive-complexity)

namespace
{
bool g_quit = false;

void onSignal(int arg)
{
  if (arg == SIGINT || arg == SIGTERM)
  {
    g_quit = true;
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
  ShapeMover(std::shared_ptr<tes::Shape> shape)
    : _shape(std::move(shape))
  {}

  virtual ~ShapeMover() = default;

  [[nodiscard]] std::shared_ptr<tes::Shape> shape() { return _shape; }
  [[nodiscard]] std::shared_ptr<const tes::Shape> shape() const { return _shape; }
  void setShape(std::shared_ptr<tes::Shape> shape)
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
  std::shared_ptr<tes::Shape> _shape;
};


class Oscilator : public ShapeMover
{
public:
  Oscilator(std::shared_ptr<tes::Shape> shape, float amplitude = 1.0f, float period = 5.0f,
            const tes::Vector3f &axis = tes::Vector3f(0, 0, 1))
    : ShapeMover(std::move(shape))
    , _reference_pos(tes::Vector3f::Zero)
    , _axis(axis)
    , _amplitude(amplitude)
    , _period(period)
  {
    if (!this->shape())
    {
      _reference_pos = tes::Vector3f(this->shape()->position());
    }
    onShapeChange();
  }

  [[nodiscard]] const tes::Vector3f &axis() const { return _axis; }
  [[nodiscard]] const tes::Vector3f &referencePos() const { return _reference_pos; }
  [[nodiscard]] float amplitude() const { return _amplitude; }
  [[nodiscard]] float period() const { return _period; }

  void reset() override
  {
    _reference_pos = shape() ? tes::Vector3f(shape()->position()) : _reference_pos;
  }

  void update(float time, float dt) override
  {
    TES_UNUSED(dt);
    const tes::Vector3f pos = _reference_pos + _amplitude * std::sin(time) * _axis;
    shape()->setPosition(pos);
  }

protected:
  void onShapeChange() override
  {
    _reference_pos = (shape()) ? tes::Vector3f(shape()->position()) : _reference_pos;
  }

private:
  tes::Vector3f _reference_pos;
  tes::Vector3f _axis;
  float _amplitude = 1.0f;
  float _period = 5.0f;
};


std::shared_ptr<tes::MeshResource> createTestMesh()
{
  auto mesh = std::make_shared<tes::SimpleMesh>(
    1, 4, 6, tes::DtTriangles,
    tes::SimpleMesh::Vertex | tes::SimpleMesh::Index | tes::SimpleMesh::Colour);
  mesh->setVertex(0, tes::Vector3f(-0.5f, 0, -0.5f));
  mesh->setVertex(1, tes::Vector3f(0.5f, 0, -0.5f));
  mesh->setVertex(2, tes::Vector3f(0.5f, 0, 0.5f));
  mesh->setVertex(3, tes::Vector3f(-0.5f, 0, 0.5f));

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

  // mesh->setNormal(0, tes::Vector3f(0, 1, 0));
  // mesh->setNormal(1, tes::Vector3f(0, 1, 0));
  // mesh->setNormal(2, tes::Vector3f(0, 1, 0));
  // mesh->setNormal(3, tes::Vector3f(0, 1, 0));

  return mesh;
}


std::shared_ptr<tes::MeshResource> createTestCloud(float draw_scale = 0.0f)
{
  auto cloud = std::make_shared<tes::PointCloud>(2);  // Considered a Mesh for ID purposes.
  cloud->resize(8);

  cloud->setPoint(0, tes::Vector3f(0, 0, 0), tes::Vector3f(0, 0, 1), tes::Colour(0, 0, 0));
  cloud->setPoint(1, tes::Vector3f(1, 0, 0), tes::Vector3f(0, 0, 1), tes::Colour(255, 0, 0));
  cloud->setPoint(2, tes::Vector3f(0, 1, 0), tes::Vector3f(0, 0, 1), tes::Colour(0, 255, 0));
  cloud->setPoint(3, tes::Vector3f(0, 0, 1), tes::Vector3f(0, 0, 1), tes::Colour(0, 0, 255));
  cloud->setPoint(4, tes::Vector3f(1, 1, 0), tes::Vector3f(0, 0, 1), tes::Colour(255, 255, 0));
  cloud->setPoint(5, tes::Vector3f(0, 1, 1), tes::Vector3f(0, 0, 1), tes::Colour(0, 255, 255));
  cloud->setPoint(6, tes::Vector3f(1, 0, 1), tes::Vector3f(0, 0, 1), tes::Colour(255, 0, 255));
  cloud->setPoint(7, tes::Vector3f(1, 1, 1), tes::Vector3f(0, 0, 1), tes::Colour(255, 255, 255));

  cloud->setDrawScale(draw_scale);

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


void createAxes(unsigned &next_id, std::vector<std::shared_ptr<tes::Shape>> &shapes,
                std::vector<std::shared_ptr<ShapeMover>> &movers,
                std::vector<std::shared_ptr<tes::Resource>> &resources, int argc, const char **argv)
{
  TES_UNUSED(movers);
  TES_UNUSED(resources);
  if (!haveOption("noaxes", argc, argv))
  {
    const float arrow_length = 1.0f;
    const float arrow_radius = 0.025f;
    const tes::Vector3f pos(0.0f);
    std::shared_ptr<tes::Arrow> arrow;

    arrow = std::make_shared<tes::Arrow>(
      next_id++, tes::Directional(pos, tes::Vector3f(1, 0, 0), arrow_radius, arrow_length));
    arrow->setColour(tes::Colour(tes::Colour::Red));
    shapes.emplace_back(arrow);

    arrow = std::make_shared<tes::Arrow>(
      next_id++, tes::Directional(pos, tes::Vector3f(0, 1, 0), arrow_radius, arrow_length));
    arrow->setColour(tes::Colour(tes::Colour::ForestGreen));
    shapes.emplace_back(arrow);

    arrow = std::make_shared<tes::Arrow>(
      next_id++, tes::Directional(pos, tes::Vector3f(0, 0, 1), arrow_radius, arrow_length));
    arrow->setColour(tes::Colour(tes::Colour::DodgerBlue));
    shapes.emplace_back(arrow);
  }
}


void createShapes(unsigned &next_id, std::vector<std::shared_ptr<tes::Shape>> &shapes,
                  std::vector<std::shared_ptr<ShapeMover>> &movers,
                  std::vector<std::shared_ptr<tes::Resource>> &resources, int argc,
                  const char **argv)
{
  const bool all_shapes = haveOption("all", argc, argv);
  const bool no_move = haveOption("nomove", argc, argv);
  const size_t initial_shape_count = shapes.size();

  if (all_shapes || haveOption("arrow", argc, argv))
  {
    auto arrow = std::make_shared<tes::Arrow>(tes::Id(next_id++, CatArrow));
    arrow->setRadius(0.5f);
    arrow->setLength(1.0f);
    arrow->setColour(tes::Colour(tes::Colour::SeaGreen));
    shapes.emplace_back(arrow);
    if (!no_move)
    {
      movers.emplace_back(std::make_shared<Oscilator>(arrow, 2.0f, 2.5f));
    }
  }

  if (all_shapes || haveOption("box", argc, argv))
  {
    auto box = std::make_shared<tes::Box>(tes::Id(next_id++, CatBox));
    box->setScale(tes::Vector3f(0.45f));
    box->setColour(tes::Colour(tes::Colour::MediumSlateBlue));
    shapes.emplace_back(box);
    if (!no_move)
    {
      movers.emplace_back(std::make_shared<Oscilator>(box, 2.0f, 2.5f));
    }
  }

  if (all_shapes || haveOption("capsule", argc, argv))
  {
    auto capsule =
      std::make_shared<tes::Capsule>(tes::Id(next_id++, CatCapsule), tes::Directional::identity());
    capsule->setLength(2.0f);
    capsule->setRadius(0.3f);
    capsule->setColour(tes::Colour(tes::Colour::LavenderBlush));
    shapes.emplace_back(capsule);
    if (!no_move)
    {
      movers.emplace_back(std::make_shared<Oscilator>(capsule, 2.0f, 2.5f));
    }
  }

  if (all_shapes || haveOption("cone", argc, argv))
  {
    auto cone = std::make_shared<tes::Cone>(tes::Id(next_id++, CatCone));
    cone->setLength(2.0f);
    cone->setRadius(0.25f);
    cone->setColour(tes::Colour(tes::Colour::SandyBrown));
    shapes.emplace_back(cone);
    if (!no_move)
    {
      movers.emplace_back(std::make_shared<Oscilator>(cone, 2.0f, 2.5f));
    }
  }

  if (all_shapes || haveOption("cylinder", argc, argv))
  {
    auto cylinder = std::make_shared<tes::Cylinder>(tes::Id(next_id++, CatCylinder));
    cylinder->setScale(tes::Vector3f(0.45f));
    cylinder->setColour(tes::Colour(tes::Colour::FireBrick));
    shapes.emplace_back(cylinder);
    if (!no_move)
    {
      movers.emplace_back(std::make_shared<Oscilator>(cylinder, 2.0f, 2.5f));
    }
  }

  if (all_shapes || haveOption("plane", argc, argv))
  {
    auto plane = std::make_shared<tes::Plane>(tes::Id(next_id++, CatPlane));
    plane->setNormal(tes::Vector3f(1.0f, 1.0f, 0.0f).normalised());
    plane->setScale(1.5f);
    plane->setNormalLength(0.5f);
    plane->setColour(tes::Colour(tes::Colour::LightSlateGrey));
    shapes.emplace_back(plane);
    if (!no_move)
    {
      movers.emplace_back(std::make_shared<Oscilator>(plane, 2.0f, 2.5f));
    }
  }

  if (all_shapes || haveOption("pose", argc, argv))
  {
    auto pose = std::make_shared<tes::Pose>(tes::Id(next_id++, CatPose));
    pose->setRotation(
      tes::Quaternionf().setAxisAngle(tes::Vector3f::AxisZ, static_cast<float>(0.25 * M_PI)));
    shapes.emplace_back(pose);
    if (!no_move)
    {
      movers.emplace_back(std::make_shared<Oscilator>(pose, 2.0f, 2.5f));
    }
  }

  if (all_shapes || haveOption("sphere", argc, argv))
  {
    auto sphere = std::make_shared<tes::Sphere>(tes::Id(next_id++, CatSphere));
    sphere->setRadius(0.75f);
    sphere->setColour(tes::Colour(tes::Colour::Coral));
    shapes.emplace_back(sphere);
    if (!no_move)
    {
      movers.emplace_back(std::make_shared<Oscilator>(sphere, 2.0f, 2.5f));
    }
  }

  if (all_shapes || haveOption("star", argc, argv))
  {
    auto star = std::make_shared<tes::Star>(tes::Id(next_id++, CatStar));
    star->setRadius(0.75f);
    star->setColour(tes::Colour(tes::Colour::DarkGreen));
    shapes.emplace_back(star);
    if (!no_move)
    {
      movers.emplace_back(std::make_shared<Oscilator>(star, 2.0f, 2.5f));
    }
  }

  if (all_shapes || haveOption("lines", argc, argv))
  {
    const std::array<tes::Vector3f, 6> line_set = {
      tes::Vector3f(0, 0, 0),        tes::Vector3f(0, 0, 1), tes::Vector3f(0, 0, 1),
      tes::Vector3f(0.25f, 0, 0.8f), tes::Vector3f(0, 0, 1), tes::Vector3f(-0.25f, 0, 0.8f)
    };
    auto lines = std::make_shared<tes::MeshShape>(tes::DtLines, tes::Id(next_id++, CatLines),
                                                  tes::DataBuffer(line_set));
    shapes.emplace_back(lines);
    // if (!no_move)
    // {
    //   movers.emplace_back(std::make_shared<Oscilator>(mesh, 2.0f, 2.5f));
    // }
  }

  if (all_shapes || haveOption("triangles", argc, argv))
  {
    const std::array<tes::Vector3f, 12> triangle_set = {
      tes::Vector3f(0, 0, 0), tes::Vector3f(0, 0.25f, 1),  tes::Vector3f(0.25f, 0, 1),
      tes::Vector3f(0, 0, 0), tes::Vector3f(-0.25f, 0, 1), tes::Vector3f(0, 0.25f, 1),
      tes::Vector3f(0, 0, 0), tes::Vector3f(0, -0.25f, 1), tes::Vector3f(-0.25f, 0, 1),
      tes::Vector3f(0, 0, 0), tes::Vector3f(0.25f, 0, 1),  tes::Vector3f(0, -0.25f, 1)
    };
    const std::array<tes::Colour, triangle_set.size()> colours = {
      tes::Colour(tes::Colour::Red),   tes::Colour(tes::Colour::Red),
      tes::Colour(tes::Colour::Red),   tes::Colour(tes::Colour::Green),
      tes::Colour(tes::Colour::Green), tes::Colour(tes::Colour::Green),
      tes::Colour(tes::Colour::Blue),  tes::Colour(tes::Colour::Blue),
      tes::Colour(tes::Colour::Blue),  tes::Colour(tes::Colour::White),
      tes::Colour(tes::Colour::White), tes::Colour(tes::Colour::White)
    };
    auto triangles = std::make_shared<tes::MeshShape>(
      tes::DtTriangles, tes::Id(next_id++, CatTriangles), tes::DataBuffer(triangle_set));
    triangles->setColours(colours);
    triangles->duplicateArrays();
    shapes.emplace_back(triangles);
    // if (!no_move)
    // {
    //   movers.emplace_back(std::make_shared<Oscilator>(mesh, 2.0f, 2.5f));
    // }
  }

  if (all_shapes || haveOption("mesh", argc, argv))
  {
    auto mesh_res = createTestMesh();
    resources.emplace_back(mesh_res);
    const unsigned part_count = 2;
    auto mesh = std::make_shared<tes::MeshSet>(tes::Id(next_id++, CatMesh), part_count);
    mesh->setPart(0, mesh_res, tes::Matrix4f::Identity, tes::Colour(tes::Colour::YellowGreen));
    mesh->setPart(1, mesh_res, tes::Matrix4f::translation(tes::Vector3f(0, 0, 1.5f)),
                  tes::Colour(tes::Colour::SkyBlue));
    shapes.emplace_back(mesh);
    // if (!no_move)
    // {
    //   movers.emplace_back(std::make_shared<Oscilator>(mesh, 2.0f, 2.5f));
    // }
    std::cout << "make mesh" << std::endl;
  }

  if (all_shapes || haveOption("points", argc, argv))
  {
    const std::array<tes::Vector3f, 5> pts = {
      tes::Vector3f(0, 0.25f, 1), tes::Vector3f(0.25f, 0, 1), tes::Vector3f(-0.25f, 0, 1),
      tes::Vector3f(0, -0.25f, 1), tes::Vector3f(0, -0.25f, 1)
    };
    const std::array<tes::Colour, pts.size()> colours = { tes::Colour(tes::Colour::Black),
                                                          tes::Colour(tes::Colour::Red),
                                                          tes::Colour(tes::Colour::Green),
                                                          tes::Colour(tes::Colour::Blue),
                                                          tes::Colour(tes::Colour::White) };
    auto points = std::make_shared<tes::MeshShape>(tes::DtPoints, tes::Id(next_id++, CatPoints),
                                                   tes::DataBuffer(pts));
    points->setColours(colours);
    points->setDrawScale(3.0f);
    shapes.emplace_back(points);
    // if (!no_move)
    // {
    //   movers.emplace_back(std::make_shared<Oscilator>(mesh, 2.0f, 2.5f));
    // }
  }

  if (all_shapes || haveOption("voxels", argc, argv))
  {
    const std::array<tes::Vector3f, 5> pts = {
      tes::Vector3f(0, 0.25f, 1), tes::Vector3f(0.25f, 0, 1), tes::Vector3f(-0.25f, 0, 1),
      tes::Vector3f(0, -0.25f, 1), tes::Vector3f(0, -0.25f, 1)
    };
    const std::array<tes::Colour, pts.size()> colours = { tes::Colour(tes::Colour::Black),
                                                          tes::Colour(tes::Colour::Red),
                                                          tes::Colour(tes::Colour::Green),
                                                          tes::Colour(tes::Colour::Blue),
                                                          tes::Colour(tes::Colour::White) };
    auto points = std::make_shared<tes::MeshShape>(tes::DtVoxels, tes::Id(next_id++, CatPoints),
                                                   tes::DataBuffer(pts));
    points->setColours(colours);
    points->setDrawScale(0.2f);
    shapes.emplace_back(points);
    // if (!no_move)
    // {
    //   movers.emplace_back(std::make_shared<Oscilator>(mesh, 2.0f, 2.5f));
    // }
  }

  if (all_shapes || haveOption("cloud", argc, argv))
  {
    auto cloud = createTestCloud(16.0f);
    auto points = std::make_shared<tes::MeshSet>(cloud, tes::Id(next_id++, CatPoints));
    shapes.emplace_back(points);
    resources.emplace_back(cloud);
    // if (!no_move)
    // {
    //   movers.emplace_back(std::make_shared<Oscilator>(points, 2.0f, 2.5f));
    // }
  }

  if (haveOption("multi", argc, argv))
  {
    // Set the block size to ensure we are larger than the multi-shape packet size.
    const int block_size = 15;
    const int many_count = block_size * block_size * block_size;
    const float separation = 0.3f;
    const float block_offset = -0.5f * block_size * separation;

    std::vector<std::shared_ptr<tes::Shape>> many_shapes(many_count);
    unsigned id = next_id++;
    unsigned i = 0;
    for (int z = 0; z < block_size; ++z)
    {
      for (int y = 0; y < block_size; ++y)
      {
        for (int x = 0; x < block_size; ++x)
        {
          tes::Vector3f pos;
          pos.x() = block_offset + static_cast<float>(x) * separation;
          pos.y() = block_offset + static_cast<float>(y) * separation;
          pos.z() = block_offset + static_cast<float>(z) * separation;

          auto capsule = std::make_shared<tes::Capsule>(tes::Id(id, CatCapsule),
                                                        tes::Directional::identity(false));
          capsule->setLength(0.4f);
          capsule->setRadius(0.15f);
          capsule->setColour(tes::ColourSet::predefined(tes::ColourSet::Standard).cycle(i));
          capsule->setPosition(pos);
          many_shapes[i++] = capsule;
        }
      }
    }

    auto shape = std::make_shared<tes::MultiShape>(many_shapes.begin(), many_shapes.end(),
                                                   tes::Vector3f(0, 10.0f, 0));
    // shape->takeOwnership();
    shapes.emplace_back(shape);

    // Clone the array for a second set and change the ID.
    id = next_id++;
    for (auto &shape : many_shapes)
    {
      shape = shape->clone();
      shape->setId(id);
    }
    shape = std::make_shared<tes::MultiShape>(many_shapes.begin(), many_shapes.end(),
                                              tes::Vector3f(-10.0f, 5.0f, 0));
    shape->takeOwnership();
    shapes.emplace_back(shape);
  }

  if (haveOption("wire", argc, argv))
  {
    for (size_t i = initial_shape_count; i < shapes.size(); ++i)
    {
      shapes[i]->setWireframe(true);
    }
  }

  // Position the shapes so they aren't all on top of one another.
  if (shapes.size() > initial_shape_count)
  {
    tes::Vector3f pos(0.0f);
    const float spacing = 2.0f;
    pos.x() -= spacing * 0.5f * static_cast<float>(shapes.size() - initial_shape_count);

    for (size_t i = initial_shape_count; i < shapes.size(); ++i)
    {
      // Set position if not already set.
      if (shapes[i]->position().isEqual(tes::Vector3f::Zero))
      {
        shapes[i]->setPosition(pos);
        // std::cout << "Positioned " << typeid(*shapes[i]).name() << " at "
        //           << shapes[i]->position().x() << "," << shapes[i]->position().y() << ","
        //           << shapes[i]->position().z() << "," << std::endl;
        pos.x() += spacing;
      }
    }

    for (auto &mover : movers)
    {
      mover->reset();
    }
  }


  // Add text after positioning and mover changes to keep fixed positions.
  if (all_shapes || haveOption("text2d", argc, argv))
  {
    std::shared_ptr<tes::Text2D> text;
    text = std::make_shared<tes::Text2D>("Hello Screen", tes::Id(next_id++, CatText2D),
                                         tes::Spherical(tes::Vector3f(0.25f, 0.75f, 0.0f)));
    shapes.emplace_back(text);
    text = std::make_shared<tes::Text2D>("Hello World 2D", tes::Id(next_id++, CatText2D),
                                         tes::Spherical(tes::Vector3f(1.0f, 1.0f, 1.0f)));
    text->setInWorldSpace(true);
    shapes.emplace_back(text);
  }

  if (all_shapes || haveOption("text3d", argc, argv))
  {
    std::shared_ptr<tes::Text3D> text;
    text = std::make_shared<tes::Text3D>(
      "Hello World 3D", tes::Id(next_id++, CatText3D),
      tes::Directional(tes::Vector3f(-1.0f, -1.0f, 1.0f), tes::Vector3f(0, 1, 0), 1.0f, 8.0f));
    shapes.emplace_back(text);
    text = std::make_shared<tes::Text3D>(
      "Hello World 3D Facing", tes::Id(next_id++, CatText3D),
      tes::Directional(tes::Vector3f(-1.0f, -1.0f, 0.0f), 1.0f, 8.0f));
    text->setScreenFacing(true);
    shapes.emplace_back(text);
  }

  // Did we create anything?
  if (initial_shape_count == shapes.size())
  {
    // Nothing created. Create the default shape by providing some fake arguments.
    std::array<const char *, 2> default_argv = { "this arg is not read", "sphere" };

    createShapes(next_id, shapes, movers, resources, static_cast<int>(default_argv.size()),
                 default_argv.data());
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


int main(int argc, char **argv_non_const)
{
  const char **argv = const_cast<const char **>(argv_non_const);
  signal(SIGINT, &onSignal);
  signal(SIGTERM, &onSignal);

  if (haveOption("help", argc, argv))
  {
    showUsage(argc, argv_non_const);
    return 0;
  }

  tes::ServerInfoMessage info;
  tes::initDefaultServerInfo(&info);
  info.coordinate_frame = tes::XYZ;
  unsigned server_flags = tes::SFDefaultNoCompression;
  if (haveOption("compress", argc, argv))
  {
    server_flags |= tes::SFCompress;
  }
  auto server = tes::Server::create(tes::ServerSettings(server_flags), &info);

  std::vector<std::shared_ptr<tes::Shape>> shapes;
  std::vector<std::shared_ptr<ShapeMover>> movers;
  std::vector<std::shared_ptr<tes::Resource>> resources;

  unsigned next_id = 1;
  createAxes(next_id, shapes, movers, resources, argc, argv);
  createShapes(next_id, shapes, movers, resources, argc, argv);

  const unsigned target_frame_time_ms = 1000 / 30;
  float time = 0;
  auto last_time = std::chrono::system_clock::now();
  auto on_new_connection = [&shapes](tes::Server & /*server*/, tes::Connection &connection) {
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

  server->connectionMonitor()->setConnectionCallback(on_new_connection);

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

  while (!g_quit)
  {
    auto now = std::chrono::system_clock::now();
    auto elapsed = now - last_time;

    last_time = now;
    const float dt =
      static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()) *
      1e-6f;
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
    elapsed = now - last_time;
    const unsigned elapsed_ms =
      static_cast<unsigned>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
    const unsigned sleep_time_ms =
      (elapsed_ms <= target_frame_time_ms) ? target_frame_time_ms - elapsed_ms : 0u;
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_ms));
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

// NOLINTEND(readability-magic-numbers, readability-function-cognitive-complexity)
