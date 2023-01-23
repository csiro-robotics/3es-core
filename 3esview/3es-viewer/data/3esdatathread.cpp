#include "3esdatathread.h"

#include "3esthirdeyescene.h"

#include <3eslog.h>
#include <3esmessages.h>
#include <3espacketreader.h>

namespace tes::viewer
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
}  // namespace tes::viewer
