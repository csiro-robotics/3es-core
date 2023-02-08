#include "TcpBase.h"

#include <cstdio>
#include <cstring>

#ifdef WIN32
#include <winsock2.h>
#endif  // WIN32

#include <cerrno>  /* Error number definitions */
#include <fcntl.h> /* File control definitions */

#ifdef __linux__
#include <linux/serial.h> /* serial_struct */
#include <linux/tty.h>    /* tty_struct -> alt_speed */
#endif                    // __linux__

#ifndef WIN32
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h> /* bzero, etc */
#include <termios.h>   /* POSIX terminal control definitions */
#include <unistd.h>

#include <arpa/inet.h>  /* inet_ntoa */
#include <netdb.h>      /* hostent, servent */
#include <netinet/in.h> /* sockaddr_in */
#include <netinet/tcp.h>
#include <sys/ioctl.h> /* ioctl */
#include <sys/stat.h>
#endif  // !WIN32

namespace tes::tcpbase
{
#ifdef WIN32
using IntVal = DWORD;
using socklen_t = int;  // NOLINT(readability-identifier-naming)
#else                   // WIN32
using IntVal = int;
#endif                  // WIN32

int create()
{
  int sock = -1;
#ifdef WIN32
  // Initialise the WinSock2 stack
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 1), &wsa) != 0)
  {
    return false;
  }

  DWORD opt_val = 0;
#else
  int opt_val = 0;
#endif

  // Create a socket for stream communications
  sock = static_cast<int>(::socket(AF_INET, SOCK_STREAM, 0));

  if (sock < 0)
  {
    return -1;
  }

#ifdef __APPLE__
  // Don't throw a SIGPIPE signal
  opt_val = 1;
  if (::setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &opt_val, sizeof(opt_val)) < 0)
  {
    tcpbase::close(sock);
    return -1;
  }
#endif  // __APPLE__

  opt_val = 1;
  if (::setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char *>(&opt_val),
                   sizeof(opt_val)) < 0)
  {
    tcpbase::close(sock);
    return -1;
  }

  //// Disable lingering on socket close
  // struct linger ling;
  // ling.l_onoff = 0;
  // ling.l_linger = 0;
  // if (::setsockopt(sock, SOL_SOCKET, SO_LINGER, reinterpret_cast<char *>(&ling), sizeof(ling)) <
  // 0)
  //{
  //  tcpbase::close(sock);
  //  return -1;
  //}

  // Enable socket re-use (un-bind)
  int reuse = 1;
  if (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&reuse),
                   sizeof(reuse)) < 0)
  {
    tcpbase::close(sock);
    return -1;
  }

  // dumpSocketOptions(sock);
  return sock;
}


void close(int socket)
{
  if (socket != -1)
  {
#ifdef WIN32
    ::shutdown(socket, SD_SEND);
    ::closesocket(socket);
#else
    ::shutdown(socket, SHUT_RDWR);
    ::close(socket);
#endif
  }
}


bool setReceiveTimeout(int socket, unsigned timeout_ms)
{
#ifdef WIN32
  DWORD tv = timeout_ms;  // milliseconds
  return ::setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&tv),
                      sizeof(tv)) >= 0;
#else  // Linux

  struct timeval tv;
  timevalFromMs(tv, timeout_ms);
  return ::setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char *>(&tv), sizeof(tv)) >=
         0;
#endif
}


unsigned getReceiveTimeout(int socket)
{
#ifdef WIN32
  DWORD tv = 0;  // milliseconds
  socklen_t len = sizeof(tv);
  if (::getsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char *>(&tv), &len) < 0)
  {
    return false;
  }

  return static_cast<unsigned>(tv);
#else  // Linux

  struct timeval tv;
  socklen_t len;
  if (::getsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char *>(&tv), &len) < 0)
  {
    return false;
  }

  return static_cast<unsigned>(1000 * tv.tv_sec + tv.tv_usec / 1000);
#endif
}


bool setSendTimeout(int socket, unsigned timeout_ms)
{
#ifdef WIN32
  DWORD tv = timeout_ms;  // milliseconds
  return ::setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char *>(&tv),
                      sizeof(tv)) >= 0;
#else  // Linux

  struct timeval tv;
  timevalFromMs(tv, timeout_ms);
  return ::setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char *>(&tv), sizeof(tv)) >=
         0;
#endif
}


unsigned getSendTimeout(int socket)
{
#ifdef WIN32
  DWORD tv = 0;  // milliseconds
  socklen_t len;
  if (::getsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char *>(&tv), &len) < 0)
  {
    return false;
  }

  return static_cast<unsigned>(tv);
#else  // Linux

  struct timeval tv;
  socklen_t len;
  if (::getsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char *>(&tv), &len) < 0)
  {
    return false;
  }

  return static_cast<unsigned>(1000 * tv.tv_sec + tv.tv_usec / 1000);
#endif
}


void enableBlocking(int socket)
{
  (void)socket;
#ifndef WIN32
  // Disable blocking on read.
  int socketFlags = fcntl(socket, F_GETFL) & ~O_NONBLOCK;
  fcntl(socket, F_SETFL, socketFlags);
#endif  // WIN32
}

void disableBlocking(int socket)
{
  (void)socket;
#ifndef WIN32
  // Disable blocking on read.
  int socketFlags = fcntl(socket, F_GETFL) | O_NONBLOCK;
  fcntl(socket, F_SETFL, socketFlags);
#endif  // WIN32
}


void timevalFromMs(timeval &tv, unsigned milliseconds)
{
  // Split into seconds an micro seconds.
  tv.tv_sec = static_cast<decltype(tv.tv_sec)>(milliseconds) * 1000;
  const auto to_microseconds = 1'000'000;
  tv.tv_usec = tv.tv_sec % to_microseconds;  // Convert to microseconds
  tv.tv_sec /= to_microseconds;
}


void dumpSocOpt(int socket, const char *name, int opt)
{
  int opt_val = 0;
  socklen_t len = sizeof(opt_val);

  ::getsockopt(socket, SOL_SOCKET, opt, reinterpret_cast<char *>(&opt_val), &len);
  printf("%s %d\n", name, opt_val);
}


void dumpSocketOptions(int socket)
{
  struct DebugInfo
  {
    const char *name;
    int opt;
  };
  // Array size varies from platform, so no std::array.
  static const DebugInfo dopt[] =  // NOLINT(modernize-avoid-c-arrays)
    {
      { "SO_DEBUG", SO_DEBUG },
      { "SO_ACCEPTCONN", SO_ACCEPTCONN },
      { "SO_REUSEADDR", SO_REUSEADDR },
      { "SO_KEEPALIVE", SO_KEEPALIVE },
      { "SO_DONTROUTE", SO_DONTROUTE },
      { "SO_BROADCAST", SO_BROADCAST },
      { "SO_OOBINLINE", SO_OOBINLINE },
#ifndef WIN32
      { "SO_REUSEPORT", SO_REUSEPORT },
      { "SO_TIMESTAMP", SO_TIMESTAMP },
#endif  // WIN32
      { "SO_SNDBUF", SO_SNDBUF },
      { "SO_RCVBUF", SO_RCVBUF },
      { "SO_SNDLOWAT", SO_SNDLOWAT },
      { "SO_RCVLOWAT", SO_RCVLOWAT },
      { "SO_SNDTIMEO", SO_SNDTIMEO },
      { "SO_RCVTIMEO", SO_RCVTIMEO },
      { "SO_ERROR", SO_ERROR },
      { "SO_TYPE", SO_TYPE },
#ifdef __APPLE__
      { "SO_WANTOOBFLAG", SO_WANTOOBFLAG },
      { "SO_WANTMORE", SO_WANTMORE },
      { "SO_DONTTRUNC", SO_DONTTRUNC },
      { "SO_USELOOPBACK", SO_USELOOPBACK },
      { "SO_TIMESTAMP_MONOTONIC", SO_TIMESTAMP_MONOTONIC },
      { "SO_LABEL", SO_LABEL },
      { "SO_PEERLABEL", SO_PEERLABEL },
      { "SO_NREAD", SO_NREAD },
      { "SO_NKE", SO_NKE },
      { "SO_NOSIGPIPE", SO_NOSIGPIPE },
      { "SO_NOADDRERR", SO_NOADDRERR },
      { "SO_NWRITE", SO_NWRITE },
      { "SO_REUSESHAREUID", SO_REUSESHAREUID },
      { "SO_NOTIFYCONFLICT", SO_NOTIFYCONFLICT },
      { "SO_UPCALLCLOSEWAIT", SO_UPCALLCLOSEWAIT },
      { "SO_LINGER_SEC", SO_LINGER_SEC },
      { "SO_RANDOMPORT", SO_RANDOMPORT },
      { "SO_NP_EXTENSIONS", SO_NP_EXTENSIONS },
#endif  // __APPLE__
    };
  static const size_t dopt_size = sizeof(dopt) / sizeof(dopt[0]);

  for (const auto &dopt_item : dopt)
  {
    dumpSocOpt(socket, dopt_item.name, dopt_item.opt);
  }

  struct linger ling;
  ling.l_onoff = 0;
  ling.l_linger = 0;
  socklen_t len = sizeof(ling);
  getsockopt(socket, SOL_SOCKET, SO_LINGER, reinterpret_cast<char *>(&ling), &len);
  printf("SO_LINGER %d:%d\n", ling.l_onoff, ling.l_linger);
}


uint16_t getSocketPort(int socket)
{
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  if (getsockname(socket, reinterpret_cast<struct sockaddr *>(&sin), &len) == -1)
  {
    return 0;
  }

  return ntohs(sin.sin_port);
}


const char *sockErrStr(int err)
{
#ifdef WIN32
  switch (err)
  {
  case 0:
    return "";
  case WSANOTINITIALISED:
    return "socket system not initialised";
  case WSAENETDOWN:
    return "net subsystem down";
  case WSAENOTCONN:
    return "not connected";
  case WSAEACCES:
    return "broadcast access";
  case WSAEINTR:
    return "cancelled";
  case WSAEINPROGRESS:
    return "in progress";
  case WSAEFAULT:
    return "invalid buffer";
  case WSAENETRESET:
    return "keep alive failed";
  case WSAENOBUFS:
    return "no buffer space";
  case WSAENOTSOCK:
    return "invalid descriptor";
  case WSAEOPNOTSUPP:
    return "not supported";
  case WSAESHUTDOWN:
    return "shutdown";
  case WSAEWOULDBLOCK:
    return "would block";
  case WSAEMSGSIZE:
    return "truncated";
  case WSAEINVAL:
    return "unbound";
  case WSAECONNABORTED:
    return "aborted";
  case WSAECONNRESET:
    return "connection reset";
  case WSAETIMEDOUT:
    return "timedout";
  default:
    break;
  }

#else   // WIN32
  switch (err)
  {
  case 0:
    return "";

  // case EAGAIN:
  case EWOULDBLOCK:
    return "would block";

  case EBADF:
    return "invalid descriptor";
  case ECONNRESET:
    return "connection reset";
  case EDESTADDRREQ:
    return "unbound";
  case EFAULT:
    return "invalid user argument";
  case EINTR:
    return "interrupted";
  case EINVAL:
    return "invalid argument";
  case EISCONN:
    return "existing connection";
  case EMSGSIZE:
    return "message size";
  case ENOBUFS:
    return "no buffer space";
  case ENOMEM:
    return "out of memory";
  case ECONNREFUSED:
    return "connection refused";
  case ENOTCONN:
    return "not connected";
  case ENOTSOCK:
    return "invalid descriptor";
  case EOPNOTSUPP:
    return "invalid flags";
  case EPIPE:
    return "pipe";

  default:
    break;
  }
#endif  // WIN32

  return "unknown";
}


bool isConnected(int socket)
{
  if (socket == -1)
  {
    return false;
  }

  char ch = 0;
  int flags = MSG_PEEK;  // NOLINT(misc-const-correctness)
#ifndef WIN32
  flags |= MSG_DONTWAIT;
#endif  // WIN32
  const int read = ::recv(socket, &ch, 1, flags);

  if (read == 0)
  {
    // Socket has been closed.
    return false;
  }

  if (read < 0)
  {
#ifdef WIN32
    const int err = WSAGetLastError();
    switch (err)
    {
    case WSANOTINITIALISED:
    case WSAENETDOWN:
    case WSAENOTCONN:
    case WSAENETRESET:
    case WSAENOTSOCK:
    case WSAESHUTDOWN:
    case WSAEINVAL:
    case WSAECONNRESET:
    case WSAECONNABORTED:
      return false;
    default:
      break;
    }
#else   // WIN32
    const int err = errno;
    switch (err)
    {
      // Disconnection states.
    case ECONNRESET:
    case ENOTCONN:
    case ENOTSOCK:
      return false;
    default:
      break;
    }
#endif  // WIN32
  }

  return true;
}


void setNoDelay(int socket, bool no_delay)
{
  IntVal opt_val = (no_delay) ? 1 : 0;
  ::setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char *>(&opt_val),
               sizeof(opt_val));
}

bool noDelay(int socket)
{
  IntVal opt_val = 0;
  socklen_t len = sizeof(opt_val);
  ::getsockopt(socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char *>(&opt_val), &len);
  return opt_val != 0;
}


bool checkSend(int socket, int ret)
{
  TES_UNUSED(socket);
  if (ret < 0)
  {
#ifdef WIN32
    const int err = WSAGetLastError();
    // WSAECONNABORTED
    if (err != WSAEWOULDBLOCK && err != WSAECONNRESET)
    {
      if (err != WSAECONNRESET && err != WSAECONNABORTED)
      {
        fprintf(stderr, "send error: %s\n", sockErrStr(err));
      }
      return false;
    }
#else   // WIN32
    const int err = errno;
    // EPIPE trips on disconnect.
    if (err != EAGAIN && err != EWOULDBLOCK && err != ECONNRESET && err != EPIPE)
    {
      fprintf(stderr, "send error: %s\n", sockErrStr(err));
      return false;
    }
#endif  // WIN32
  }

  return true;
}


bool checkRecv(int socket, int ret)
{
  TES_UNUSED(socket);
  if (ret < 0)
  {
#ifdef WIN32
    const int err = WSAGetLastError();
    if (err != WSAEWOULDBLOCK && err != WSAECONNRESET)
    {
      fprintf(stderr, "recv error: %s\n", sockErrStr(err));
      return false;
    }
#else   // WIN32
    const int err = errno;
    if (err != EAGAIN && err != EWOULDBLOCK && err != ECONNRESET)
    {
      fprintf(stderr, "recv error: %s\n", sockErrStr(err));
      return false;
    }
#endif  // WIN32
  }

  return true;
}

int getSendBufferSize(int socket)
{
  IntVal buffer_size = 0;
  socklen_t len = sizeof(buffer_size);
  if (::getsockopt(socket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char *>(&buffer_size), &len) < 0)
  {
    return -1;
  }
  return static_cast<int>(buffer_size);
}

bool setSendBufferSize(int socket, int buffer_size)
{
  const socklen_t len = sizeof(buffer_size);
  return ::setsockopt(socket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char *>(&buffer_size), len) >=
         0;
}


int getReceiveBufferSize(int socket)
{
  IntVal buffer_size = 0;
  socklen_t len = sizeof(buffer_size);
  if (::getsockopt(socket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char *>(&buffer_size), &len) < 0)
  {
    return -1;
  }
  return static_cast<int>(buffer_size);
}

bool setReceiveBufferSize(int socket, int buffer_size)
{
  const socklen_t len = sizeof(buffer_size);
  return ::setsockopt(socket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char *>(&buffer_size), len) >=
         0;
}
}  // namespace tes::tcpbase
