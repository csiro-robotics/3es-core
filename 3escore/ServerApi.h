///
/// This defines a complete server API which can be disabled at either compile time or runtime.
///
//
// author Kazys Stepanas
//
// Copyright (c) Kazys Stepanas 2023
#ifndef TES_CORE_SERVER_API_H
#define TES_CORE_SERVER_API_H

#ifdef TES_ENABLE
#include "CoreConfig.h"

#include "ConnectionMonitor.h"
#include "CoordinateFrame.h"
#include "Messages.h"
#include "Server.h"
#include "ServerUtil.h"
#endif  // TES_ENABLE

#include <functional>
#include <string>
#include <memory>

/// @ingroup tescpp
/// @defgroup tesmacros 3rd Eye Scene Macro Interface
/// The 3<sup>rd</sup> Eye Scene macro interface provides macros which support the @ref tesserverapi
/// in removing 3es instrumentation code based on the absence of the @c TES_ENABLE definition.

/// @ingroup tesmacros
/// @def TES_ENABLE
/// This macro controls 3es instrumentation, with 3es instrumenation functions and macros enabled
/// when @c TES_ENABLE is present, and removed when @c TES_ENABLE is not defined.

/// @ingroup tesmacros
/// Enable @p statement when @c TES_ENABLE is present.
///
/// The statement is completely removed when 3es instrumentation is not enabled.
///
/// @code
/// // In this main function, all 3es function calls are wrapped in TES_STMT() and are removed when
/// // TES_ENABLE is not defined.
/// // Note there are alternative ways to express this same functionality. See examples.
/// int main()
/// {
///   TES_STMT(tes::ServerPtr tes_server = nullptr);
///   TES_STMT(tes_server = createServer(tes::ServerSettings()));
///   TES_STMT(startServer(tes_server));
///   TES_STMT(stopServer(tes_server));
/// }
/// @endcode
///
/// @param statement The code statement to execute.
#define TES_STMT(statement) statement

/// @ingroup tesmacros
/// @def TES_IF
/// Begins an if statement with condition, but only if TES is enabled. Otherwise the macro is
/// `if constexpr (false)`
/// @param condition The if statement condition.

#ifdef TES_ENABLE
#define TES_IF(condition) if (condition)
#else  // TES_ENABLE
#define TES_IF(condition) if constexpr (false)
#endif  // TES_ENABLE

/// @ingroup tescpp
/// @defgroup tesserverapi 3rd Eye Scene Server API
/// The 3<sup>rd</sup> Eye Scene macro interface provides a way of instrumenting your code with
/// 3<sup>rd</sup> Eye Scene directives, while being able to conditionally remove these directives
/// from selected builds.

namespace tes
{
//-----------------------------------------------------------------------------
// Declarations.
// Most are redundant when TES_ENABLE is defined. They are present for the
// case when it is not defined.
//-----------------------------------------------------------------------------
class Connection;
class ConnectionMonitor;
class Server;
class Shape;
struct ServerInfoMessage;
struct ServerSettings;

#ifndef TES_ENABLE

// Note(KS): this duplication of the enums is painful, but I really need the ServerApi.h header to
// alone when TES_ENABLE is not defined.

enum class ConnectionMode
{
  None,         ///< Invalid
  Synchronous,  ///< The @c ConnectionMonitor requires synchronous calls.
  Asynchronous  ///< The @c ConnectionMonitor runs as a background thread..
};

enum CoordinateFrame
{
  // Right handled frames.
  XYZ,
  XZYNeg,
  YXZNeg,
  YZX,
  ZXY,
  ZYXNeg,
  // Left handed frames
  XYZNeg,
  XZY,
  YXZ,
  YZXNeg,
  ZXYNeg,
  ZYX,

  CFCount,
  CFLeft = XYZNeg
};
#endif  // TES_ENABLE

/// Shared pointer definition for a @c Server object.
using ServerPtr = std::shared_ptr<Server>;

//-----------------------------------------------------------------------------
// Server and connection functions.
//-----------------------------------------------------------------------------

/// @ingroup tesserverapi
/// Create a @c Server object and initialised with the given settings and server info.
/// @param settings The server settings.
/// @param info The server client information.
/// @return A newly created server. Null if the API is disabled.
inline ServerPtr createServer(const ServerSettings &settings, const ServerInfoMessage &info)
{
#ifdef TES_ENABLE
  return tes::Server::create(settings, &info);
#else   // TES_ENABLE
  (void)settings;
  (void)info;
  return {};
#endif  // TES_ENABLE
}


/// @ingroup tesserverapi
/// Create a @c Server object.
/// @param settings The server settings.
/// @param coordinate_frame The server coordinated frame.
/// @param time_unit The server time unit (us). Each frame time step is scaled by this value.
/// The default is 1000us (microseconds) or 1ms. Pass zero to use the default.
/// @param default_time_frame The default time step used when a frame update message has a zero
/// time step. This applies to how the client visualises the frame and has nothing to do with the
/// passing of real time. The default is 33 (ms when using the default @p time_unit ).
inline ServerPtr createServer(const ServerSettings &settings, CoordinateFrame coordinate_frame,
                              uint64_t time_unit = 0, uint32_t default_frame_time = 0)
{
#ifdef TES_ENABLE
  tes::ServerInfoMessage info;
  tes::initDefaultServerInfo(&info);
  info.coordinate_frame = coordinate_frame;
  info.time_unit = (time_unit) ? time_unit : info.time_unit;
  info.default_frame_time = (default_frame_time) ? default_frame_time : info.default_frame_time;
  return tes::Server::create(settings, &info);
#else   // TES_ENABLE
  (void)settings;
  (void)coordinate_frame;
  (void)time_unit;
  (void)default_frame_time;
  return {};
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// @overload
inline ServerPtr createServer(const ServerSettings &settings)
{
#ifdef TES_ENABLE
  return tes::Server::create(settings);
#else   // TES_ENABLE
  (void)settings;
  return {};
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// Start the given @c Server in the specified mode (synchronous or asynchronous).
///
/// After this call, the server can accept connections.
/// @param server The @c Server pointer. May be null.
/// @param mode The server mode: @c ConnectionMode::Synchronous or @c ConnectionMode::Asynchronous.
inline void startServer(Server *server, ConnectionMode mode)
{
#ifdef TES_ENABLE
  if (server)
  {
    server->connectionMonitor()->start(mode);
  }
#else   // TES_ENABLE
  (void)server;
  (void)mode;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// @overload
inline void startServer(Server *server)
{
#ifdef TES_ENABLE
  if (server)
  {
    server->connectionMonitor()->start(ConnectionMode::Asynchronous);
  }
#else   // TES_ENABLE
  (void)server;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// @overload
inline void startServer(const ServerPtr &server, ConnectionMode mode)
{
#ifdef TES_ENABLE
  startServer(server.get(), mode);
#else   // TES_ENABLE
  (void)server;
  (void)mode;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// @overload
inline void startServer(const ServerPtr &server)
{
#ifdef TES_ENABLE
  return startServer(server.get(), ConnectionMode::Asynchronous);
#else   // TES_ENABLE
  (void)server;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// Stop the server, closing all connections and releasing the @c Server pointer.
/// @param server The @c Server pointer. May be null.
inline void stopServer(Server *server)
{
#ifdef TES_ENABLE
  if (server)
  {
    server->close();
    server = nullptr;
  }
#else   // TES_ENABLE
  (void)server;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// @overload
inline void stopServer(const ServerPtr &server)
{
#ifdef TES_ENABLE
  stopServer(server.get());
#else   // TES_ENABLE
  (void)server;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// Call to update the @p connection flushing the frame.
///
/// This update macro performs the following update commands:
///
/// - Call @c Connection::updateTransfers() to transfer any outstanding resource data.
/// - Call @c Connection::updateFrame() to progress the frames.
///
/// @param client The @c Connection pointer. May be null.
/// @param dt The update time step. Zero implies using the @c ServerInfoMessage::default_frame_time
/// from the server which owns this @p Connection
/// @param flush True to allow clients to flush transient objects, false to instruct clients to
/// preserve such objects.
inline void updateConnection(Connection *connection, float dt = 0.0f, bool flush = true)
{
#ifdef TES_ENABLE
  if (connection)
  {
    connection->updateTransfers(0);
    connection->updateFrame(dt, flush);
  }
#else   // TES_ENABLE
  (void)connection;
  (void)dt;
  (void)flush;
#endif  // TES_ENABLE
}


/// @ingroup tesserverapi
/// Call to update the server flushing the frame and potentially monitoring new connections.
///
/// This update macro performs the following update commands:
///
/// - Call @c Server::updateTransfers() to transfer any outstanding resource data.
/// - Call @c Server::updateFrame() to progress the frames.
/// - Update connections, accepting new and expiring old.
///
/// Any additional macro arguments are passed to @c Server::updateFrame(). At the very least
/// a delta time value must be passed (floating point, in seconds). This should be zero when
/// using TES for algorithm debugging, or a valid time delta in real-time debugging.
///
/// @param server The @c Server pointer. May be null.
/// @param dt The update time step. Zero implies using the @c ServerInfoMessage::default_frame_time.
/// @param flush True to allow clients to flush transient objects, false to instruct clients to
/// preserve such objects.
inline void updateServer(Server *server, float dt, bool flush = true)
{
#ifdef TES_ENABLE
  if (server)
  {
    updateConnection(server, dt, flush);
    auto connection_monitor = server->connectionMonitor();
    if (connection_monitor->mode() == ConnectionMode::Synchronous)
    {
      connection_monitor->monitorConnections();
    }
    connection_monitor->commitConnections();
  }
#else   // TES_ENABLE
  (void)server;
  (void)dt;
  (void)flush;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// @overload
inline void updateServer(const ServerPtr &server, float dt, bool flush = true)
{
#ifdef TES_ENABLE
  updateServer(server.get(), dt, flush);
#else   // TES_ENABLE
  (void)server;
  (void)dt;
  (void)flush;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// Wait for the specified time period for a client connection to @p server.
///
/// This monitors the connections, blocking until either a client connection is made or
/// @p timeout_ms has elapsed.
///
/// @param server The server pointer.
/// @param timeout_ms The time period to wait for (ms).
inline bool waitForConnection(Server *server, unsigned timeout_ms)
{
#ifdef TES_ENABLE
  if (server)
  {
    auto connection_monitor = server->connectionMonitor();
    if (connection_monitor->waitForConnection(timeout_ms) > 0)
    {
      connection_monitor->commitConnections();
      return true;
    }
  }
#else   // TES_ENABLE
  (void)server;
  (void)timeout_ms;
#endif  // TES_ENABLE
  return false;
}

/// @ingroup tesserverapi
/// @overload
inline bool waitForConnection(const ServerPtr &server, unsigned timeout_ms)
{
#ifdef TES_ENABLE
  return waitForConnection(server.get(), timeout_ms);
#else   // TES_ENABLE
  (void)server;
  (void)timeout_ms;
  return false;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// Set the callback to invoke when a new client @c Connection is made.
///
/// The provided function can be used to ensure the client connection is updated to match the
/// current server state.
/// @param server The server pointer.
/// @param callback Callback to invoke on each new @c Connection.
inline void setConnectionCallback(Server *server,
                                  const std::function<void(Server &, Connection &)> &callback)
{
#ifdef TES_ENABLE
  if (server)
  {
    auto connection_monitor = server->connectionMonitor();
    connection_monitor->setConnectionCallback(callback);
  }
#else   // TES_ENABLE
  (void)server;
  (void)callback;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// @overload
inline void setConnectionCallback(const ServerPtr &server,
                                  const std::function<void(Server &, Connection &)> &callback)
{
#ifdef TES_ENABLE
  setConnectionCallback(server.get(), callback);
#else   // TES_ENABLE
  (void)server;
  (void)callback;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// Open a file stream connection to @p file_path.
///
/// This opens a new client @c Connection which streams data to a file.
///
/// @param server The server pointer.
/// @param file_path File to save the server messages to.
/// @return The @c Connection pointer or null on failure.
inline std::shared_ptr<Connection> openFileStream(Server *server, const std::string &file_path)
{
#ifdef TES_ENABLE
  if (server)
  {
    auto connection_monitor = server->connectionMonitor();
    return connection_monitor->openFileStream(file_path);
  }
#else   // TES_ENABLE
  (void)server;
  (void)file_path;
#endif  // TES_ENABLE
  return {};
}

/// @ingroup tesserverapi
/// @overload
inline std::shared_ptr<Connection> openFileStream(const ServerPtr &server,
                                                  const std::string &file_path)
{
#ifdef TES_ENABLE
  return openFileStream(server.get(), file_path);
#else   // TES_ENABLE
  (void)server;
  (void)file_path;
  return {};
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// Check if the connection is active - see @c Connection::isActive() .
/// @param connection The connection pointer.
/// @return True if active.
inline bool isConnectionActive(const Connection *connection)
{
#ifdef TES_ENABLE
  return connection && connection->active();
#else   // TES_ENABLE
  (void)connection;
  return false;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// Set the active status of @p connection - see @c Connection::setActive() .
/// @param connection The connection pointer.
/// @return True if active after the call.
inline bool setConnectionActive(Connection *connection, bool active)
{
#ifdef TES_ENABLE
  if (connection)
  {
    connection->setActive(active);
    return connection->active();
  }
#else   // TES_ENABLE
  (void)connection;
  (void)active;
#endif  // TES_ENABLE
  return false;
}

/// @ingroup tesserverapi
/// Check if the server is active - see @c Connection::isActive() .
/// @param server The server pointer.
/// @return True if active.
inline bool isServerActive(Server *server)
{
#ifdef TES_ENABLE
  return isConnectionActive(server);
#else   // TES_ENABLE
  (void)server;
  return false;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// @overload
inline bool isServerActive(const ServerPtr &server)
{
#ifdef TES_ENABLE
  return isServerActive(server.get());
#else   // TES_ENABLE
  (void)server;
  return false;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// Set the active status of @p server - see @c Connection::setActive() .
/// @param server The server pointer.
/// @return True if active after the call.
inline bool setServerActive(Server *server, bool active)
{
#ifdef TES_ENABLE
  return setConnectionActive(server, active);
#else   // TES_ENABLE
  (void)server;
  (void)active;
  return false;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// @overload
inline bool setServerActive(const ServerPtr &server, bool active)
{
#ifdef TES_ENABLE
  return setServerActive(server.get(), active);
#else   // TES_ENABLE
  (void)server;
  (void)active;
  return false;
#endif  // TES_ENABLE
}

//-----------------------------------------------------------------------------
// Category functions.
//-----------------------------------------------------------------------------

/// @ingroup tesserverapi
/// Define a named category for the clients to display.
/// @param connection The @c Server or @c Connection object. Must be a pointer type, may be null.
/// @param name A UTF-8 string.
/// @param category_id ID of the category being named [0, 65535].
/// @param parent_id ID of the parent category, to support category trees. Zero for none. [0, 65535]
/// @param active Default the category to the active state (true/false)?
inline void defineCategory(Connection *connection, const std::string &name, uint16_t category_id,
                           uint16_t parent_id, bool active = true)
{
#ifdef TES_ENABLE
  if (connection)
  {
    tes::CategoryNameMessage msg;
    msg.category_id = static_cast<uint16_t>(category_id);
    msg.parent_id = static_cast<uint16_t>(parent_id);
    msg.default_active = (active) ? 1 : 0;
    const size_t name_len = name.length();
    msg.name_length = (uint16_t)((name_len <= 0xffffu) ? name_len : 0xffffu);
    msg.name = name.c_str();
    tes::sendMessage(*connection, tes::MtCategory, tes::CategoryNameMessage::MessageId, msg);
  }
#else   // TES_ENABLE
  (void)connection;
  (void)name;
  (void)category_id;
  (void)parent_id;
  (void)active;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// @overload
inline void defineCategory(Connection *connection, const std::string &name, uint16_t category_id,
                           bool active = true)
{
#ifdef TES_ENABLE
  defineCategory(connection, name, category_id, 0, active);
#else   // TES_ENABLE
  (void)connection;
  (void)name;
  (void)category_id;
  (void)active;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// @overload
inline void defineCategory(const ServerPtr &server, const std::string &name, uint16_t category_id,
                           bool active = true)
{
#ifdef TES_ENABLE
  defineCategory(server.get(), name, category_id, active);
#else   // TES_ENABLE
  (void)server;
  (void)name;
  (void)category_id;
  (void)active;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// @overload
inline void defineCategory(const ServerPtr &server, const std::string &name, uint16_t category_id,
                           uint16_t parent_id, bool active = true)
{
#ifdef TES_ENABLE
  defineCategory(server.get(), name, category_id, parent_id, active);
#else   // TES_ENABLE
  (void)server;
  (void)name;
  (void)category_id;
  (void)parent_id;
  (void)active;
#endif  // TES_ENABLE
}

//-----------------------------------------------------------------------------
// Shape functions.
//-----------------------------------------------------------------------------

/// @ingroup tesserverapi
/// Send the create message for a @p shape to the given @p connection or server.
/// @param connection The @c Connection or @c Server object to send to. May be null.
/// @param shape The shape to send.
/// @return The number of bytes sent, or -1 on failure. A null @p connection yields a zero result.
inline int create(Connection *connection, const Shape &shape)
{
#ifdef TES_ENABLE
  if (connection)
  {
    return connection->create(shape);
  }
#else   // TES_ENABLE
  (void)connection;
  (void)shape;
#endif  // TES_ENABLE
  return 0;
}

/// @ingroup tesserverapi
/// @overload
inline int create(const ServerPtr &server, const Shape &shape)
{
#ifdef TES_ENABLE
  return create(server.get(), shape);
#else   // TES_ENABLE
  (void)server;
  (void)shape;
  return 0;
#endif  // TES_ENABLE
}

/// @ingroup tesserverapi
/// Send the destroy message for a @p shape to the given @p connection or server.
/// @param connection The @c Connection or @c Server object to send to. May be null.
/// @param shape The shape to destroy.
/// @return The number of bytes sent, or -1 on failure. A null @p connection yields a zero result.
inline int destroy(Connection *connection, const Shape &shape)
{
#ifdef TES_ENABLE
  if (connection)
  {
    return connection->destroy(shape);
  }
#else   // TES_ENABLE
  (void)connection;
  (void)shape;
#endif  // TES_ENABLE
  return 0;
}

/// @ingroup tesserverapi
/// @overload
inline int destroy(const ServerPtr &server, const Shape &shape)
{
#ifdef TES_ENABLE
  return destroy(server.get(), shape);
#else   // TES_ENABLE
  (void)server;
  (void)shape;
  return 0;
#endif  // TES_ENABLE
}

#ifdef TES_ENABLE
/// A helper class which sends a create message for a shape in the constructor and ensures the
/// destroy message is sent on destruction (when out of scope).
///
/// The class is designed to use a move constructor for the shape type - @c S - so the shape
/// should be declared when the @p ScopedShape is constructed.
///
/// @code
/// void showBox(Server *server)
/// {
///   TES_STMT(tes::ScopedShape::<Box> tes::box(
///       server, tes::Box(1, Transform(tes::Vector3f(1, 2, 3), tes::Vector3f(3, 2, 1))))
///     ->setColour(tes::Colour(tes::Colour::Green)));
///   updateServer(server);
/// }
/// @endcode
///
/// Usage of @c ScopedShape should generally be wrapped in @c TES_STMT() to ensure it is correctly
/// removed when @c TES_ENABLE is not defined.
///
/// @tparam S A derivation of @c tes::Shape .
template <typename S>
class ScopedShape
{
public:
  /// The @c Server or @c Connection pointer. May be null.
  Connection *const connection = nullptr;
  /// The @c Shape class.
  S shape;

  /// Construct, sending the given @p shape to the @p connection .
  /// @param connection The @c Server or @c Connection pointer. May be null.
  /// @param shape The shape to send.
  ScopedShape(Connection *connection, S &&shape)
    : connection(connection)
    , shape(std::move(shape))
  {
    if (connection)
    {
      connection->create(this->shape);
    }
  }

  /// Destruct, sending the destroy message for @p shape, provided it is not transient.
  /// Does nothing with a transient shape.
  ~ScopedShape()
  {
    if (connection && !shape->isTransient())
    {
      connection->destroy(shape);
    }
  }

  /// Dereference operator, mapping function calls to the @c shape.
  /// @return A pointer to the @c shape.
  S *operator->() { return &shape; }
  /// Dereference operator, mapping function calls to the @c shape.
  /// @return A pointer to the @c shape.
  const S *operator->() const { return &shape; }

  /// Send an update message for the @c shape member on @c connection.
  ///
  /// This can be called after modifying the @c shape attributes such as transform or colour.
  /// @param update_flags A set of @c UpdateFlag values, used to limit what attributes are updated.
  /// @return The number of bytes sent, -1 on failure, zero when @c connection is null.
  int update(unsigned update_flags)
  {
    if (connection)
    {
      shape.setFlags(static_cast<uint16_t>(update_flags) | shape.flags());
      return connection->update(shape);
    }
    return 0;
  }

  /// @overload
  int update()
  {
    if (connection)
    {
      shape.setFlags((shape.flags() | UFUpdateMode) & ~UFPosRotScaleColour);
      return connection->update(shape);
    }
    return 0;
  }
};
#endif  // TES_ENABLE
}  // namespace tes

#endif  // TES_CORE_SERVER_API_H
