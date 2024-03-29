//
// author: Kazys Stepanas
//
#include <3escore/TcpSocket.h>

#include "TcpDetail.h"

using namespace tes;

const unsigned TcpSocket::IndefiniteTimeout = ~unsigned(0u);

TcpSocket::TcpSocket()
  : _detail(std::make_unique<TcpSocketDetail>)
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
  if (_detail->socket)
  {
    return false;
  }

  _detail->socket = std::make_unique<QTcpSocket>();
  _detail->socket->connectToHost(host, port);
  return true;
}


void TcpSocket::close()
{
  if (_detail->socket)
  {
    _detail->socket->close();
    _detail->socket.release();
  }
}


bool TcpSocket::isConnected() const
{
  if (_detail->socket)
  {
    switch (_detail->socket->state())
    {
    case QTcpSocket::ConnectingState:
    case QTcpSocket::ConnectedState:
      return true;
    default:
      break;
    }
  }

  return false;
}


void TcpSocket::setNoDelay(bool no_delay)
{
  if (_detail->socket)
  {
    _detail->socket->setSocketOption(QTcpSocket::LowDelayOption, no_delay ? 1 : 0);
  }
}


bool TcpSocket::noDelay() const
{
  if (_detail->socket)
  {
    return _detail->socket->socketOption(QTcpSocket::LowDelayOption).toInt() == 1;
  }
  return false;
}


void TcpSocket::setReadTimeout(unsigned timeout_ms)
{
  _detail->read_timeout = timeout_ms;
}


unsigned TcpSocket::readTimeout() const
{
  return _detail->read_timeout;
}


void TcpSocket::setIndefiniteReadTimeout()
{
  _detail->read_timeout = IndefiniteTimeout;
}


void TcpSocket::setWriteTimeout(unsigned timeout_ms)
{
  _detail->write_timeout = timeout_ms;
}


unsigned TcpSocket::writeTimeout() const
{
  return _detail->write_timeout;
}


void TcpSocket::setIndefiniteWriteTimeout()
{
  _detail->write_timeout = IndefiniteTimeout;
}


void TcpSocket::setReadBufferSize(int bufferSize)
{
  if (_detail->socket)
  {
    _detail->socket->setSocketOption(QTcpSocket::ReceiveBufferSizeSocketOption, bufferSize);
  }
}


int TcpSocket::readBufferSize() const
{
  if (_detail->socket)
  {
    _detail->socket->socketOption(QTcpSocket::ReceiveBufferSizeSocketOption).toInt();
  }
  return 0;
}


void TcpSocket::setSendBufferSize(int bufferSize)
{
  if (_detail->socket)
  {
    _detail->socket->setSocketOption(QTcpSocket::SendBufferSizeSocketOption, bufferSize);
  }
}


int TcpSocket::sendBufferSize() const
{
  if (_detail->socket)
  {
    _detail->socket->socketOption(QTcpSocket::SendBufferSizeSocketOption).toInt();
  }
  return 0;
}


int TcpSocket::read(char *buffer, int bufferLength) const
{
  if (isConnected() && _detail->socket->waitForReadyRead(_detail->read_timeout))
  {
    return _detail->socket->read(buffer, bufferLength);
  }

  return 0;
}


int TcpSocket::readAvailable(char *buffer, int bufferLength) const
{
  if (!isConnected())
  {
    return -1;
  }

  _detail->socket->waitForReadyRead(0);
  return _detail->socket->read(buffer, bufferLength);
}


int TcpSocket::write(const char *buffer, int bufferLength) const
{
  if (!_detail->socket)
  {
    return -1;
  }

  int wrote = -1;
  int totalWritten = 0;
  while (isConnected() && wrote < 0 && bufferLength > 0)
  {
    wrote = _detail->socket->write(buffer, bufferLength);
    if (wrote > 0)
    {
      totalWritten += wrote;
      buffer += wrote;
      bufferLength -= wrote;
    }
    else
    {
      _detail->socket->waitForBytesWritten(0);
    }
  }

  if (isConnected())
  {
    _detail->socket->waitForBytesWritten(0);
  }

  return wrote >= 0 ? totalWritten : -1;
}


uint16_t TcpSocket::port() const
{
  return _detail->socket ? _detail->socket->localPort() : 0;
}
