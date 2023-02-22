//
// author: Kazys Stepanas
//
#ifndef TES_CORE_PRIVATE_BASE_CONNECTION_H
#define TES_CORE_PRIVATE_BASE_CONNECTION_H

#include "../Server.h"

#include <3escore/Connection.h>
#include <3escore/Messages.h>
#include <3escore/PacketWriter.h>

#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace tes
{
class CollatedPacket;
class Resource;
class ResourcePacker;
class TcpSocket;


// Resource management:
// - Reference count resources.
// - Track active transmission item
// - Send all parts for a shape at a time.

/// Common @c Connection implementation base. Implements conversion of @c Shape messages into raw
/// byte @c send() calls reducing the required subclass implementations to @c writeBytes().
class BaseConnection : public Connection
{
public:
  using Lock = std::mutex;

  /// Create a new connection using the given @p clientSocket.
  /// @param clientSocket The socket to communicate on.
  /// @param settings Various server settings to initialise with.
  BaseConnection(const ServerSettings &settings);
  ~BaseConnection() override;

  /// Activate/deactivate the connection. Messages are ignored while inactive.
  /// @param enable The active state to set.
  void setActive(bool enable) override;

  /// Check if currently active.
  /// @return True while active.
  bool active() const override;

  bool sendServerInfo(const ServerInfoMessage &info) override;

  int send(const PacketWriter &packet, bool allow_collation) override;
  int send(const CollatedPacket &collated) override;
  int send(const uint8_t *data, int byte_count, bool allow_collation) override;
  using Connection::send;

  int create(const Shape &shape) override;
  int destroy(const Shape &shape) override;
  int update(const Shape &shape) override;

  int updateTransfers(unsigned byte_limit) override;
  int updateFrame(float dt, bool flush) override;
  using Connection::updateFrame;

  unsigned referenceResource(const ResourcePtr &resource) override;
  unsigned releaseResource(const ResourcePtr &resource) override;

protected:
  virtual int writeBytes(const uint8_t *data, int byte_count) = 0;

  /// Internal structure for managing a resource.
  struct ResourceInfo
  {
    ResourcePtr resource;  ///< Resource pointer.
    /// Number of active references. This increases when @c referenceResource() is called and
    /// decreases when @c releaseResource() is called. It also changes as non-transient shapes
    /// with resources are created and destroyed.
    unsigned reference_count = 0;
    bool started = false;  ///< Started sending?
    bool sent = false;     ///< Completed sending?

    ResourceInfo() = default;
    ResourceInfo(ResourcePtr resource)
      : resource(std::move(resource))
      , reference_count(1)
    {}
  };

  /// Decrement references count to the indicated @c resource_id, removing if necessary.
  ///
  /// @note The @c _packet_lock must be locked before calling this function.
  unsigned releaseResource(uint64_t resource_id);

  /// Package and send @c DataMessage packets for @p shape assuming @p shape.isComplex().
  ///
  /// This sends all the data messages for @p shape. Such messages should only be sent if the
  /// shape is complex (see @c Shape::isComplex() ) and this function assumes that has been checked
  /// (assertion used if compile time enabled).
  ///
  /// @param shape The shape to send @c DataMessage packets for.
  /// @return The number of bytes written on success (possibly zero), -1 on failure.
  int sendShapeData(const Shape &shape);

  /// Queue sending resources for @p shape .
  ///
  /// Uses @c Shape::enumerateResources() to collect resources for sending and adds them to the
  /// queue. Note that transient objects - @c Shape::isTransient() - cannot have resources queued
  /// as they do not persist long enough for resource book keeping. However, they can use existing
  /// resources.
  ///
  /// If a transient @p shape is given, then this function checks that the shapes resource are
  /// present in the resource set. A warning message is logged for each missing resource.
  ///
  /// This assumes @c Shape::skipResources() has already been checked.
  ///
  /// @param shape The shape to queue resources for.
  /// @return The number of resources queued.
  unsigned queueResources(const Shape &shape);

  /// Check if the resources for @p shape are present in the resource set logging warnings for
  /// missing resources.
  /// @param shape The shape to check for.
  /// @return True if all resources are present in the known resource set for this connection.
  bool checkResources(const Shape &shape);

  /// Send pending collated/compressed data.
  ///
  /// Note: the @c _lock must be locked before calling this function.
  void flushCollatedPacket();

  /// Send pending collated/compressed data without using the threadding guard.
  void flushCollatedPacketUnguarded();

  /// Write data to the client. Handles collation and compression if enabled.
  ///
  /// Note: the @c _lock must be locked before calling this function.
  /// @param buffer The data buffer to send from.
  /// @param byte_count Number of bytes from @p buffer to send.
  /// @param True to allow collation and compression for this packet.
  int writePacket(const uint8_t *buffer, uint16_t byte_count, bool allow_collation);

  void ensurePacketBufferCapacity(size_t size);

  Lock _packet_lock;    ///< Lock for using @c _packet
  Lock _send_lock;      ///< Lock for @c writePacket() and @c flushCollatedPacket()
  Lock _resource_lock;  ///< Lock for @c _resources
  std::unique_ptr<PacketWriter> _packet;
  std::vector<uint8_t> _packet_buffer;
  std::unique_ptr<ResourcePacker> _current_resource;  ///< Current resource being transmitted.
  std::list<uint64_t> _resource_queue;
  std::unordered_map<uint64_t, ResourceInfo> _resources;
  /// Buffer used when calling @c Shape::enumerateResources() . Use is transient.
  std::vector<ResourcePtr> _resource_buffer;
  ServerInfoMessage _server_info = {};
  float _seconds_to_time_unit = 0;
  unsigned _server_flags = 0;
  std::unique_ptr<CollatedPacket> _collation;
  std::atomic_bool _active = { true };
};
}  // namespace tes

#endif  // TES_CORE_PRIVATE_BASE_CONNECTION_H
