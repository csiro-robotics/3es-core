//
// author: Kazys Stepanas
//
#ifndef _3ESBASECONNECTION_H_
#define _3ESBASECONNECTION_H_

#include "../3esserver.h"

#include <3esconnection.h>
#include <3esmessages.h>
#include <3espacketwriter.h>
#include <3esspinlock.h>

#include <atomic>
#include <list>
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

  /// Common @c Connection implementation base. Implements conversion of @c Shape messages into raw byte @c send()
  /// calls reducing the required subclass implentations to @c writeBytes().
  class BaseConnection : public Connection
  {
  public:
    typedef SpinLock Lock;

    /// Create a new connection using the given @p clientSocket.
    /// @param clientSocket The socket to communicate on.
    /// @param settings Various server settings to initialise with.
    BaseConnection(const ServerSettings &settings);
    ~BaseConnection();

    /// Activate/deactivate the connection. Messages are ignored while inactive.
    /// @param enable The active state to set.
    void setActive(bool enable) override;

    /// Check if currently active.
    /// @return True while active.
    bool active() const override;

    bool sendServerInfo(const ServerInfoMessage &info) override;

    int send(const PacketWriter &packet, bool allowCollation) override;
    int send(const CollatedPacket &collated);// override;
    int send(const uint8_t *data, int byteCount, bool allowCollation = true) override;

    int create(const Shape &shape) override;
    int destroy(const Shape &shape) override;
    int update(const Shape &shape) override;

    int updateTransfers(unsigned byteLimit) override;
    int updateFrame(float dt, bool flush = true) override;

    unsigned referenceResource(const Resource *resource) override;
    unsigned releaseResource(const Resource *resource) override;

  protected:
    virtual int writeBytes(const uint8_t *data, int byteCount) = 0;

    struct ResourceInfo
    {
      const Resource *resource;
      unsigned referenceCount;
      bool started; ///< Started sending?
      bool sent;  ///< Completed sending?

      inline ResourceInfo() : resource(nullptr), referenceCount(0), started(false), sent(false) {}
      inline ResourceInfo(const Resource *resource) : resource(resource), referenceCount(1), started(false), sent(false) {}
    };

    /// Decrement references count to the indicated @c resourceId, removing if necessary.
    unsigned releaseResource(uint64_t resourceId);

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
    /// @param byteCount Number of bytes from @p buffer to send.
    /// @param True to allow collation and compression for this packet.
    int writePacket(const uint8_t *buffer, uint16_t byteCount, bool allowCollation);

    void ensurePacketBufferCapacity(size_t size);

    Lock _packetLock; ///< Lock for using @c _packet
    Lock _sendLock;   ///< Lock for @c writePacket() and @c flushCollatedPacket()
    PacketWriter *_packet;
    std::vector<uint8_t> _packetBuffer;
    ResourcePacker *_currentResource; ///< Current resource being transmitted.
    std::list<uint64_t> _resourceQueue;
    std::unordered_map<uint64_t, ResourceInfo> _resources;
    ServerInfoMessage _serverInfo;
    float _secondsToTimeUnit;
    unsigned _serverFlags;
    CollatedPacket *_collation;
    std::atomic_bool _active;
  };
}

#endif // _3ESBASECONNECTION_H_
