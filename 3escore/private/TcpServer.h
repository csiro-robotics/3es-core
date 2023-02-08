//
// author: Kazys Stepanas
//
#ifndef TES_CORE_PRIVATE_TCP_SERVER_H
#define TES_CORE_PRIVATE_TCP_SERVER_H

#include "../Server.h"

//
#include <3escore/MeshMessages.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <vector>

namespace tes
{
class BaseConnection;
class TcpConnectionMonitor;
class TcpListenSocket;
class TcpServer;

/// A TCP based implementation of a 3es @c Server.
class TcpServer final : public Server
{
public:
  using Lock = std::mutex;

  TcpServer(const ServerSettings &settings, const ServerInfoMessage *server_info);
  ~TcpServer() final;

  const ServerSettings &settings() const { return _settings; }

  unsigned flags() const final;

  /// Close all connections and stop listening for new connections.
  void close() final;

  /// Activate/deactivate the connection. Messages are ignored while inactive.
  /// @param enable The active state to set.
  void setActive(bool enable) final;

  /// Check if currently active.
  /// @return True while active.
  bool active() const final;

  /// Always "TcpServer".
  /// @return "TcpServer".
  const char *address() const final;
  /// Return the listen port or zero when not listening.
  /// @return The listen port.
  uint16_t port() const final;

  /// Any current connections?
  /// @return True if we have at least one connection.
  bool isConnected() const final;

  int create(const Shape &shape) final;
  int destroy(const Shape &shape) final;
  int update(const Shape &shape) final;

  int updateFrame(float dt, bool flush) final;
  int updateTransfers(unsigned byte_limit) final;

  /// Override
  /// @param resource The resource to reference.
  /// @return The count from the last connection.
  unsigned referenceResource(const Resource *resource) final;

  /// Override
  /// @param resource The resource to release.
  /// @return The count from the last connection.
  unsigned releaseResource(const Resource *resource) final;

  /// Ignored. Controlled by this class.
  /// @param info Ignored.
  /// @return False.
  bool sendServerInfo(const ServerInfoMessage &info) final;

  int send(const PacketWriter &packet, bool allow_collation) final;
  int send(const CollatedPacket &collated) final;
  int send(const uint8_t *data, int byte_count, bool allow_collation) final;

  std::shared_ptr<ConnectionMonitor> connectionMonitor() final;
  unsigned connectionCount() const final;
  std::shared_ptr<Connection> connection(unsigned index) final;
  std::shared_ptr<const Connection> connection(unsigned index) const final;

  /// Updates the internal connections list to the given one.
  /// Intended only for use by the @c ConnectionMonitor.
  void updateConnections(const std::vector<std::shared_ptr<Connection>> &connections,
                         const std::function<void(Server &, Connection &)> &callback);

private:
  mutable Lock _lock;
  std::vector<std::shared_ptr<Connection>> _connections;
  std::shared_ptr<TcpConnectionMonitor> _monitor;
  ServerSettings _settings;
  ServerInfoMessage _server_info;
  std::atomic_bool _active;
};
}  // namespace tes

#endif  // TES_CORE_PRIVATE_TCP_SERVER_H
