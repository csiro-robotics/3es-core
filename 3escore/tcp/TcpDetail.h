
//
// author: Kazys Stepanas
//
#ifndef _3ESTCPDETAIL_H_
#define _3ESTCPDETAIL_H_

#include <3escore/CoreConfig.h>

#ifdef WIN32
#include <winsock2.h>
typedef int socklen_t;
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
  int socket;
  sockaddr_in address;

  TcpSocketDetail()
    : socket(-1)
  {
    memset(&address, 0, sizeof(address));
  }
};

struct TcpListenSocketDetail
{
  int listenSocket;
  struct sockaddr_in address;

  TcpListenSocketDetail()
    : listenSocket(-1)
  {
    memset(&address, 0, sizeof(address));
  }
};
}  // namespace tes

#endif  // _3ESTCPDETAIL_H_
