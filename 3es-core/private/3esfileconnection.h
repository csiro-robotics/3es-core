//
// author: Kazys Stepanas
//
#ifndef _3ESFILECONNECTION_H_
#define _3ESFILECONNECTION_H_

#include "../3esserver.h"

#include "3esbaseconnection.h"

#include <fstream>
#include <string>

namespace tes
{
  class FileConnection : public BaseConnection
  {
  public:
    /// Create a new connection using the given @p clientSocket.
    /// @param filename Path to the file to write to.
    /// @param settings Various server settings to initialise with.
    FileConnection(const char *filename, const ServerSettings &settings);
    ~FileConnection();

    /// Close the socket connection.
    void close() override;

    const char *filename() const;

    /// Aliases filename()
    const char *address() const override;
    uint16_t port() const override;
    bool isConnected() const override;

    bool sendServerInfo(const ServerInfoMessage &info) override;

    int updateFrame(float dt, bool flush = true) override;

  protected:
    int writeBytes(const uint8_t *data, int byteCount) override;

  private:
    mutable Lock _fileLock; ///< Lock for @c _outFile() operations
    std::fstream _outFile;
    std::string _filename;
    unsigned _frameCount = 0;
  };
}

#endif // _3ESFILECONNECTION_H_
