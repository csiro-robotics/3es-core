
//
// author: Kazys Stepanas
//
#ifndef TES_CORE_TCP_TCP_DETAIL_H
#define TES_CORE_TCP_TCP_DETAIL_H

#include <3escore/CoreConfig.h>

#ifdef WIN32
#include <winsock2.h>
using socklen_t = int;  // NOLINT(readability-identifier-naming)
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#endif

#include <cstring>

namespace tes
{
struct TcpSocketDetail
{
  int socket = -1;
  sockaddr_in address = {};
};

struct TcpListenSocketDetail
{
  int listen_socket = -1;
  struct sockaddr_in address = {};
};
}  // namespace tes

#endif  // TES_CORE_TCP_TCP_DETAIL_H
