//
// author: Kazys Stepanas
//
#include <3escore/TcpSocket.h>

#include <3escore/CoreUtil.h>

#include "TcpBase.h"
#include "TcpDetail.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <thread>

#ifdef WIN32
#include <Ws2tcpip.h>
#endif  // WIN32

namespace tes
{
const unsigned TcpSocket::IndefiniteTimeout = ~0u;

namespace
{
const char *socketErrorString(int err)
{
  switch (err)
  {
    //    case EAGAIN:
    //      return "again";
  case EWOULDBLOCK:
    return "would block";
  case EBADF:
    return "bad socket";
  case ECONNRESET:
    return "connection reset";
  case EINTR:
    return "interrupt";
  case EINVAL:
    return "no out of bound data";
  case ENOTCONN:
    return "not connected";
  case ENOTSOCK:
    return "invalid socket descriptor";
  case EOPNOTSUPP:
    return "not supported";
  case ETIMEDOUT:
    return "timed out";
  case EIO:
    return "io error";
  case ENOBUFS:
    return "insufficient resources";
  case ENOMEM:
    return "out of memory";
  case ECONNREFUSED:
    return "connection refused";
  default:
    break;
  }

  return "unknown";
}
}  // namespace


TcpSocket::TcpSocket()
  : _detail(std::make_unique<TcpSocketDetail>())
{}


TcpSocket::TcpSocket(std::unique_ptr<TcpSocketDetail> &&detail)
  : _detail(std::move(detail))
{}


TcpSocket::~TcpSocket()
{
  close();
}


bool TcpSocket::open(const char *host, uint16_t port)
{
  if (_detail->socket != -1)
  {
    return false;
  }

  _detail->socket = tcpbase::create();
  _detail->address.sin_family = AF_INET;
  _detail->address.sin_port = htons(port);

  if (::inet_pton(AF_INET, host, &_detail->address.sin_addr) <= 0)
  {
    close();  // -1 and 0 are inet_pton() errors
    return false;
  }

  // Connect to server
  if (::connect(_detail->socket, reinterpret_cast<struct sockaddr *>(&_detail->address),
                sizeof(_detail->address)) != 0)
  {
    const int err = errno;
    if (err && err != ECONNREFUSED)
    {
      fprintf(stderr, "errno : %d -> %s\n", err, socketErrorString(err));
    }

    if (err == EAGAIN)
    {
      fprintf(stderr, "...\n");
    }
    close();
    return false;
  }

#ifdef WIN32
  // Set non blocking.
  u_long i_mode = 1;
  ::ioctlsocket(_detail->socket, FIONBIO, &i_mode);
#endif  // WIN32

  return true;
}


void TcpSocket::close()
{
  if (_detail->socket != -1)
  {
    tcpbase::close(_detail->socket);
    memset(&_detail->address, 0, sizeof(_detail->address));
    _detail->socket = -1;
  }
}


bool TcpSocket::isConnected() const
{
  if (_detail->socket == -1)
  {
    return false;
  }

  return tcpbase::isConnected(_detail->socket);
}


void TcpSocket::setNoDelay(bool no_delay)
{
  tcpbase::setNoDelay(_detail->socket, no_delay);
}


bool TcpSocket::noDelay() const
{
  return tcpbase::noDelay(_detail->socket);
}


void TcpSocket::setReadTimeout(unsigned timeout_ms)
{
  tcpbase::setReceiveTimeout(_detail->socket, timeout_ms);
}


unsigned TcpSocket::readTimeout() const
{
  return tcpbase::getReceiveTimeout(_detail->socket);
}


void TcpSocket::setIndefiniteReadTimeout()
{
  setReadTimeout(IndefiniteTimeout);
}


void TcpSocket::setWriteTimeout(unsigned timeout_ms)
{
  tcpbase::setSendTimeout(_detail->socket, timeout_ms);
}


unsigned TcpSocket::writeTimeout() const
{
  return tcpbase::getSendTimeout(_detail->socket);
}


void TcpSocket::setIndefiniteWriteTimeout()
{
  setWriteTimeout(IndefiniteTimeout);
}


void TcpSocket::setReadBufferSize(int buffer_size)
{
  tcpbase::setReceiveBufferSize(_detail->socket, buffer_size);
}


int TcpSocket::readBufferSize() const
{
  return tcpbase::getReceiveBufferSize(_detail->socket);
}


void TcpSocket::setSendBufferSize(int buffer_size)
{
  tcpbase::setSendBufferSize(_detail->socket, buffer_size);
}


int TcpSocket::sendBufferSize() const
{
  return tcpbase::getSendBufferSize(_detail->socket);
}


int TcpSocket::read(char *buffer, int buffer_length) const
{
#if 0
  int bytesRead = 0;  // bytes read so far

  while (bytesRead < buffer_length)
  {
    const auto read =
      static_cast<int>(::recv(_detail->socket, buffer + bytesRead, buffer_length - bytesRead, 0));

    if (read < 0)
    {
      if (!tcpbase::checkRecv(_detail->socket, read))
      {
        return -1;
      }
      return 0;
    }
    else if (read == 0)
    {
      return bytesRead;
    }

    bytesRead += read;
  }

  return bytesRead;
#else   // #
  if (_detail->socket == -1)
  {
    return -1;
  }

  const int flags = MSG_WAITALL;
  const auto read = static_cast<int>(::recv(_detail->socket, buffer, buffer_length, flags));
  if (read < 0)
  {
    if (!tcpbase::checkRecv(_detail->socket, read))
    {
      return -1;
    }
    return 0;
  }
  return read;
#endif  // #
}


int TcpSocket::readAvailable(char *buffer, int buffer_length) const
{
  if (_detail->socket == -1)
  {
    return -1;
  }

  //  tcpbase::disableBlocking(_detail->socket);
  int flags = 0;  // NOLINT(misc-const-correctness)
#ifndef WIN32
  flags |= MSG_DONTWAIT;
#endif  // WIN32
  const auto read = static_cast<int>(::recv(_detail->socket, buffer, buffer_length, flags));
  if (read == -1)
  {
    if (!tcpbase::checkRecv(_detail->socket, read))
    {
      return -1;
    }
    return 0;
  }
  return read;
}


int TcpSocket::write(const char *buffer, int buffer_length) const
{
  if (_detail->socket == -1)
  {
    return -1;
  }

  int bytes_sent = 0;

  while (bytes_sent < buffer_length)
  {
    int flags = 0;  // NOLINT(misc-const-correctness)
#ifdef __linux__
    flags = MSG_NOSIGNAL;
#endif  // __linux__
    int sent;
    bool retry = true;


    while (retry)
    {
      retry = false;
      sent = static_cast<int>(::send(_detail->socket,
                                     reinterpret_cast<const char *>(buffer) + bytes_sent,
                                     buffer_length - bytes_sent, flags));
#ifdef WIN32
      if (sent < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
#else   // WIN32
      if (sent < 0 && errno == EWOULDBLOCK)
#endif  // WIN32
      {
        // Send buffer full. Wait and retry.
        std::this_thread::yield();
        fd_set wfds;
        struct timeval tv;
        FD_ZERO(&wfds);
        FD_SET(_detail->socket, &wfds);

        tv.tv_sec = 0;
        tv.tv_usec = 1000;

        sent = ::select(1, nullptr, &wfds, nullptr, &tv);
        retry = sent >= 0;
      }
    }


    if (sent < 0)
    {
      if (!tcpbase::checkSend(_detail->socket, sent))
      {
        return -1;
      }
      return 0;
    }

    if (sent == 0)
    {
      return bytes_sent;
    }

    bytes_sent += sent;
  }

  return bytes_sent;
}


uint16_t TcpSocket::port() const
{
  return _detail->address.sin_port;
}
}  // namespace tes
