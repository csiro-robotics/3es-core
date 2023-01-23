#include "DataThread.h"

#include <3esview/ThirdEyeScene.h>

#include <3escore/Log.h>
#include <3escore/Messages.h>
#include <3escore/PacketReader.h>

namespace tes::view
{
DataThread::~DataThread() = default;


bool DataThread::processServerInfo(PacketReader &reader, ServerInfoMessage &server_info)
{
  ServerInfoMessage msg;
  if (msg.read(reader))
  {
    server_info = msg;
    return true;
  }

  log::error("Failed to decode server info.");
  return false;
}
}  // namespace tes::view
