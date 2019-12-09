//
// author Kazys Stepanas
//
#include "occupancyloader.h"

#ifdef _MSC_VER
// std::equal with parameters that may be unsafe warning under Visual Studio.
#pragma warning(disable : 4996)
#endif  // _MSC_VER

#include <cstring>
#include <fstream>
#include <string>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif // __GNUC__

#define TINYPLY_IMPLEMENTATION
#include "3rd-party/tinyply.h"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif // __GNUC__

struct TrajectoryPoint
{
  double timestamp;
  tes::Vector3d position;
};

struct PlyReader
{
  tinyply::PlyFile plyFile;
  std::shared_ptr<tinyply::PlyData> xData;
  std::shared_ptr<tinyply::PlyData> yData;
  std::shared_ptr<tinyply::PlyData> zData;
  std::shared_ptr<tinyply::PlyData> tData;
  size_t nextPointIndex = 0;
  size_t pointCount = 0;

  void close()
  {
    xData.reset<tinyply::PlyData>(nullptr);
    yData.reset<tinyply::PlyData>(nullptr);
    zData.reset<tinyply::PlyData>(nullptr);
    tData.reset<tinyply::PlyData>(nullptr);
    pointCount = 0;
  }

  bool bindProperties()
  {
    xData = plyFile.request_properties_from_element("vertex", { "x" });
    yData = plyFile.request_properties_from_element("vertex", { "y" });
    zData = plyFile.request_properties_from_element("vertex", { "z" });

    const std::vector<std::string> timeNames = { "time", "timestamp", "scalar_GpsTime", "GpsTime" };
    for (const auto &name : timeNames)
    {
      try
      {
        tData = plyFile.request_properties_from_element("vertex", { name });
        if (validField(tData))
        {
          // Have time data.
          break;
        }
      }
      catch (...)
      {
        // Failed to resolve. Try the next one.
      }
    }

    pointCount = 0;
    if (!validField(xData) || !validField(yData) || !validField(zData) || !validField(tData))
    {
      return false;
    }

    if (xData->count != yData->count || yData->count != zData->count || zData->count != tData->count)
    {
      return false;
    }

    pointCount = xData->count;
    return true;
  }

  bool nextPoint(double &timestamp, tes::Vector3d &pt)
  {
    if (nextPointIndex >= pointCount)
    {
      return false;
    }

    timestamp = readAs<double>(tData->buffer.get(), tData->t, nextPointIndex);
    pt.x = readAs<double>(xData->buffer.get(), xData->t, nextPointIndex);
    pt.y = readAs<double>(yData->buffer.get(), yData->t, nextPointIndex);
    pt.z = readAs<double>(zData->buffer.get(), zData->t, nextPointIndex);
    ++nextPointIndex;
    return true;
  }

  static bool validField(const std::shared_ptr<tinyply::PlyData> &field) { return field && field->count; }

  template <class T>
  static T readAs(const void *const data, const tinyply::Type &t, const size_t index)
  {
    switch (t)
    {
    case (tinyply::Type::INT8):
      return T(((int8_t *)data)[index]);
    case (tinyply::Type::UINT8):
      return T(((uint8_t *)data)[index]);
    case (tinyply::Type::INT16):
      return T(((int16_t *)data)[index]);
    case (tinyply::Type::UINT16):
      return T(((uint16_t *)data)[index]);
    case (tinyply::Type::INT32):
      return T(((int32_t *)data)[index]);
    case (tinyply::Type::UINT32):
      return T(((uint32_t *)data)[index]);
    case (tinyply::Type::FLOAT32):
      return T(((float *)data)[index]);
    case (tinyply::Type::FLOAT64):
      return T(((double *)data)[index]);
    default:
      throw std::runtime_error("Point Cloud Error - Unhandled attribute type");
    }
  }
};

struct OccupancyLoaderDetail
{
  PlyReader sampleReader;
  PlyReader trajectoryReader;
  std::string sampleFilePath;
  std::string trajectoryFilePath;
  std::ifstream sampleFile;
  std::ifstream trajectoryFile;
  TrajectoryPoint trajectoryBuffer[2];

  inline OccupancyLoaderDetail() { memset(&trajectoryBuffer, 0, sizeof(trajectoryBuffer)); }
};

OccupancyLoader::OccupancyLoader()
  : _imp(new OccupancyLoaderDetail)
{}


OccupancyLoader::~OccupancyLoader()
{
  delete _imp;
}


bool OccupancyLoader::open(const char *sampleFilePath, const char *trajectoryFilePath)
{
  close();

  _imp->sampleFilePath = sampleFilePath;
  _imp->trajectoryFilePath = trajectoryFilePath;

  _imp->sampleFile.open(_imp->sampleFilePath);
  _imp->trajectoryFile.open(_imp->trajectoryFilePath);

  if (!sampleFileIsOpen() || !trajectoryFileIsOpen())
  {
    close();
    return false;
  }

  _imp->sampleReader.plyFile.parse_header(_imp->sampleFile);
  _imp->trajectoryReader.plyFile.parse_header(_imp->trajectoryFile);

  _imp->sampleReader.bindProperties();
  _imp->trajectoryReader.bindProperties();

  _imp->sampleReader.plyFile.read(_imp->sampleFile);
  _imp->trajectoryReader.plyFile.read(_imp->trajectoryFile);

  // Prime the trajectory buffer.
  bool trajectoryPrimed = true;
  for (int i = 0; i < 2; ++i)
  {
    if (!_imp->trajectoryReader.nextPoint(_imp->trajectoryBuffer[i].timestamp, _imp->trajectoryBuffer[i].position))
    {
      trajectoryPrimed = false;
    }
  }

  if (!trajectoryPrimed)
  {
    close();
    return false;
  }

  return true;
}


void OccupancyLoader::close()
{
  _imp->sampleReader.close();
  _imp->trajectoryReader.close();
  _imp->sampleFile.close();
  _imp->trajectoryFile.close();
  _imp->sampleFilePath.clear();
  _imp->trajectoryFilePath.clear();
}


bool OccupancyLoader::sampleFileIsOpen() const
{
  return _imp->sampleFile.is_open();
}


bool OccupancyLoader::trajectoryFileIsOpen() const
{
  return _imp->trajectoryFile.is_open();
}


bool OccupancyLoader::nextPoint(tes::Vector3f &sample, tes::Vector3f &origin, double *timestamp)
{
  tes::Vector3d sd, od;
  if (!nextPoint(sd, od, timestamp))
  {
    return false;
  }

  sample = sd;
  origin = od;
  return true;
}


bool OccupancyLoader::nextPoint(tes::Vector3d &sample, tes::Vector3d &origin, double *timestampOut)
{
  double timestamp = 0;
  if (_imp->sampleReader.nextPoint(timestamp, sample))
  {
    if (timestampOut)
    {
      *timestampOut = timestamp;
    }
    origin = tes::Vector3f(0.0f);
    sampleTrajectory(origin, timestamp);
    return true;
  }
  return false;
}


bool OccupancyLoader::sampleTrajectory(tes::Vector3d &position, double timestamp)
{
  if (_imp->trajectoryReader.pointCount)
  {
    double nextTimestamp;
    tes::Vector3d pt;
    while (timestamp > _imp->trajectoryBuffer[1].timestamp && _imp->trajectoryReader.nextPoint(nextTimestamp, pt))
    {
      _imp->trajectoryBuffer[0] = _imp->trajectoryBuffer[1];
      _imp->trajectoryBuffer[1].timestamp = nextTimestamp;
      _imp->trajectoryBuffer[1].position = pt;
    }

    if (_imp->trajectoryBuffer[0].timestamp <= timestamp && timestamp <= _imp->trajectoryBuffer[1].timestamp &&
        _imp->trajectoryBuffer[0].timestamp != _imp->trajectoryBuffer[1].timestamp)
    {
      double lerp = double((timestamp - _imp->trajectoryBuffer[0].timestamp) /
                           (_imp->trajectoryBuffer[1].timestamp - _imp->trajectoryBuffer[0].timestamp));
      position = _imp->trajectoryBuffer[0].position +
                 lerp * (_imp->trajectoryBuffer[1].position - _imp->trajectoryBuffer[0].position);
      return true;
    }
  }
  return false;
}
