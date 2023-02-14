#ifndef _3ES_OCCUPANCY_H_
#define _3ES_OCCUPANCY_H_

#include <3escore/CoreConfig.h>

#include <memory>

#define TES_ENABLE
namespace tes
{
class Server;
}
extern std::shared_ptr<tes::Server> g_tes_server;

#include "debugids.h"

#endif  // _3ES_OCCUPANCY_H_
