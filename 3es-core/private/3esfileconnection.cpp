//
// author: Kazys Stepanas
//
#include "3esfileconnection.h"

#include "3esstreamutil.h"

#include <mutex>

using namespace tes;

FileConnection::FileConnection(const char *filename, const ServerSettings &settings)
  : BaseConnection(settings)
  , _outFile(filename, std::ios::binary | std::ios::out | std::ios::in | std::ios::trunc)
  , _filename(filename)
{}


FileConnection::~FileConnection()
{
  close();
}


void FileConnection::close()
{
  std::lock_guard<Lock> guard(_fileLock);
  if (_outFile.is_open())
  {
    _outFile.flush();
    streamutil::finaliseStream(_outFile, _frameCount);
    _outFile.close();
  }
}


const char *FileConnection::filename() const
{
  return _filename.c_str();
}


const char *FileConnection::address() const
{
  return filename();
}


uint16_t FileConnection::port() const
{
  return 0;
}


bool FileConnection::isConnected() const
{
  std::lock_guard<Lock> guard(_fileLock);
  return _outFile.is_open();
}


bool FileConnection::sendServerInfo(const ServerInfoMessage &info)
{
  if (!BaseConnection::sendServerInfo(info))
  {
    return false;
  }

  // Server info already written. No need to write it again.
  return streamutil::initialiseStream(_outFile, nullptr);
}


int FileConnection::updateFrame(float dt, bool flush)
{
  ++_frameCount;
  return BaseConnection::updateFrame(dt, flush);
}


int FileConnection::writeBytes(const uint8_t *data, int byteCount)
{
  _outFile.write(reinterpret_cast<const char *>(data), byteCount);
  if (!_outFile.fail())
  {
    return byteCount;
  }

  return -1;
}
