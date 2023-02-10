//
// author: Kazys Stepanas
//
#include "TcpConnection.h"

#include <3escore/TcpSocket.h>

namespace tes
{
TcpConnection::TcpConnection(std::shared_ptr<TcpSocket> client_socket,
                             const ServerSettings &settings)
  : BaseConnection(settings)
  , _client(std::move(client_socket))
{}


TcpConnection::~TcpConnection()
{
  close();
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


int TcpConnection::writeBytes(const uint8_t *data, int byte_count)
{
  return _client->write(data, byte_count);
}
}  // namespace tes
