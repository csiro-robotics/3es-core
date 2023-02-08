//
// author: Kazys Stepanas
//
#ifndef TES_CORE_TCP_SOCKET_H
#define TES_CORE_TCP_SOCKET_H

#include "CoreConfig.h"

#include <cinttypes>
#include <cstddef>
#include <memory>

namespace tes
{
struct TcpSocketDetail;

/// A TCP/IP communication socket implementation.
class TES_CORE_API TcpSocket
{
public:
  /// Value used to signify an indefinite timeout.
  static const unsigned IndefiniteTimeout;

  /// Constructor.
  TcpSocket();
  /// Constructor.
  TcpSocket(std::unique_ptr<TcpSocketDetail> &&detail);
  /// Destructor.
  ~TcpSocket();

  /// Open a connection to the target @p host and @p port (blocking).
  /// @param host The host IP address or host name.
  /// @param port The target port.
  /// @return True if the connection was successfully established before timing out.
  bool open(const char *host, uint16_t port);

  /// Close the socket connection (no custom messages sent). Safe to call when not open.
  void close();

  /// Checks the connected state.
  /// @bug This is not reliable for a client socket. It only works when
  /// the @c TcpSocket was created from a @c TcpListenSocket.
  /// @return @c true if the socket is currently connected.
  [[nodiscard]] bool isConnected() const;

  /// Disable Nagle's algorithm, effectively disabling send delays?
  /// @param no_delay Disable the delay?
  void setNoDelay(bool no_delay);

  /// Check if Nagle's algorithm is disable.
  /// @return True if Nagle's algorithm is disabled.
  [[nodiscard]] bool noDelay() const;

  /// Sets the blocking timeout on calls to @c read().
  /// All calls to @c read() either until there are data
  /// available or this time elapses. Set to zero for non-blocking
  /// Use @c setIndefiniteReadTimeout() to block undefinately.
  /// @c read() calls.
  /// @param timeout_ms Read timeout in milliseconds.
  void setReadTimeout(unsigned timeout_ms);

  /// Returns the read timeout.
  /// @return The current read timeout in milliseconds. A value of
  ///   @c IndefiniteTimeout indicates indefinite blocking.
  [[nodiscard]] unsigned readTimeout() const;

  /// Clears the timeout for @c read() calls, blocking indefinitely
  /// until data are available.
  void setIndefiniteReadTimeout();

  /// Sets the blocking timeout on calls to @c write().
  /// This behaves in the same way as @c setReadTimeout(),
  /// except that it relates to @c write() calls.
  /// @param timeout_ms Read timeout in milliseconds.
  void setWriteTimeout(unsigned timeout_ms);

  /// Returns the write timeout.
  /// @return The current write timeout in milliseconds. A value of
  ///   @c IndefiniteTimeout indicates indefinite blocking.
  [[nodiscard]] unsigned writeTimeout() const;

  /// Clears the timeout for @c read() calls, blocking indefinitely
  /// until data have been sent.
  void setIndefiniteWriteTimeout();

  /// Sets the read buffer size (bytes).
  /// @param buffer_size The new buffer size. Max is 2^16 - 1.
  void setReadBufferSize(int buffer_size);

  /// Gets the current read buffer size (bytes).
  [[nodiscard]] int readBufferSize() const;

  /// Sets the send buffer size (bytes).
  /// @param buffer_size The new buffer size. Max is 2^16 - 1.
  void setSendBufferSize(int buffer_size);

  /// Gets the current send buffer size (bytes).
  [[nodiscard]] int sendBufferSize() const;

  /// Attempts to read data from the socket until the buffer is full.
  /// This may block. The blocking time may vary, but it will only block
  /// for at least the read timeout value so long as there is no activity.
  ///
  /// @param buffer The data buffer to read into.
  /// @param buffer_length The maximum number of types to read into @p buffer.
  /// @return The number of bytes read, which may be less than @p buffer_length, or -1 on error.
  [[nodiscard]] int read(char *buffer, int buffer_length) const;

  /// @overload
  [[nodiscard]] inline int read(unsigned char *buffer, int buffer_length) const
  {
    return read(reinterpret_cast<char *>(buffer), buffer_length);
  }

  /// Reads available data from the socket, returning immediately if there
  /// are no data available.
  /// @param buffer The data buffer to read into.
  /// @param buffer_length The maximum number of types to read into @p buffer.
  /// @return The number of bytes read, or -1 on error.
  [[nodiscard]] int readAvailable(char *buffer, int buffer_length) const;

  /// @overload
  [[nodiscard]] inline int readAvailable(unsigned char *buffer, int buffer_length) const
  {
    return readAvailable(reinterpret_cast<char *>(buffer), buffer_length);
  }

  /// Attempts to write data from the socket. This may block for the set write
  /// timeout.
  /// @param buffer The data buffer to send.
  /// @param buffer_length The number of bytes to send.
  /// @return The number of bytes sent, or -1 on error.
  int write(const char *buffer, int buffer_length) const;

  /// @overload
  inline int write(const unsigned char *buffer, int buffer_length) const
  {
    return write(reinterpret_cast<const char *>(buffer), buffer_length);
  }

  [[nodiscard]] uint16_t port() const;

private:
  std::unique_ptr<TcpSocketDetail> _detail;  ///< Implementation detail.
};

using TcpSocketPtr = std::shared_ptr<TcpSocket>;
}  // namespace tes

#endif  // TES_CORE_TCP_SOCKET_H
