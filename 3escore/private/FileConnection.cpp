//
// author: Kazys Stepanas
//
#include "FileConnection.h"

#include <3escore/StreamUtil.h>

#include <mutex>

namespace tes
{
FileConnection::FileConnection(const std::string &filename, const ServerSettings &settings)
  : BaseConnection(settings)
  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  , _out_file(filename, std::ios::binary | std::ios::out | std::ios::in | std::ios::trunc)
  , _filename(filename)
{}


// TODO(KS): What's the correct way to handle the potential for close() throwing an exception?
// We need to call close() in the destructor for completeness, but it does do work which could
// throw. However, not closing the stream will cause other resource issues. So which is more
// correct? Leaving the potential for an exception or leaving the potential for an incomplete
// stream?
// NOLINTNEXTLINE(bugprone-exception-escape)
FileConnection::~FileConnection()
{
  close();
}


void FileConnection::close()
{
  const std::lock_guard<Lock> guard(_file_lock);
  if (_out_file.is_open())
  {
    _out_file.flush();
    streamutil::finaliseStream(_out_file, _frame_count);
    _out_file.close();
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
  const std::lock_guard<Lock> guard(_file_lock);
  return _out_file.is_open();
}


bool FileConnection::sendServerInfo(const ServerInfoMessage &info)
{
  if (!BaseConnection::sendServerInfo(info))
  {
    return false;
  }

  // Server info already written. No need to write it again.
  return streamutil::initialiseStream(_out_file, nullptr);
}


int FileConnection::updateFrame(float dt, bool flush)
{
  ++_frame_count;
  return BaseConnection::updateFrame(dt, flush);
}


int FileConnection::writeBytes(const uint8_t *data, int byte_count)
{
  _out_file.write(reinterpret_cast<const char *>(data), byte_count);
  if (!_out_file.fail())
  {
    return byte_count;
  }

  return -1;
}
}  // namespace tes
