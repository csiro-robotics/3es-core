//
// author: Kazys Stepanas
//
#ifndef TES_CORE_TCP_LISTEN_SOCKET_H
#define TES_CORE_TCP_LISTEN_SOCKET_H

#include "CoreConfig.h"

#include <cinttypes>
#include <memory>

namespace tes
{
class TcpSocket;
struct TcpListenSocketDetail;

/// Represents a TCP server socket, listening for connections.
/// Each new connection is serviced by it's own @c TcpSocket,
/// spawned from this class.
class TES_CORE_API TcpListenSocket
{
public:
  /// Constructor.
  TcpListenSocket();
  /// Destructor.
  ~TcpListenSocket();

  /// The port on which the socket is listening, or zero when not listening.
  /// @return The TCP listen port.
  [[nodiscard]] uint16_t port() const;

  /// Start listening for connections on the specified port.
  /// @param port The port to listen on.
  /// @return @c true on success. Failure may be because it is
  /// already listening.
  [[nodiscard]] bool listen(unsigned short port);

  /// Close the connection and stop listening.
  /// Spawned sockets remaining active.
  ///
  /// Safe to call if not already listening.
  void close();

  /// Checks the listening state.
  /// @return @c true if listening for connections.
  [[nodiscard]] bool isListening() const;

  /// Accepts the first pending connection. This will block for the
  /// given timeout period.
  /// @param timeout_ms The timeout to block for, awaiting new connections.
  /// @return A new @c TcpSocket representing the accepted connection,
  ///   or @c nullptr if there are not pending connections. The caller
  ///   takes ownership of the socket and must delete it when done either
  ///   by invoking @c delete, or by calling @c releaseClient() (preferred).
  std::shared_ptr<TcpSocket> accept(unsigned timeout_ms = 0);

private:
  TcpListenSocketDetail *_detail;  ///< Implementation detail.
};

}  // namespace tes

#endif  // TES_CORE_TCP_LISTEN_SOCKET_H
