//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SERVER_H
#define TES_CORE_SERVER_H

#include "CoreConfig.h"

#include "CompressionLevel.h"
#include "Connection.h"

#include <cstdint>
#include <memory>

namespace tes
{
class CollatedPacket;
class Connection;
class ConnectionMonitor;
class PacketWriter;
class Shape;
struct ServerInfoMessage;

/// Server option flags.
enum ServerFlag : uint32_t
{
  /// Send frame update messages uncompressed and uncollated.
  /// This can be used to clearly demarcate frame boundaries without the need to decode
  /// collated and/or compressed data.
  SFNakedFrameMessage = (1u << 0u),
  /// Set to collate outgoing messages into larger packets.
  SFCollate = (1u << 1u),
  /// Set to compress collated outgoing packets using GZip compression.
  /// Has no effect if @c SFCollate is not set or if the library is not built against ZLib.
  SFCompress = (1u << 2u),

  /// The combination of @c SFCollate and @c SFCompress
  SFCollateAndCompress = SFCollate | SFCompress,
  /// The default recommended flags for initialising the server.
  /// This includes collation, compression and naked frame messages.
  SFDefault = SFNakedFrameMessage | SFCollate,
  /// The default recommended flags without compression.
  /// This includes collation, compression and naked frame messages.
  SFDefaultNoCompression = (SFDefault & ~SFCompress),
};

/// Settings used to create the server.
struct TES_CORE_API ServerSettings
{
  /// Default server port.
  static constexpr uint16_t kDefaultPort = 33500u;
  /// Default server buffer size per client.
  static constexpr uint16_t kDefaultBufferSize = 0xffe0u;
  static constexpr uint32_t kDefaultAsyncTimeoutMs = 5000u;

  /// First port to try listening on.
  uint16_t listen_port = kDefaultPort;
  /// Additional number of ports the server may try listening on.
  uint16_t port_range = 0;
  /// @c ServerFlag values.
  uint32_t flags = SFDefault;
  /// Timeout used to wait for the connection monitor to start (milliseconds). Only for asynchronous
  /// mode.
  uint32_t async_timeout_ms = kDefaultAsyncTimeoutMs;
  /// Size of the client packet buffers.
  uint16_t client_buffer_size = kDefaultBufferSize;
  /// Compression level to use if enabled. See @c CompressionLevel.
  uint16_t compression_level = ClDefault;

  ServerSettings() = default;
  ServerSettings(uint32_t flags, uint16_t port = kDefaultPort,
                 uint16_t client_buffer_size = kDefaultBufferSize,
                 CompressionLevel compression_level = ClDefault)
    : listen_port(port)
    , flags(flags)
    , client_buffer_size(client_buffer_size)
    , compression_level(compression_level)
  {}

  // TODO(KS): Allowed client IPs.
};

/// Defines the interface for managing a 3es server.
///
/// Listening must be initiated via the @c Server object's @c ConnectionMonitor,
/// available via @c connectionMonitor(). See that class's comments for
/// details of synchronous and asynchronous operation. The monitor
/// will be null if connections are not supported (generally internal only).
class TES_CORE_API Server : public Connection
{
public:
  /// Creates a server with the given settings.
  ///
  /// The @p settings affect the local server state, while @p server_info describes
  /// the server to newly connected clients (first message sent). The @p server_info
  /// may be omitted to use the defaults.
  ///
  /// @param settings The local server settings.
  /// @param server_info Server settings published to clients. Null to use the defaults.
  static std::shared_ptr<Server> create(const ServerSettings &settings = ServerSettings(),
                                        const ServerInfoMessage *server_info = nullptr);

  /// Destructor.
  ~Server() override = default;

  using Connection::create;

  /// Retrieve the @c ServerFlag set with which the server was created.
  [[nodiscard]] virtual uint32_t flags() const = 0;

  /// Returns the connection monitor object for this @c Server.
  /// Null if connections are not supported (internal only).
  virtual std::shared_ptr<ConnectionMonitor> connectionMonitor() = 0;

  /// Returns the number of current connections.
  /// @return The current number of connections.
  [[nodiscard]] virtual unsigned connectionCount() const = 0;

  /// Requests the connection at the given index.
  ///
  /// This data may be stale if the @c ConnectionMonitor has yet to update.
  /// @param index The index of the requested connection.
  /// @return The requested connection, or null if @p index is out of range.
  virtual std::shared_ptr<Connection> connection(unsigned index) = 0;

  /// @overload
  [[nodiscard]] virtual std::shared_ptr<const Connection> connection(unsigned index) const = 0;
};
}  // namespace tes

#endif  // TES_CORE_SERVER_H
