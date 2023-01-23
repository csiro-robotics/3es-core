//
// author: Kazys Stepanas
//
#ifndef _3ESSERVER_H_
#define _3ESSERVER_H_

#include "3es-core.h"

#include "3escompressionlevel.h"
#include "3esconnection.h"

#include <cstdint>

namespace tes
{
class CollatedPacket;
class Connection;
class ConnectionMonitor;
class PacketWriter;
class Shape;
struct ServerInfoMessage;

/// Server option flags.
enum ServerFlag
{
  /// Send frame update messages uncompressed and uncollated.
  /// This can be used to clearly demarcate frame boundaries without the need to decode
  /// collated and/or compressed data.
  SF_NakedFrameMessage = (1 << 0),
  /// Set to collate outgoing messages into larger packets.
  SF_Collate = (1 << 1),
  /// Set to compress collated outgoing packets using GZip compression.
  /// Has no effect if @c SF_Collate is not set or if the library is not built against ZLib.
  SF_Compress = (1 << 2),

  /// The combination of @c SF_Collate and @c SF_Compress
  SF_CollateAndCompress = SF_Collate | SF_Compress,
  /// The default recommended flags for initialising the server.
  /// This includes collation, compression and naked frame messages.
  SF_Default = SF_NakedFrameMessage | SF_Collate,
  /// The default recommended flags without compression.
  /// This includes collation, compression and naked frame messages.
  SF_DefaultNoCompression = (SF_Default & ~SF_Compress),
};

/// Settings used to create the server.
struct _3es_coreAPI ServerSettings
{
  /// First port to try listening on.
  uint16_t listenPort = 33500u;
  /// Additional number of ports the server may try listening on.
  uint16_t portRange = 0;
  /// @c ServerFlag values.
  unsigned flags = SF_Default;
  /// Timeout used to wait for the connection monitor to start (milliseconds). Only for asynchronous mode.
  unsigned asyncTimeoutMs = 5000u;
  /// Size of the client packet buffers.
  uint16_t clientBufferSize = 0xffe0u;
  /// Compression level to use if enabled. See @c CompressionLevel.
  uint16_t compressionLevel = CL_Default;

  ServerSettings() = default;
  inline ServerSettings(unsigned flags, uint16_t port = 33500u, uint16_t clientBufferSize = 0xffe0u,
                        CompressionLevel compressionLevel = CL_Default)
    : listenPort(port)
    , flags(flags)
    , clientBufferSize(clientBufferSize)
    , compressionLevel(compressionLevel)
  {}

  // TODO: Allowed client IPs.
};

/// Defines the interface for managing a 3es server.
///
/// Listening must be initiated via the @c Server object's @c ConnectionMonitor,
/// available via @c connectionMonitor(). See that class's comments for
/// details of synchronous and asynchronous operation. The monitor
/// will be null if connections are not supported (generally internal only).
class _3es_coreAPI Server : public Connection
{
public:
  /// Creates a server with the given settings.
  ///
  /// The @p settings affect the local server state, while @p serverInfo describes
  /// the server to newly connected clients (first message sent). The @p serverInfo
  /// may be omitted to use the defaults.
  ///
  /// @param settings The local server settings.
  /// @param serverInfo Server settings published to clients. Null to use the defaults.
  static Server *create(const ServerSettings &settings = ServerSettings(),
                        const ServerInfoMessage *serverInfo = nullptr);

  /// Destroys the server this method is called on. This ensures correct clean up.
  virtual void dispose() = 0;

protected:
  /// Hidden virtual destructor.
  virtual ~Server() {}

public:
  /// Retrieve the @c ServerFlag set with which the server was created.
  virtual unsigned flags() const = 0;

  //---------------------
  // Connection methods.
  //---------------------

  using Connection::create;
  using Connection::send;

  /// Set a completed packet to all clients.
  ///
  /// The @p packet must be finalised first.
  ///
  /// @param packet The packet to send.
  /// @param allowCollation True to allow the message to be collated (and compressed) with other messages.
  virtual int send(const PacketWriter &packet, bool allowCollation = true) = 0;

  /// Send a collated packet to all clients.
  ///
  /// This supports sending collections of packets as a single send operation
  /// while maintaining thread safety.
  ///
  /// The collated packet may be larger than the normal send limit as collated
  /// message is extracted and sent individually. To support this, compression on
  /// @p collated is not supported.
  ///
  /// @par Note sending in this way bypasses the shape and resource caches and
  /// can only work when the user maintains state.
  ///
  /// @param collated Collated packets to send. Compression is not supported.
  virtual int send(const CollatedPacket &collated) = 0;

  /// Returns the connection monitor object for this @c Server.
  /// Null if connections are not supported (internal only).
  virtual ConnectionMonitor *connectionMonitor() = 0;

  /// Returns the number of current connections.
  /// @return The current number of connections.
  virtual unsigned connectionCount() const = 0;

  /// Requests the connection at the given index.
  ///
  /// This data may be stale if the @c ConnectionMonitor has yet to update.
  /// @param index The index of the requested connection.
  /// @return The requested connection, or null if @p index is out of range.
  virtual Connection *connection(unsigned index) = 0;

  /// @overload
  virtual const Connection *connection(unsigned index) const = 0;
};
}  // namespace tes

#endif  // _3ESSERVER_H_
