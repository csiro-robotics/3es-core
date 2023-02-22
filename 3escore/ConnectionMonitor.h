//
// author: Kazys Stepanas
//
#ifndef TES_CORE_CONNECTION_MONITOR_H
#define TES_CORE_CONNECTION_MONITOR_H

#include "CoreConfig.h"

#include <functional>
#include <memory>
#include <string>

namespace tes
{
class Connection;
class Server;

/// Controls how the monitor behaves - synchronously or asynchronously.
enum class ConnectionMode
{
  None,         ///< Invalid
  Synchronous,  ///< The @c ConnectionMonitor requires synchronous calls.
  Asynchronous  ///< The @c ConnectionMonitor runs as a background thread..
};

/// A utility class for monitoring new connections for a @c Server.
///
/// The monitor manages listening for new connections and expiring old ones.
/// Doing so can be time consuming, so the monitor supports running its
/// own monitor thread. It also supports synchronous operation in case
/// connection monitoring is unnecessary, or unable to be pushed off thread.
///
/// Asynchronous mode is activated by calling @c start() and stopped with
/// @c stop(). Calls to @c join() will block until the monitor thread has
/// completed, but should only be called after @c stop() has been called.
/// The @c commitConnections() method must still be called by the main thread
/// (synchronously) to control when connections are activated and deactivated.
///
/// Synchronous mode is supported by calling @c monitorConnections() to
/// accept new connections. This must be followed by a call to
/// @c commitConnections() to commit the changes to the owning @c Server.
///
/// @par Synchronous Usage
/// @code
/// float dt = 0;
/// Server *server = Server::create()
/// server->connectionMonitor()->start(tes::ConnectionMode::Synchronous);
/// for (;;)
/// {
///   // Prepare frame.
///   //...
///
///   server->updateFrame(dt);
///   server->connectionMonitor()->monitorConnections();
///   server->connectionMonitor()->commitConnections();
///
///   // Loop end...
/// }
/// @endcode
///
/// @par Asynchronous Usage
/// @code
/// float dt = 0;
/// Server *server = Server::create()
/// server->connectionMonitor()->start(tes::ConnectionMode::Asynchronous);
/// for (;;)
/// {
///   // Prepare frame.
///   //...
///
///   server->updateFrame(dt);
///   server->connectionMonitor()->commitConnections();
///
///   // Loop end...
/// }
///
/// server->connectionMonitor()->stop();  // Safe even if Synchronous
/// server->connectionMonitor()->join();  // Safe even if Synchronous
/// @endcode
class TES_CORE_API ConnectionMonitor
{
protected:
  /// Protected virtual destructor.
  virtual ~ConnectionMonitor();

public:
  /// Report the port being used by the connection monitor.
  ///
  /// This may be TCP specific.
  ///
  /// @return The port connections are being monitored on.
  [[nodiscard]] virtual unsigned short port() const = 0;

  /// Starts the monitor listening in the specified mode.
  ///
  /// The listening thread is started if @p mode is @c tes::ConnectionMode::Asynchronous.
  /// @param mode The listening mode. Mode @c None is ignored.
  /// @return True if listening is running in the specified @p mode.
  /// This includes both newly started and if it was already running in that
  /// mode. False is returned if @p mode is @c None, or differs from the
  /// running mode.
  virtual bool start(ConnectionMode mode) = 0;

  /// Stops listening for further connections. This requests termination
  /// of the monitor thread if running.
  ///
  /// Safe to call if not running.
  virtual void stop() = 0;
  /// Called to join the monitor thread. Returns immediately
  /// if not running.
  virtual void join() = 0;

  /// Returns true if the connection monitor has start.
  /// @return True if running.
  [[nodiscard]] virtual bool isRunning() const = 0;

  /// Returns the current running mode.
  ///
  /// @c ConnectionMode::Asynchronous mode is set as soon as @c
  /// start(tes::ConnectionMode::Asynchronous) is called and drops to @c None after calling
  /// @c stop() once the thread has stopped.
  ///
  /// @c ConnectionMode::Synchronous mode is set as soon as
  /// @c start(tes::ConnectionMode::Synchronous) is called and drops to @c ConnectionMode::None
  /// on calling @c stop().
  ///
  /// The mode is @c ConnectionMode::None if not running in either mode.
  [[nodiscard]] virtual ConnectionMode mode() const = 0;

  /// Wait up to @p timeout_ms milliseconds for a connection.
  /// Returns immediately if we already have a connection.
  /// @param timeout_ms The time out to wait in milliseconds.
  /// @return The number of connections on returning. These may need to be committed.
  virtual int waitForConnection(unsigned timeout_ms) = 0;

  /// Accepts new connections and checks for expired connections, but
  /// effects neither in the @c Server.
  ///
  /// This is either called on the main thread for synchronous operation,
  /// or internally in asynchronous mode.
  virtual void monitorConnections() = 0;

  /// Opens a @c Connection object which serialises directly to the local file system.
  ///
  /// The connection persisits until either the monitor is stopped, or until @p Connection::close()
  /// is called. In asynchronous mode, the pointer cannot be used after @c close() is called.
  ///
  /// @param file_path The path to the file to open/write to.
  /// @return A pointer to a @c Connection object which represents the file stream.
  virtual std::shared_ptr<Connection> openFileStream(const std::string &file_path) = 0;

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
  virtual void setConnectionCallback(void (*callback)(Server &, Connection &, void *),
                                     void *user) = 0;

  /// An overload of @p setConnectionCallback() using the C++11 @c funtion object.
  /// Both methods are provided to cater for potential ABI issues.
  ///
  /// No user argument is supported as the flexibility of @c std::function obviates the
  /// for such.
  ///
  /// @param callback The function to invoke for each new connection.
  virtual void setConnectionCallback(
    const std::function<void(Server &, Connection &)> &callback) = 0;

  /// Retrieve a function object representing the connection callback.
  /// @return The current function wrapper invoked for each new connection.
  [[nodiscard]] virtual const std::function<void(Server &, Connection &)> &connectionCallback()
    const = 0;

  /// Migrates new connections to the owning @c Server and removes expired
  /// connections.
  ///
  /// For each new connection, the callback set in @c setConnectionCallback() is invoked,
  /// passing the server, connection and @p user argument.
  ///
  /// @param callback When given, called for each new connection.
  /// @param user User argument passed to @p callback.
  virtual void commitConnections() = 0;
};
}  // namespace tes

#endif  // TES_CORE_CONNECTION_MONITOR_H
