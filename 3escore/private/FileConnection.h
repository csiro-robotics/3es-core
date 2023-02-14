//
// author: Kazys Stepanas
//
#ifndef TES_CORE_PRIVATE_FILE_CONNECTION_H
#define TES_CORE_PRIVATE_FILE_CONNECTION_H

#include "../Server.h"

#include "BaseConnection.h"

#include <fstream>
#include <string>

namespace tes
{
/// A file stream implementation of a 3es @c Connection.
class FileConnection final : public BaseConnection
{
public:
  /// Create a new connection using the given @p clientSocket.
  /// @param filename Path to the file to write to.
  /// @param settings Various server settings to initialise with.
  FileConnection(const std::string &filename, const ServerSettings &settings);

  // See cpp file for details on disabling bugprone-exception-escape
  ~FileConnection() final;  // NOLINT(bugprone-exception-escape)

  /// Close the socket connection.
  void close() final;

  const char *filename() const;

  /// Aliases filename()
  const char *address() const override;
  uint16_t port() const override;
  bool isConnected() const override;

  bool sendServerInfo(const ServerInfoMessage &info) final;

  int updateFrame(float dt, bool flush) final;

protected:
  int writeBytes(const uint8_t *data, int byte_count) final;

private:
  mutable Lock _file_lock;  ///< Lock for @c _out_file() operations
  std::fstream _out_file;
  std::string _filename;
  unsigned _frame_count = 0;
};
}  // namespace tes

#endif  // TES_CORE_PRIVATE_FILE_CONNECTION_H
