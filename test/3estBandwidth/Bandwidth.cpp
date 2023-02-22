//
// author: Kazys Stepanas
//
#include <3escore/Connection.h>
#include <3escore/ConnectionMonitor.h>
#include <3escore/CoordinateFrame.h>
#include <3escore/Feature.h>
#include <3escore/Server.h>
#include <3escore/shapes/PointCloud.h>
#include <3escore/shapes/Shapes.h>
#include <3escore/shapes/SimpleMesh.h>
#include <3escore/tessellate/Sphere.h>
#include <3escore/Timer.h>
#include <3escore/Vector3.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

// Bandwidth test. Tessellate a sphere to a high number of polygons and repeatedly send this data to
// the server.

using namespace tes;

namespace
{
using TimingClock = std::chrono::high_resolution_clock;

bool quit = false;

void onSignal(int arg)
{
  if (arg == SIGINT || arg == SIGTERM)
  {
    quit = true;
  }
}

TimingClock::duration findMinDuration(const TimingClock::duration *durations,
                                      unsigned durationCount)
{
  TimingClock::duration minDuration = durations[0];
  for (unsigned i = 1; i < durationCount; ++i)
  {
    if (durations[i] < minDuration)
    {
      minDuration = durations[i];
    }
  }

  return minDuration;
}

TimingClock::duration findMaxDuration(const TimingClock::duration *durations,
                                      unsigned durationCount)
{
  TimingClock::duration maxDuration = durations[0];
  for (unsigned i = 1; i < durationCount; ++i)
  {
    if (durations[i] > maxDuration)
    {
      maxDuration = durations[i];
    }
  }

  return maxDuration;
}

TimingClock::duration calcAvgDuration(const TimingClock::duration *durations,
                                      unsigned durationCount)
{
  TimingClock::duration totalDuration = durations[0];
  for (unsigned i = 1; i < durationCount; ++i)
  {
    totalDuration += durations[i];
  }

  return totalDuration / durationCount;
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


void showUsage(int argc, char **argv)
{
  TES_UNUSED(argc);
  std::cout << "Usage:\n";
  std::cout << argv[0] << " [options]\n";
  std::cout << "\nValid options:\n";
  std::cout << "  help: show this message\n";
  if (tes::checkFeature(tes::TFeatureCompression))
  {
    std::cout << "  compress: write collated and compressed packets\n";
  }
  std::cout.flush();
}


template <typename U, typename D>
inline std::ostream &operator<<(std::ostream &out, const std::chrono::duration<U, D> &duration)
{
  const bool negative = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() < 0;
  const char *sign = (!negative) ? "" : "-";
  std::chrono::duration<U, D> abs_duration = (!negative) ? duration : duration * -1;
  auto s = std::chrono::duration_cast<std::chrono::seconds>(abs_duration).count();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(abs_duration).count();
  ms = ms % 1000;

  if (s)
  {
    out << sign << s << "." << std::setw(3) << std::setfill('0') << ms << "s";
  }
  else
  {
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(abs_duration).count();
    us = us % 1000;

    if (ms)
    {
      out << sign << ms << "." << std::setw(3) << std::setfill('0') << us << "ms";
    }
    else
    {
      auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(abs_duration).count();
      ns = ns % 1000;

      if (us)
      {
        out << sign << us << "." << std::setw(3) << std::setfill('0') << ns << "us";
      }
      else
      {
        out << sign << ns << "ns";
      }
    }
  }
  return out;
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

  const unsigned targetPolyCount = 10000;
  std::vector<Vector3f> vertices;
  std::vector<Vector3f> triangles;
  std::vector<unsigned> indices;
  tes::sphere::SphereVertexMap sphereMap;

  std::cout << "Tessellating to at least " << targetPolyCount << " polygons." << std::endl;

  tes::sphere::initialise(vertices, indices, &sphereMap);
  while (indices.size() / 3 < targetPolyCount)
  {
    tes::sphere::subdivide(vertices, indices, sphereMap);
  }

  std::cout << "Created " << indices.size() / 3 << " triangles." << std::endl;

  // Unwrap the "mesh" to use contiguous indexing. This will duplicate vertices.
  std::cout << "Unrolling indexing." << std::endl;

  triangles.reserve(indices.size());
  for (unsigned vindex : indices)
  {
    triangles.push_back(vertices[vindex]);
  }
  vertices.clear();
  indices.clear();

  std::cout << "Starting server and sending triangle data." << std::endl;


  ServerInfoMessage info;
  initDefaultServerInfo(&info);
  info.coordinate_frame = XYZ;
  unsigned serverFlags = SFDefaultNoCompression;
  if (haveOption("compress", argc, argv))
  {
    serverFlags |= SFCompress | SFCollate;
  }

  auto server = Server::create(ServerSettings(serverFlags), &info);

  server->connectionMonitor()->start(tes::ConnectionMode::Asynchronous);

  const unsigned durationHistorySize = 100;
  TimingClock::duration durationWindow[durationHistorySize];
  unsigned nextDurationIndex = 0;

  for (unsigned i = 0; i < durationHistorySize; ++i)
  {
    durationWindow[i] = TimingClock::duration(0);
  }

  while (!quit)
  {
    const auto sendStart = TimingClock::now();

    // Send triangle data in chunks.
    MeshShape shape(DtTriangles, tes::Id(), tes::DataBuffer(triangles));  // Transient triangles.
    server->create(shape);

    server->updateFrame(0.0f);
    if (server->connectionMonitor()->mode() == tes::ConnectionMode::Synchronous)
    {
      server->connectionMonitor()->monitorConnections();
    }
    server->connectionMonitor()->commitConnections();
    server->updateTransfers(0);

    const auto sendEnd = TimingClock::now();
    const auto elapsed = sendEnd - sendStart;

    durationWindow[nextDurationIndex] = elapsed;
    nextDurationIndex = (nextDurationIndex + 1) % durationHistorySize;

    const auto minDuration = findMinDuration(durationWindow, durationHistorySize);
    const auto maxDuration = findMaxDuration(durationWindow, durationHistorySize);
    const auto avgDuration = calcAvgDuration(durationWindow, durationHistorySize);

    std::ostringstream timeStream;
    timeStream << elapsed << " avg: " << avgDuration << " [" << minDuration << "," << maxDuration
               << ']';

    printf("\r%u connection(s) %u triangles : %s      ", server->connectionCount(),
           unsigned(triangles.size()), timeStream.str().c_str());
    fflush(stdout);
  }

  server->updateFrame(0.0f, false);
  server->close();

  server->connectionMonitor()->stop();
  server->connectionMonitor()->join();

  server.reset();

  return 0;
}
