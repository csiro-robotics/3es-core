//
// author: Kazys Stepanas
//
#define TES_ENABLE
#include <3escore/ServerApi.h>
#include <3escore/Vector3.h>
#include <3escore/VectorHash.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <csignal>
#include <iostream>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

#define TEXT_ID 1u
#define SPHERE_ID 2u

// Example to view a sphere tessellation. This code duplicates tessellate/3essphere code and adds
// 3ES commands.
using Vector3f = tes::Vector3f;

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

tes::ServerPtr tes_server;
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


void showUsage(int argc, char **argv)
{
  TES_UNUSED(argc);
  std::cout << "Usage:\n";
  std::cout << argv[0] << " [options] [iterations]\n";
  std::cout << "\nValid options:\n";
  std::cout << "  help: show this message\n";
  std::cout << "  collate: use packet collation.\n";
  TES_IF(tes::featureFlag(tes::TFeatureCompression))
  {
    std::cout << "  compress: collated and compress packets (implies collation).\n";
  }
  std::cout.flush();
}

namespace
{
struct SphereVertexHash
{
  inline size_t operator()(const Vector3f &v) const
  {
    return tes::vhash::hash(v.x(), v.y(), v.z());
  }
};

using SphereVertexMap = std::unordered_multimap<Vector3f, unsigned, SphereVertexHash>;


unsigned tesUnrollDisplay(const std::vector<Vector3f> &vertices,
                          const std::vector<unsigned> &indices)
{
  unsigned shape_count = 0;
  // Maximum 6500 vertices per message. Take it down to the nearest multiple of 3 (triangle).
  const size_t send_limit = 64998;
  size_t cursor = 0;
  size_t count = 0;

  std::vector<Vector3f> local_vertices(send_limit);
  while (cursor < indices.size())
  {
    count = std::min(indices.size() - cursor, send_limit);
    // Copy vertices into local_vertices.
    for (size_t i = 0; i < count; ++i)
    {
      local_vertices[i] = vertices[indices[cursor + i]];
    }

    cursor += count;

    TES_STMT(tes::create(tes_server, tes::MeshShape(tes::DtTriangles, tes::Id(SPHERE_ID),
                                                    tes::DataBuffer(local_vertices))
                                       .setColour(tes::Colour(200, 200, 200))));
    ++shape_count;
  }

  return shape_count;
}


/// Add a vertex to @p points, reusing an existing vertex is a matching one is found.
///
/// This first searches for a matching vertex in @p point and returns its index if found.
/// Otherwise a new vertex is added.
///
/// @param vertex The vertex to add.
/// @param vertices The vertex data to add to.
/// @return The index which can be used to refer to the target vertex.
unsigned insertVertex(const Vector3f &vertex, std::vector<Vector3f> &vertices,
                      SphereVertexMap &vertex_map)
{
  auto find_result = vertex_map.find(vertex);
  size_t hashVal = tes::vhash::hash(vertex.x(), vertex.y(), vertex.z());
  if (find_result != vertex_map.end())
  {
    do
    {
      if (find_result->first.isEqual(vertex, 0))
      {
        return find_result->second;
      }
      ++find_result;
    } while (find_result != vertex_map.end() &&
             tes::vhash::hash(find_result->first.x(), find_result->first.y(),
                              find_result->first.z()) == hashVal);
  }


  // Add new vertex.
  unsigned idx = unsigned(vertices.size());
  vertices.push_back(vertex);
  vertex_map.insert(std::make_pair(vertex, idx));
  return idx;
}

void sphereInitialise(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
                      SphereVertexMap *vertex_map)
{
  // We start with two hexagonal rings to approximate the sphere.
  // All subdivision occurs on a unit radius sphere, at the origin. We translate and
  // scale the vertices at the end.
  vertices.clear();
  indices.clear();

  static const float ring_control_angle = 25.0f / 180.0f * float(M_PI);
  static const float ring_height = std::sin(ring_control_angle);
  static const float ring_radius = std::cos(ring_control_angle);
  static const float hex_angle = 2.0f * float(M_PI) / 6.0f;
  static const float ring_to_offset_angle = 0.5f * hex_angle;
  static const std::array<Vector3f, 14> initial_vertices = {
    Vector3f(0, 0, 1),

    // Upper hexagon.
    Vector3f(ring_radius, 0, ring_height),
    Vector3f(ring_radius * std::cos(hex_angle), ring_radius * std::sin(hex_angle), ring_height),
    Vector3f(ring_radius * std::cos(2 * hex_angle), ring_radius * std::sin(2 * hex_angle),
             ring_height),
    Vector3f(ring_radius * std::cos(3 * hex_angle), ring_radius * std::sin(3 * hex_angle),
             ring_height),
    Vector3f(ring_radius * std::cos(4 * hex_angle), ring_radius * std::sin(4 * hex_angle),
             ring_height),
    Vector3f(ring_radius * std::cos(5 * hex_angle), ring_radius * std::sin(5 * hex_angle),
             ring_height),

    // Lower hexagon.
    Vector3f(ring_radius * std::cos(ring_to_offset_angle),
             ring_radius * std::sin(ring_to_offset_angle), -ring_height),
    Vector3f(ring_radius * std::cos(ring_to_offset_angle + hex_angle),
             ring_radius * std::sin(ring_to_offset_angle + hex_angle), -ring_height),
    Vector3f(ring_radius * std::cos(ring_to_offset_angle + 2 * hex_angle),
             ring_radius * std::sin(ring_to_offset_angle + 2 * hex_angle), -ring_height),
    Vector3f(ring_radius * std::cos(ring_to_offset_angle + 3 * hex_angle),
             ring_radius * std::sin(ring_to_offset_angle + 3 * hex_angle), -ring_height),
    Vector3f(ring_radius * std::cos(ring_to_offset_angle + 4 * hex_angle),
             ring_radius * std::sin(ring_to_offset_angle + 4 * hex_angle), -ring_height),
    Vector3f(ring_radius * std::cos(ring_to_offset_angle + 5 * hex_angle),
             ring_radius * std::sin(ring_to_offset_angle + 5 * hex_angle), -ring_height),

    Vector3f(0, 0, -1),
  };
  const auto initial_vertex_count = initial_vertices.size();

  const std::array<unsigned, 72> initial_indices = {
    0, 1,  2, 0, 2,  3, 0, 3,  4,  0,  4,  5,  0,  5,  6,  0,  6,  1,

    1, 7,  2, 2, 8,  3, 3, 9,  4,  4,  10, 5,  5,  11, 6,  6,  12, 1,

    7, 8,  2, 8, 9,  3, 9, 10, 4,  10, 11, 5,  11, 12, 6,  12, 7,  1,

    7, 13, 8, 8, 13, 9, 9, 13, 10, 10, 13, 11, 11, 13, 12, 12, 13, 7
  };
  const auto initial_index_count = initial_indices.size();

  for (unsigned i = 0; i < static_cast<unsigned>(initial_vertex_count); ++i)
  {
    vertices.push_back(initial_vertices[i]);
    if (vertex_map)
    {
      vertex_map->insert(std::make_pair(initial_vertices[i], i));
    }
  }

  for (unsigned i = 0; i < static_cast<unsigned>(initial_index_count); i += 3)
  {
    indices.push_back(initial_indices[i + 0]);
    indices.push_back(initial_indices[i + 1]);
    indices.push_back(initial_indices[i + 2]);
  }

  // Send the initial sphere. We know it has less than 65k vertices.
  if (!vertices.empty() && !indices.empty())
  {
    TES_STMT(tes::create(
      tes_server, tes::MeshShape(tes::DtTriangles, tes::Id(SPHERE_ID), tes::DataBuffer(vertices))
                    .setColour(tes::Colour(200, 200, 200))));
  }
}


void subdivideUnitSphere(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices,
                         SphereVertexMap &vertex_map)
{
  const unsigned triangle_count = unsigned(indices.size() / 3);
  unsigned triangle[3];
  unsigned abc[3];
  unsigned def[3];
  Vector3f verts[3];
  Vector3f new_vertices[3];

  for (unsigned i = 0; i < triangle_count; ++i)
  {
    triangle[0] = abc[0] = indices[i * 3 + 0];
    triangle[1] = abc[1] = indices[i * 3 + 1];
    triangle[2] = abc[2] = indices[i * 3 + 2];

    // Fetch the vertices.
    verts[0] = vertices[triangle[0]];
    verts[1] = vertices[triangle[1]];
    verts[2] = vertices[triangle[2]];

    // Highlight the working triangle: extrude it a bit to make it pop.
    TES_STMT(tes::create(
      tes_server, tes::MeshShape(tes::DtTriangles, tes::Id(),
                                 std::array<tes::Vector3f, 3>{ verts[0] * 1.01f, verts[1] * 1.01f,
                                                               verts[2] * 1.01f })
                    .setColour(tes::Colour::FireBrick)));

    // Calculate the new vertex at the centre of the existing triangle.
    new_vertices[0] = (0.5f * (verts[0] + verts[1])).normalised();
    new_vertices[1] = (0.5f * (verts[1] + verts[2])).normalised();
#if 1
    new_vertices[2] = (0.5f * (verts[2] + verts[0])).normalised();
#else   // #
        // Introduce a bug in the tessellation
    new_vertices[2] = (1.0f * (verts[2] + verts[2])).normalised();
#endif  // #

    // Create new triangles.
    // Given triangle ABC, and adding vertices DEF such that:
    //  D = AB/2  E = BC/2  F = CA/2
    // We have four new triangles:
    //  ADF, BED, CFE, DEF
    // ABC are in order in 'abc', while DEF will be in 'def'.
    // FIXME: find existing point to use.
    def[0] = insertVertex(new_vertices[0], vertices, vertex_map);
    def[1] = insertVertex(new_vertices[1], vertices, vertex_map);
    def[2] = insertVertex(new_vertices[2], vertices, vertex_map);

    TES_STMT(tes::create(
      tes_server, tes::MeshShape(tes::DtTriangles, tes::Id(),
                                 std::array<tes::Vector3f, 3>{ vertices[def[0]], vertices[def[1]],
                                                               vertices[def[2]] })
                    .setColour(tes::Colour::Cyan)));
    TES_STMT(tes::create(
      tes_server, tes::MeshShape(tes::DtTriangles, tes::Id(),
                                 std::array<tes::Vector3f, 3>{ vertices[def[0]], vertices[def[1]],
                                                               vertices[def[2]] })
                    .setColour(tes::Colour::Navy)
                    .setWireframe(true)));

    // Replace the original triangle ABC with DEF
    indices[i * 3 + 0] = def[0];
    indices[i * 3 + 1] = def[1];
    indices[i * 3 + 2] = def[2];

    // Add triangles ADF, BED, CFE
    indices.push_back(abc[0]);
    indices.push_back(def[0]);
    indices.push_back(def[2]);

    TES_STMT(tes::create(
      tes_server, tes::MeshShape(tes::DtTriangles, tes::Id(),
                                 std::array<tes::Vector3f, 3>{ vertices[abc[0]], vertices[abc[1]],
                                                               vertices[abc[2]] })
                    .setColour(tes::Colour::Cyan)));
    TES_STMT(tes::create(
      tes_server, tes::MeshShape(tes::DtTriangles, tes::Id(),
                                 std::array<tes::Vector3f, 3>{ vertices[abc[0]], vertices[abc[1]],
                                                               vertices[abc[2]] })
                    .setColour(tes::Colour::Navy)
                    .setWireframe(true)));

    indices.push_back(abc[1]);
    indices.push_back(def[1]);
    indices.push_back(def[0]);

    TES_STMT(tes::create(
      tes_server, tes::MeshShape(tes::DtTriangles, tes::Id(),
                                 std::array<tes::Vector3f, 3>{ vertices[abc[1]], vertices[def[1]],
                                                               vertices[def[2]] })
                    .setColour(tes::Colour::Cyan)));
    TES_STMT(tes::create(
      tes_server, tes::MeshShape(tes::DtTriangles, tes::Id(),
                                 std::array<tes::Vector3f, 3>{ vertices[abc[1]], vertices[def[1]],
                                                               vertices[def[2]] })
                    .setColour(tes::Colour::Navy)
                    .setWireframe(true)));

    indices.push_back(abc[2]);
    indices.push_back(def[2]);
    indices.push_back(def[1]);

    TES_STMT(tes::create(
      tes_server, tes::MeshShape(tes::DtTriangles, tes::Id(),
                                 std::array<tes::Vector3f, 3>{ vertices[abc[2]], vertices[def[2]],
                                                               vertices[def[1]] })
                    .setColour(tes::Colour::Cyan)));
    TES_STMT(tes::create(
      tes_server, tes::MeshShape(tes::DtTriangles, tes::Id(),
                                 std::array<tes::Vector3f, 3>{ vertices[abc[2]], vertices[def[2]],
                                                               vertices[def[1]] })
                    .setColour(tes::Colour::Navy)
                    .setWireframe(true)));


    TES_STMT(tes::updateServer(tes_server));
  }
}
}  // namespace

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

  unsigned iterations = 5;
  if (argc > 1)
  {
    std::istringstream in(argv[argc - 1]);
    in >> iterations;
  }

  std::vector<Vector3f> vertices;
  std::vector<unsigned> indices;
  SphereVertexMap sphere_map;

  // Initialise settings: zero flags: no cache, compression or collation.
  unsigned server_flags = 0;
#ifdef TES_ENABLE
  server_flags = tes::SFNakedFrameMessage;
  if (haveOption("collate", argc, argv))
  {
    server_flags = tes::SFCollate;
  }
  if (tes::checkFeature(tes::TFeatureCompression) && haveOption("compress", argc, argv))
  {
    server_flags = tes::SFCompress | tes::SFCollate;
  }

  tes_server = tes::createServer(tes::ServerSettings{ server_flags }, tes::XYZ);
  tes::startServer(tes_server);
  tes::waitForConnection(tes_server, 1000u);
  std::cout << "Starting with " << tes_server->connectionCount() << " connection(s)." << std::endl;
#endif  // TES_ENABLE

  // Start building the sphere.
  std::cout << "Initialise sphere for " << iterations << " iterations." << std::endl;
  sphereInitialise(vertices, indices, &sphere_map);

  const tes::Vector3f text_pos(0.05f, 0.05f, 0);
  TES_STMT(tes::create(tes_server, tes::Text2D("Initial", 0u, text_pos)));
  TES_STMT(tes::updateServer(tes_server));

  // Start with one shape.
  TES_STMT(unsigned shape_count = 1);

  for (unsigned i = 0; i < iterations; ++i)
  {
    std::stringstream label;
    label << "Division " << i + 1;
    std::cout << label.str() << std::endl;
    subdivideUnitSphere(vertices, indices, sphere_map);
#ifdef TES_ENABLE
    for (unsigned i = 0; i < shape_count; ++i)
    {
      tes::destroy(tes_server, tes::MeshShape(tes::DtTriangles, tes::Id(SPHERE_ID) + i));
    }
    // Send the updated sphere. We must unroll into sets of triangles of less than 65K
    // vertices.
    shape_count = tesUnrollDisplay(vertices, indices);
    if (i)
    {
      tes::destroy(tes_server, tes::Text2D("", tes::Id(TEXT_ID)));
    }
    tes::create(tes_server, tes::Text2D(label.str(), TEXT_ID, text_pos));
    tes::updateServer(tes_server);
#endif  // TES_ENABLE
  }

  std::cout << "Done" << std::endl;

  // Stop and close the server.
  TES_STMT(tes::stopServer(tes_server));

  return 0;
}
