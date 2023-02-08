//
// author: Kazys Stepanas
//
#include <3escore/TcpListenSocket.h>

#include <3escore/TcpSocket.h>

#include "TcpDetail.h"

#include <QHostAddress>

#include <cstring>

using namespace tes;

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
  return _detail->listen_socket.serverPort();
}


bool TcpListenSocket::listen(uint16_t port)
{
  if (isListening())
  {
    return false;
  }

  return _detail->listen_socket.listen(QHostAddress::Any, port);
}


void TcpListenSocket::close()
{
  if (isListening())
  {
    _detail->listen_socket.close();
  }
}


bool TcpListenSocket::isListening() const
{
  return _detail->listen_socket.isListening();
}


TcpSocket::Ptr tcpListenSocket::accept(unsigned timeout_ms)
{
  if (!_detail->listen_socket.waitForNewConnection(timeout_ms))
  {
    return {};
  }

  if (!_detail->listen_socket.hasPendingConnections())
  {
    return {};
  }

  QTcpSocket *new_socket = _detail->listen_socket.nextPendingConnection();
  if (!new_socket)
  {
    return {};
  }
  TcpSocketDetail *clientDetail = new TcpSocketDetail;
  clientDetail->socket = new_socket;
  return std::make_shared<TcpSocket>(clientDetail);
}
