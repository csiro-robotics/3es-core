//
// author Kazys Stepanas
//
#include "Occupancy.h"

#include <3escore/Vector3.h>

#include <3escore/Colour.h>
#include <3escore/ServerApi.h>

#include "OccupancyLoader.h"
#include "OccupancyMesh.h"
#include "p2p.h"

#include <csignal>
#include <cstring>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <unordered_set>

// Forced bug ideas to show now 3es highlights the issue(s).
// 1. Skip inserting the sample voxel key assuming that the ray will do so.
// 2. call integrateMiss() instead of integrateHit().
// 3. no trajectory data.

using namespace tes;

tes::ServerPtr g_tes_server;

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


enum RayLevel
{
  Rays_Off,
  Rays_Lines = (1 << 0),
  Rays_Voxels = (1 << 1),
  Rays_All = Rays_Lines | Rays_Voxels
};


enum SampleLevel
{
  Samples_Off,
  Samples_Voxels = (1 << 0),
  Samples_Points = (1 << 1),
  Samples_All = Samples_Voxels | Samples_Points
};


struct Options
{
  std::string cloud_file;
  std::string trajectory_file;
  std::string out_stream;
  uint64_t point_limit = 0;
  double start_time = 0;
  double end_time = 0;
  float resolution = 0.1f;
  float prob_hit = 0.7f;
  float prob_miss = 0.49f;
  unsigned batch_size = 1000;
  int rays = Rays_Lines;
  int samples = Samples_Voxels;
  bool quiet = false;
};

using KeySet = std::unordered_set<octomap::OcTreeKey, octomap::OcTreeKey::KeyHash>;


bool matchArg(const char *arg, const char *expect)
{
  return strncmp(arg, expect, strlen(expect)) == 0;
}

bool optionValue(const char *arg, int argc, char *argv[], std::string &value)
{
  TES_UNUSED(argc);
  TES_UNUSED(argv);
  if (*arg == '=')
  {
    ++arg;
  }
  value = arg;
  return true;
}

template <typename NUMERIC>
bool optionValue(const char *arg, int argc, char *argv[], NUMERIC &value)
{
  std::string strValue;
  if (optionValue(arg, argc, argv, strValue))
  {
    std::istringstream instr(strValue);
    instr >> value;
    return !instr.fail();
  }

  return false;
}


void shiftToSet(UnorderedKeySet &dst, UnorderedKeySet &src, const octomap::OcTreeKey &key)
{
  auto iter = src.find(key);
  if (iter != src.end())
  {
    src.erase(iter);
  }
  dst.insert(key);
}

#ifdef TES_ENABLE
void renderVoxels(const UnorderedKeySet &keys, const octomap::OcTree &map,
                  const tes::Colour &colour, uint16_t category)
{
  // Convert to voxel centres.
  if (!keys.empty())
  {
    std::vector<Vector3f> centres(keys.size());
    size_t index = 0;
    for (auto key : keys)
    {
      centres[index++] = p2p(map.keyToCoord(key));
    }

    // Render slightly smaller than the actual voxel size.
    create(g_tes_server, MeshShape(tes::DtVoxels, tes::Id(0u, category), centres)
                           .setUniformNormal(0.95f * float(map.getResolution()))
                           .setColour(colour));
  }
}
#endif  // TES_ENABLE
}  // namespace


int populateMap(const Options &opt)
{
  printf("Loading points from %s with trajectory %s \n", opt.cloud_file.c_str(),
         opt.trajectory_file.c_str());

  OccupancyLoader loader;
  if (!loader.open(opt.cloud_file.c_str(), opt.trajectory_file.c_str()))
  {
    fprintf(stderr, "Error loading cloud %s with trajectory %s \n", opt.cloud_file.c_str(),
            opt.trajectory_file.c_str());
    return -2;
  }

  octomap::KeyRay ray_keys;
  octomap::OcTree map(opt.resolution);
  octomap::OcTreeKey key;
  tes::Vector3f origin;
  tes::Vector3f sample;
  tes::Vector3f voxel;
  tes::Vector3f ext(opt.resolution);
  double timestamp;
  uint64_t point_count = 0;
  size_t key_index = 0;
  // Update map visualisation every N samples.
  const size_t ray_batch_size = opt.batch_size;
  double timebase = -1;
  double first_batch_timestamp = -1;
  double last_timestamp = -1;
#ifdef TES_ENABLE
  char time_str_buffer[256];
  // Keys of voxels touched in the current batch.
  UnorderedKeySet become_occupied;
  UnorderedKeySet become_free;
  UnorderedKeySet touched_free;
  UnorderedKeySet touched_occupied;
  std::vector<Vector3f> rays;
  std::vector<Vector3f> samples;
  OccupancyMesh map_mesh(RES_MapMesh, map);
#endif  // TES_ENABLE

  map.setProbHit(opt.prob_hit);
  map.setProbMiss(opt.prob_miss);

  // Prevent ready saturation to free.
  map.setClampingThresMin(0.01);
  // printf("min: %g\n", map.getClampingThresMinLog());

  TES_STMT(
    create(g_tes_server, MeshSet(&map_mesh, Id(RES_Map, CAT_Map)).setColour(Colour::SteelBlue)));
  // Ensure mesh is created for later update.
  TES_STMT(updateServer(g_tes_server));

  // Load the first point.
  bool have_point = loader.nextPoint(sample, origin, &timestamp);
  if (!have_point)
  {
    printf("No data to load\n");
    return -1;
  }

  timebase = timestamp;

  if (opt.start_time > 0)
  {
    // Get to the start time.
    printf("Skipping to start time offset: %g\n", opt.start_time);
    while ((have_point = loader.nextPoint(sample, origin, &timestamp)))
    {
      if (timestamp - timebase >= opt.start_time)
      {
        break;
      }
    }
  }

  printf("Populating map\n");
  while (have_point)
  {
    ++point_count;
    TES_IF(opt.rays & Rays_Lines)
    {
      TES_STMT(rays.push_back(origin));
      TES_STMT(rays.push_back(sample));
    }
    TES_IF(opt.samples & Samples_Points)
    {
      TES_STMT(samples.push_back(sample));
    }

    if (first_batch_timestamp < 0)
    {
      first_batch_timestamp = timestamp;
    }
    // Compute free ray.
    map.computeRayKeys(p2p(origin), p2p(sample), ray_keys);
    // Draw intersected voxels.
    key_index = 0;
    for (auto key : ray_keys)
    {
      if (octomap::OcTree::NodeType *node = map.search(key))
      {
        // Existing node.
        const bool initiallyOccupied = map.isNodeOccupied(node);
        map.integrateMiss(node);
        if (initiallyOccupied && !map.isNodeOccupied(node))
        {
          // Node became free.
          TES_STMT(shiftToSet(become_free, become_occupied, key));
        }
      }
      else
      {
        // New node.
        map.updateNode(key, false, true);
      }
      voxel = p2p(map.keyToCoord(key));
      // Collate for render.
      TES_STMT(touched_free.insert(key));
      ++key_index;
    }

    // Update the sample node.
    key = map.coordToKey(p2p(sample));
    if (octomap::OcTree::NodeType *node = map.search(key))
    {
      // Existing node.
      const bool initiallyOccupied = map.isNodeOccupied(node);
      map.integrateHit(node);
      if (!initiallyOccupied && map.isNodeOccupied(node))
      {
        // Node became occupied.
        TES_STMT(shiftToSet(become_occupied, become_free, key));
      }
    }
    else
    {
      // New node.
      map.updateNode(key, true, true);
      // Collate for render.
      TES_STMT(shiftToSet(become_occupied, become_free, key));
    }
    TES_STMT(shiftToSet(touched_occupied, touched_free, key));

    if (point_count % ray_batch_size == 0 || quit)
    {
      //// Collapse the map.
      // map.isNodeCollapsible()
#ifdef TES_ENABLE
      double elapsed_time =
        (last_timestamp >= 0) ? timestamp - last_timestamp : timestamp - first_batch_timestamp;
      // Handle time jumps back.
      elapsed_time = std::max(elapsed_time, 0.0);
      // Cull large time differences.
      elapsed_time = std::min(elapsed_time, 1.0);
      first_batch_timestamp = -1;

#ifdef _MSC_VER
      sprintf_s(time_str_buffer, "%g", timestamp - timebase);
#else   // _MSC_VER
      sprintf(time_str_buffer, "%g", timestamp - timebase);
#endif  // _MSC_VER

      create(g_tes_server, Text2D(time_str_buffer, Id(0u, CAT_Info), Vector3f(0.05f, 0.1f, 0.0f)));
      // Draw sample lines.
      if (opt.rays & Rays_Lines)
      {
        create(g_tes_server,
               MeshShape(DtLines, Id(0u, CAT_Rays), rays).setColour(Colour::DarkOrange));
      }
      rays.clear();
      // Render touched voxels in bulk.
      if (opt.rays & Rays_Voxels)
      {
        renderVoxels(touched_free, map, tes::Colour(tes::Colour::MediumSpringGreen), CAT_FreeCells);
      }
      if (opt.samples & Samples_Voxels)
      {
        renderVoxels(touched_occupied, map, tes::Colour(tes::Colour::Turquoise), CAT_OccupiedCells);
      }
      if (opt.samples)
      {
        create(g_tes_server,
               MeshShape(DtPoints, Id(0u, CAT_OccupiedCells), samples).setColour(Colour::Orange));
      }
      samples.clear();
      // updateServer(g_tes_server);

      // Ensure touched_occupied does not contain newly occupied nodes for mesh update.
      for (auto key : become_occupied)
      {
        auto search = touched_occupied.find(key);
        if (search != touched_occupied.end())
        {
          touched_occupied.erase(search);
        }
      }

      // Render changes to the map.
      map_mesh.update(become_occupied, become_free, touched_occupied);

      touched_free.clear();
      touched_occupied.clear();
      become_occupied.clear();
      become_free.clear();
      updateServer(g_tes_server, static_cast<float>(elapsed_time));
      if (opt.point_limit && point_count >= opt.point_limit ||
          opt.end_time > 0 && last_timestamp - timebase >= opt.end_time || quit)
      {
        break;
      }
#endif  // TES_ENABLE

      last_timestamp = timestamp;
      if (!opt.quiet)
      {
        printf("\r%g        ", last_timestamp - timebase);
        // fflush(stdout);
      }
    }

    have_point = loader.nextPoint(sample, origin, &timestamp);
  }

  TES_STMT(updateServer(g_tes_server));

  if (!opt.quiet)
  {
    printf("\n");
  }

  printf("Processed %" PRIu64 " points.\n", point_count);

  // Save the occupancy map.
  printf("Saving map");
  map.writeBinary("map.bt");

  return 0;
}


void usage(const Options &opt)
{
  printf("Usage:\n");
  printf("3esOccupancy [options] <cloud.ply> <trajectory.ply>\n");
  printf("\nGenerates an Octomap occupancy map from a PLY based point cloud and accompanying "
         "trajectory file.\n\n");
  printf("The trajectory marks the scanner trajectory with timestamps loosely corresponding to "
         "cloud point timestamps. ");
  printf("Trajectory points are interpolated for each cloud point based on corresponding times in "
         "the trajectory.\n\n");
  printf("Third Eye Scene render commands are interspersed throughout the code to visualise the "
         "generation process\n\n");
  printf("Options:\n");
  printf("-b=<batch-size> (%u)\n", opt.batch_size);
  printf("  The number of points to process in each batch. Controls debug display.\n");
  printf("-h=<hit-probability> (%g)\n", opt.prob_hit);
  printf("  The occupancy probability due to a hit. Must be >= 0.5.\n");
  printf("-m=<miss-probability> (%g)\n", opt.prob_miss);
  printf("  The occupancy probability due to a miss. Must be < 0.5.\n");
  printf("-o=<stream-file>\n");
  printf("  Specifies a file to write a 3es stream to directly without the need for an external "
         "client.\n");
  printf("-p=<point-limit> (0)\n");
  printf("  The voxel resolution of the generated map.\n");
  printf("-q\n");
  printf("  Run in quiet mode. Suppresses progress messages.\n");
  printf("-r=<resolution> (%g)\n", opt.resolution);
  printf("  The voxel resolution of the generated map.\n");
  printf("-s=<time> (%g)\n", opt.start_time);
  printf("  Specifies a time offset for the start time. Ignore points until the time offset from "
         "the first point "
         "exceeds this value.\n");
  printf("-e=<time> (%g)\n", opt.end_time);
  printf("  Specifies an end time relative to the first point. Stop after processing time interval "
         "of points.\n");
  printf("--rays=[off,lines,voxels,all] (lines)\n");
  printf("  Enable or turn off visualisation of sample rays.\n");
  printf("    off: disable. Lowest throughput\n");
  printf("    lines: visualise line samples. Lower throughput\n");
  printf("    voxels: visualise intersected voxels. High throughput\n");
  printf("    all: visualise all previous options. Very high throughput\n");
  printf("--samples=[off,voxel,points,all] (voxels)\n");
  printf("  Enable visualisation of sample voxels in each batch (occupied).\n");
  printf("    off: disable. Lowest throughput\n");
  printf("    voxels : visualise intersected voxels. Lower throughput\n");
  printf("    points: visualise sample points. High throughput\n");
  printf("    all: visualise all previous options. Very high throughput\n");
}

void initialiseDebugCategories(const Options &opt)
{
#ifdef TES_ENABLE
  defineCategory(g_tes_server, "Map", CAT_Map, 0, true);
  defineCategory(g_tes_server, "Populate", CAT_Populate, 0, true);
  if (opt.rays & Rays_Lines)
  {
    defineCategory(g_tes_server, "Rays", CAT_Rays, CAT_Populate, (opt.rays & Rays_Lines) != 0);
  }
  if (opt.rays & Rays_Voxels)
  {
    defineCategory(g_tes_server, "Free", CAT_FreeCells, CAT_Populate, (opt.rays & Rays_Lines) == 0);
  }
  if (opt.samples)
  {
    defineCategory(g_tes_server, "Occupied", CAT_OccupiedCells, CAT_Populate, true);
  }
  defineCategory(g_tes_server, "Info", CAT_Info, 0, true);
#else   // TES_ENABLE
  (void)opt;
#endif  // TES_ENABLE
}

int main(int argc, char *argv[])
{
  Options opt;

  signal(SIGINT, onSignal);

  if (argc < 3)
  {
    usage(opt);
    return 0;
  }

  std::string str;
  for (int i = 1; i < argc; ++i)
  {
    if (argv[i][0] == '-')
    {
      bool ok = true;
      switch (argv[i][1])
      {
      case 'b':  // batch size
        ok = optionValue(argv[i] + 2, argc, argv, opt.batch_size);
        break;
      case 'e':  // start time
        ok = optionValue(argv[i] + 2, argc, argv, opt.end_time);
        break;
      case 'h':
        ok = optionValue(argv[i] + 2, argc, argv, opt.prob_hit);
        break;
      case 'm':
        ok = optionValue(argv[i] + 2, argc, argv, opt.prob_miss);
        break;
      case 'o':
        ok = optionValue(argv[i] + 2, argc, argv, opt.out_stream);
        break;
      case 'p':  // point limit
        ok = optionValue(argv[i] + 2, argc, argv, opt.point_limit);
        break;
      case 'q':  // quiet
        opt.quiet = true;
        break;
      case 'r':  // resolution
        ok = optionValue(argv[i] + 2, argc, argv, opt.resolution);
        break;
      case 's':  // start time
        ok = optionValue(argv[i] + 2, argc, argv, opt.start_time);
        break;
      case '-':  // Long option name.
      {
        if (matchArg(&argv[i][2], "rays"))
        {
          ok = optionValue(argv[i] + 6, argc, argv, str);
          if (ok)
          {
            if (str.compare("off") == 0)
            {
              opt.rays = Rays_Off;
            }
            else if (str.compare("lines") == 0)
            {
              opt.rays = Rays_Lines;
            }
            else if (str.compare("voxels") == 0)
            {
              opt.rays = Rays_Voxels;
            }
            else if (str.compare("all") == 0)
            {
              opt.rays = Rays_All;
            }
            else
            {
              ok = false;
            }
          }
        }
        else if (matchArg(&argv[i][2], "samples"))
        {
          ok = optionValue(argv[i] + 9, argc, argv, str);
          if (ok)
          {
            if (str.compare("off") == 0)
            {
              opt.samples = Samples_Off;
            }
            else if (str.compare("voxels") == 0)
            {
              opt.samples = Samples_Voxels;
            }
            else if (str.compare("points") == 0)
            {
              opt.samples = Samples_Points;
            }
            else if (str.compare("all") == 0)
            {
              opt.samples = Samples_All;
            }
            else
            {
              ok = false;
            }
          }
        }
        break;
      }
      }

      if (!ok)
      {
        fprintf(stderr, "Failed to read %s option value.\n", argv[i]);
      }
    }
    else if (opt.cloud_file.empty())
    {
      opt.cloud_file = argv[i];
    }
    else if (opt.trajectory_file.empty())
    {
      opt.trajectory_file = argv[i];
    }
  }

  if (opt.cloud_file.empty())
  {
    fprintf(stderr, "Missing input cloud (-i)\n");
    return -1;
  }
  if (opt.trajectory_file.empty())
  {
    fprintf(stderr, "Missing trajectory file (-t)\n");
    return -1;
  }

#ifdef TES_ENABLE
  // Initialise TES
  g_tes_server = createServer(ServerSettings(SFDefault), XYZ);

  // Start the server and wait for the connection monitor to start.
  startServer(g_tes_server, ConnectionMode::Asynchronous);
  waitForConnection(g_tes_server, 1000u);

  if (!opt.out_stream.empty())
  {
    g_tes_server->connectionMonitor()->openFileStream(opt.out_stream.c_str());
    g_tes_server->connectionMonitor()->commitConnections();
  }

  std::cout << "Starting with " << g_tes_server->connectionCount() << " connection(s)."
            << std::endl;
#endif  // TES_ENABLE

  initialiseDebugCategories(opt);

  int res = populateMap(opt);
  TES_STMT(stopServer(g_tes_server));
  return res;
}
