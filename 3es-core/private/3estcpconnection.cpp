//
// author: Kazys Stepanas
//
#include "3estcpconnection.h"

#include <3estcpsocket.h>

using namespace tes;

TcpConnection::TcpConnection(TcpSocket *clientSocket, const ServerSettings &settings)//unsigned flags, uint16_t bufferSize)
: BaseConnection(settings)
, _client(clientSocket)
{
}


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
