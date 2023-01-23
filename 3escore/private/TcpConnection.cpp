//
// author: Kazys Stepanas
//
#include "TcpConnection.h"

#include <TcpSocket.h>

using namespace tes;

TcpConnection::TcpConnection(TcpSocket *clientSocket, const ServerSettings &settings)
  : BaseConnection(settings)
  , _client(clientSocket)
{}


TcpConnection::~TcpConnection()
{
  close();
  delete _client;
}


void TcpConnection::close()
{
  if (_client)
  {
    _client->close();
  }
}


const char *TcpConnection::address() const
{
  // FIXME:
  //_client->address();
  return "";
}


uint16_t TcpConnection::port() const
{
  return _client->port();
}


bool TcpConnection::isConnected() const
{
  return _client && _client->isConnected();
}


int TcpConnection::writeBytes(const uint8_t *data, int byteCount)
{
  return _client->write(data, byteCount);
}
