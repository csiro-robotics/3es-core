//
// author: Kazys Stepanas
//
#ifndef TES_CORE_CONNECTION_H
#define TES_CORE_CONNECTION_H

#include "CoreConfig.h"

#include <cstddef>
#include <cstdint>

namespace tes
{
class PacketWriter;
class Resource;
class Shape;
struct ServerInfoMessage;

/// Defines the interfaces for a client connection.
class TES_CORE_API Connection
{
public:
  /// Virtual destructor.
  virtual ~Connection() = default;

  /// Close the socket connection.
  virtual void close() = 0;

  /// Activate/deactivate the connection. Messages are ignored while inactive.
  /// @param active The active state to set.
  virtual void setActive(bool active) = 0;

  /// Check if currently active.
  /// @return True while active.
  [[nodiscard]] virtual bool active() const = 0;

  /// Address string for the connection. The string depends on
  /// the connection type.
  /// @return The connection end point address.
  [[nodiscard]] virtual const char *address() const = 0;
  /// Get the connection port.
  /// @return The connection end point port.
  [[nodiscard]] virtual uint16_t port() const = 0;
  /// Is the connection active and valid?
  /// @return True while connected.
  [[nodiscard]] virtual bool isConnected() const = 0;

  /// Sends a create message for the given shape.
  /// @param shape The shape details.
  /// @return The number of bytes queued for transfer for this message, or negative on error.
  ///   The negative value may be less than -1 and still indicate the successful transfer size.
  virtual int create(const Shape &shape) = 0;

  /// Sends a destroy message for the given shape.
  /// @param shape The shape details.
  /// @return The number of bytes queued for transfer for this message, or negative on error.
  ///   The negative value may be less than -1 and still indicate the successful transfer size.
  virtual int destroy(const Shape &shape) = 0;

  /// Sends an update message for the given shape.
  /// @param shape The shape details.
  /// @return The number of bytes queued for transfer for this message, or negative on error.
  ///   The negative value may be less than -1 and still indicate the successful transfer size.
  virtual int update(const Shape &shape) = 0;

  /// Sends a message marking the end of the current frame (and start of a new frame).
  ///
  /// @param dt Indicates the time passed since over this frame (seconds).
  /// @param flush True to allow clients to flush transient objects, false to instruct clients
  ///   to preserve such objects.
  /// @return The number of bytes queued for transfer for this message, or negative on error.
  ///   The negative value may be less than -1 and still indicate the successful transfer size.
  virtual int updateFrame(float dt, bool flush) = 0;

  /// @overload
  int updateFrame(float dt) { return updateFrame(dt, true); }

  /// Update any pending resource transfers (e.g., mesh transfer).
  ///
  /// Transfer may be amortised by setting a @c byte_limit or enforced by a zero byte limit.
  /// Zero guarantees all outstanding resources are transferred.
  ///
  /// This method should generally be called once for every @c updateFrame(), normally
  /// before the frame update. This holds especially true when not amortising transfer (zero byte
  /// limit).
  ///
  /// @param byte_limit Limit the packet payload size to approximately this
  /// amount of data.
  /// @return The number of bytes queued for transfer for this message, or negative on error.
  ///   The negative value may be less than -1 and still indicate the successful transfer size.
  virtual int updateTransfers(unsigned byte_limit) = 0;

  /// Add a resource to this connection.
  ///
  /// The resource is either added with a reference count of 1, or the resource
  /// reference count is incremented. The @p resource pointer must remain valid until
  /// the reference count returns to zero. A newly added resource is pushed into the
  /// resource queue for transfer.
  ///
  /// @param resource The resource to reference.
  /// @return The resource reference count after adjustment.
  virtual unsigned referenceResource(const Resource *resource) = 0;

  /// Release a resource within this connection.
  ///
  /// If found, the resource has its reference count reduced. A destroy message is sent for
  /// the resource if the count becomes zero.
  ///
  /// @param resource The resource to release.
  /// @return The resource reference count after adjustment.
  virtual unsigned releaseResource(const Resource *resource) = 0;

  /// Send server details to the client.
  virtual bool sendServerInfo(const ServerInfoMessage &info) = 0;

  /// Send data from a @c PacketWriter. PacketWriter::finalise() must have already been called.
  /// @param packet The packet to send.
  /// @param allow_collation True to allow the message to be collated (and compressed) with other
  /// messages.
  virtual int send(const PacketWriter &packet, bool allow_collation) = 0;

  /// @overload
  int send(const PacketWriter &packet) { return send(packet, true); }

  /// Send pre-prepared message data to all connections.
  /// @param data Data buffer to send.
  /// @param byte_count Number of bytes to send.
  /// @param allow_collation True to allow the message to be collated (and compressed) with other
  /// messages.
  virtual int send(const uint8_t *data, int byte_count, bool allow_collation) = 0;

  /// @overload
  int send(const uint8_t *data, int byte_count) { return send(data, byte_count, true); }

  /// @overload
  int send(const int8_t *data, int byte_count, bool allow_collation = true)
  {
    return send(reinterpret_cast<const uint8_t *>(data), byte_count, allow_collation);
  }
};
}  // namespace tes

#endif  // TES_CORE_CONNECTION_H
