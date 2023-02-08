//
// author: Kazys Stepanas
//
#ifndef TES_CORE_PRIVATE_TCP_CONNECTION_MONITOR_H
#define TES_CORE_PRIVATE_TCP_CONNECTION_MONITOR_H

#include <3escore/CoreConfig.h>

#include <3escore/ConnectionMonitor.h>
#include <3escore/SpinLock.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

namespace tes
{
class BaseConnection;
class TcpServer;
class TcpListenSocket;

/// Implements a @c ConnectionMonitor using the TCP protocol. Intended only for use with a @c
/// TcpServer.
class TcpConnectionMonitor final : public ConnectionMonitor
{
public:
  using Lock = std::mutex;

  /// Error codes.
  enum ConnectionError
  {
    CENone,
    /// Failed to listen on the requested port.
    CEListenFailure,
    /// Timeout has expired.
    CETimeout
  };

  /// Construct a TCP based connection monitor for @p server.
  /// @param server The server owning this connection monitor.
  TcpConnectionMonitor(TcpServer &server);

  /// Destructor.
  ~TcpConnectionMonitor() final;

  /// Get the @c TcpServer which owns this @c ConnectionMonitor.
  /// @return The owning server.
  TcpServer &server() { return _server; }

  /// @overload
  [[nodiscard]] const TcpServer &server() const { return _server; }

  /// Get the TCP socket used to manage connections.
  /// @return The @c TcpListenSocket the connection monitor listens on.
  ///    May be null when not currently listening (before @c start()).
  [[nodiscard]] const TcpListenSocket *socket() const { return _listen.get(); }

  /// Get the last error code.
  /// @return The @c ConnectionError for the last error.
  [[nodiscard]] int lastErrorCode() const;

  /// Clear the last error code.
  /// @return The @c ConnectionError for the last error.
  [[nodiscard]] int clearErrorCode();

  /// Report the port on which the connection monitor is listening.
  /// @return The listen port or zero if not listening.
  uint16_t port() const final;

  /// Starts the monitor thread (asynchronous mode).
  bool start(Mode mode) final;
  /// Requests termination of the monitor thread.
  /// Safe to call if not running.
  void stop() final;
  /// Called to join the monitor thread. Returns immediately
  /// if not running.
  void join() final;

  /// Returns true if the connection monitor has start.
  /// @return True if running.
  bool isRunning() const final;

  /// Returns the current running mode.
  ///
  /// @c Asynchronous mode is set as soon as @c start(Asynchronous) is called and
  /// drops to @c None after calling @c stop() once the thread has stopped.
  ///
  /// @c Synchronous mode is set as soon as @c start(Synchronous) is called and
  /// drops to @c None on calling @c stop().
  ///
  /// The mode is @c None if not running in either mode.
  Mode mode() const final;

  /// Wait up to @p timeout_ms milliseconds for a connection.
  /// Returns immediately if we already have a connection.
  /// @param timeout_ms The time out to wait in milliseconds.
  /// @return The number of connections on returning. These may need to be committed.
  [[nodiscard]] int waitForConnection(unsigned timeout_ms) final;

  /// Accepts new connections and checks for expired connections, but
  /// effects neither in the @c Server.
  ///
  /// This is either called on the main thread for synchronous operation,
  /// or internally in asynchronous mode.
  void monitorConnections() final;

  /// Opens a @c Connection object which serialises directly to the local file system.
  ///
  /// The connection persisits until either the monitor is stopped, or until @p Connection::close()
  /// is called. In asynchronous mode, the pointer cannot be used after @c close() is called.
  ///
  /// @param file_path The path to the file to open/write to.
  /// @return A pointer to a @c Connection object which represents the file stream.
  std::shared_ptr<Connection> openFileStream(const char *file_path) final;

  /// Sets the callback invoked for each new connection.
  ///
  /// This is invoked from @p commitConnections() for each new connection.
  /// The arguments passed to the callback are:
  /// - @c server : the @c Server object.
  /// - @c connection : the new @c Connection object.
  /// - @c user : the @c user argument given here.
  ///
  /// Write only.
  ///
  /// @param callback The callback function pointer.
  /// @param user A user pointer passed to the @c callback whenever it is invoked.
  void setConnectionCallback(void (*callback)(Server &, Connection &, void *), void *user) final;

  /// An overload of @p setConnectionCallback() using the C++11 @c funtion object.
  /// Both methods are provided to cater for potential ABI issues.
  ///
  /// No user argument is supported as the flexibility of @c std::function obviates the
  /// for such.
  ///
  /// @param callback The function to invoke for each new connection.
  void setConnectionCallback(const std::function<void(Server &, Connection &)> &callback) final;

  /// Retrieve a function object representing the connection callback.
  /// @return The current function wrapper invoked for each new connection.
  [[nodiscard]] const std::function<void(Server &, Connection &)> &connectionCallback() const final;

  /// Migrates new connections to the owning @c Server and removes expired
  /// connections.
  ///
  /// For each new connection, the callback set in @c setConnectionCallback() is invoked,
  /// passing the server, connection and @p user argument.
  ///
  /// @param callback When given, called for each new connection.
  /// @param user User argument passed to @p callback.
  void commitConnections() final;

private:
  bool listen();
  void stopListening();
  void monitorThread();

  TcpServer &_server;
  std::unique_ptr<TcpListenSocket> _listen;
  std::function<void(Server &, Connection &)> _on_new_connection;
  Mode _mode = None;  ///< Current execution mode.
  std::vector<std::shared_ptr<Connection>> _connections;
  std::vector<std::shared_ptr<Connection>> _expired;
  std::atomic_int _error_code = { 0 };
  std::atomic_uint16_t _listen_port = { 0 };
  std::atomic_bool _running = { false };
  std::atomic_bool _quit_flag = { false };
  mutable Lock _connection_lock;
  std::unique_ptr<std::thread> _thread;
};
}  // namespace tes

#endif  // TES_CORE_PRIVATE_TCP_CONNECTION_MONITOR_H
