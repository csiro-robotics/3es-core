//
// author: Kazys Stepanas
//
#ifndef _3ESTCPCONNECTION_H_
#define _3ESTCPCONNECTION_H_

#include "../3esserver.h"

#include "3esbaseconnection.h"

namespace tes
{
class TcpSocket;

/// A TCP based implementation of a 3es @c Connection. Each @c TcpConnection represents a remote client connection.
///
/// These connections are created by the @c TcpServer.
class TcpConnection : public BaseConnection
{
public:
  /// Create a new connection using the given @p clientSocket.
  /// @param clientSocket The socket to communicate on.
  /// @param settings Various server settings to initialise with.
  TcpConnection(TcpSocket *clientSocket, const ServerSettings &settings);

  /// Destructor.s
  ~TcpConnection();

  /// Close the socket connection.
  void close() override;

  const char *address() const override;
  uint16_t port() const override;
  bool isConnected() const override;

protected:
  int writeBytes(const uint8_t *data, int byteCount) override;

private:
  TcpSocket *_client;
};
}  // namespace tes

#endif  // _3ESTCPCONNECTION_H_
