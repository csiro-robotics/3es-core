//
// author: Kazys Stepanas
//
#include <3escore/TcpListenSocket.h>

#include <3escore/TcpSocket.h>

#include "TcpBase.h"

#include "TcpDetail.h"

#include <cstring>

namespace tes
{
namespace
{
constexpr int kTcpbaseMaxBacklog = 10;

bool acceptConnection(TcpListenSocketDetail &server, TcpSocketDetail &client)
{
  socklen_t address_len = sizeof(client.address);

  client.socket =
    static_cast<int>(::accept(server.listen_socket, reinterpret_cast<sockaddr *>(&client.address),
                              reinterpret_cast<socklen_t *>(&address_len)));

  if (client.socket <= 0)
  {
    return false;
  }

  // printf("remote connection on port %d (%d) created from port %d (%d)\n",
  //       tcpbase::getSocketPort(client.socket), client.socket,
  //       tcpbase::getSocketPort(server.listenSocket), server.listenSocket);

#ifdef __APPLE__
  // Don't throw a SIGPIPE signal
  int noSignal = 1;
  if (::setsockopt(client.socket, SOL_SOCKET, SO_NOSIGPIPE, &noSignal, sizeof(noSignal)) < 0)
  {
    tcpbase::close(client.socket);
    return false;
  }
#endif  // __APPLE__

#ifdef WIN32
  // Set non blocking.
  u_long i_mode = 1;
  ::ioctlsocket(client.socket, FIONBIO, &i_mode);
#endif  // WIN32

  // tcpbase::dumpSocketOptions(client.socket);
  return true;
}
}  // namespace

TcpListenSocket::TcpListenSocket()
  : _detail(new TcpListenSocketDetail)
{}


TcpListenSocket::~TcpListenSocket()
{
  close();
  delete _detail;
}


uint16_t TcpListenSocket::port() const
{
  return (isListening()) ? ntohs(_detail->address.sin_port) : 0;
}


bool TcpListenSocket::listen(unsigned short port)
{
  if (isListening())
  {
    return false;
  }

  _detail->listen_socket = tcpbase::create();
  if (_detail->listen_socket == -1)
  {
    return false;
  }

  _detail->address.sin_family = AF_INET;
  _detail->address.sin_addr.s_addr = htonl(INADDR_ANY);
  _detail->address.sin_port = htons(port);

  //  Give the socket a local address as the TCP server
  if (::bind(_detail->listen_socket, reinterpret_cast<struct sockaddr *>(&_detail->address),
             sizeof(_detail->address)) < 0)
  {
    close();
    return false;
  }

  if (::listen(_detail->listen_socket, kTcpbaseMaxBacklog) < 0)
  {
    close();
    return false;
  }

  // printf("Listening on port %d\n", tcpbase::getSocketPort(_detail->listenSocket));

  return true;
}


void TcpListenSocket::close()
{
  if (_detail->listen_socket != -1)
  {
    tcpbase::close(_detail->listen_socket);
    _detail->listen_socket = -1;
    memset(&_detail->address, 0, sizeof(_detail->address));
  }
}


bool TcpListenSocket::isListening() const
{
  return _detail->listen_socket != -1;
}


std::shared_ptr<TcpSocket> TcpListenSocket::accept(unsigned timeout_ms)
{
  struct timeval timeout;
  fd_set fd_read = {};

  if (_detail->listen_socket < 0)
  {
    return {};
  }

  // use select() to avoid blocking on accept()

  FD_ZERO(&fd_read);                         // Clear the set of selected objects
  FD_SET(_detail->listen_socket, &fd_read);  // Add socket to read set

  tcpbase::timevalFromMs(timeout, timeout_ms);

  if (::select(_detail->listen_socket + 1, &fd_read, nullptr, nullptr, &timeout) < 0)
  {
    return {};
  }

  // Test if the socket file descriptor (m_sockLocal) is part of the
  // set returned by select().  If not, then select() timed out.

  if (FD_ISSET(_detail->listen_socket, &fd_read) == 0)
  {
    return {};
  }

  // Accept a connection from a client
  auto client_detail = std::make_unique<TcpSocketDetail>();
  if (!tes::acceptConnection(*_detail, *client_detail))
  {
    return {};
  }

  return std::make_shared<TcpSocket>(std::move(client_detail));
}
}  // namespace tes
