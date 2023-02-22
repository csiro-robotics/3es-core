//
// author: Kazys Stepanas
//
#include "TcpServer.h"

#include "TcpConnection.h"
#include "TcpConnectionMonitor.h"

#include <3escore/PacketWriter.h>

#include <algorithm>
#include <mutex>

namespace tes
{
std::shared_ptr<Server> Server::create(const ServerSettings &settings,
                                       const ServerInfoMessage *server_info)
{
  return std::make_shared<TcpServer>(settings, server_info);
}


TcpServer::TcpServer(const ServerSettings &settings, const ServerInfoMessage *server_info)
  : _monitor(nullptr)
  , _settings(settings)
  , _active(true)
{
  _monitor = std::make_shared<TcpConnectionMonitor>(*this);

  if (server_info)
  {
    std::memcpy(&_server_info, server_info, sizeof(_server_info));
  }
  else
  {
    initDefaultServerInfo(&_server_info);
  }
}


TcpServer::~TcpServer() = default;


unsigned TcpServer::flags() const
{
  return _settings.flags;
}


void TcpServer::close()
{
  _monitor->stop();
  _monitor->join();

  const std::lock_guard<Lock> guard(_lock);

  for (const auto &con : _connections)
  {
    con->close();
  }
}


void TcpServer::setActive(bool enable)
{
  _active = enable;
}


bool TcpServer::active() const
{
  return _active;
}


const char *TcpServer::address() const
{
  return "TcpServer";
}


uint16_t TcpServer::port() const
{
  return _monitor->mode() != ConnectionMode::None ? _settings.listen_port : 0;
}


bool TcpServer::isConnected() const
{
  const std::lock_guard<Lock> guard(_lock);
  return !_connections.empty();
}


int TcpServer::create(const Shape &shape)
{
  if (!_active)
  {
    return 0;
  }

  const std::lock_guard<Lock> guard(_lock);
  int transferred = 0;
  bool error = false;
  for (const auto &con : _connections)
  {
    const int txc = con->create(shape);
    if (txc >= 0)
    {
      transferred += txc;
    }
    else
    {
      error = true;
    }
  }

  return (!error) ? transferred : -transferred;
}


int TcpServer::destroy(const Shape &shape)
{
  if (!_active)
  {
    return 0;
  }

  const std::lock_guard<Lock> guard(_lock);
  int transferred = 0;
  bool error = false;
  for (const auto &con : _connections)
  {
    const int txc = con->destroy(shape);
    if (txc >= 0)
    {
      transferred += txc;
    }
    else
    {
      error = true;
    }
  }

  return (!error) ? transferred : -transferred;
}


int TcpServer::update(const Shape &shape)
{
  if (!_active)
  {
    return 0;
  }

  const std::lock_guard<Lock> guard(_lock);
  int transferred = 0;
  bool error = false;
  for (const auto &con : _connections)
  {
    const int txc = con->update(shape);
    if (txc >= 0)
    {
      transferred += txc;
    }
    else
    {
      error = true;
    }
  }

  return (!error) ? transferred : -transferred;
}


int TcpServer::updateFrame(float dt, bool flush)
{
  if (!_active)
  {
    return 0;
  }

  std::unique_lock<Lock> guard(_lock);
  int transferred = 0;
  bool error = false;
  for (const auto &con : _connections)
  {
    const int txc = con->updateFrame(dt, flush);
    if (txc >= 0)
    {
      transferred += txc;
    }
    else
    {
      error = true;
    }
  }

  // Async mode: commit new connections after the current frame is sent.
  // We do it after a frame update to prevent doubling up on creation messages.
  // Consider this: the application code uses a callback on new connections
  // to create objects to reflect the current state, invoked when commitConnections()
  // is called. If we did this before the end of frame transfer, then we may
  // generate create messages in the callback for objects which have buffered
  // create messages. Alternatively, if the server is not in collated mode, the
  // we'll get different behaviour between collated and uncollated modes.
  guard.unlock();
  if (_monitor->mode() == ConnectionMode::Asynchronous)
  {
    _monitor->commitConnections();
  }

  return (!error) ? transferred : -transferred;
}


int TcpServer::updateTransfers(unsigned byte_limit)
{
  if (!_active)
  {
    return 0;
  }

  const std::lock_guard<Lock> guard(_lock);
  int transferred = 0;
  bool error = false;
  for (const auto &con : _connections)
  {
    const int txc = con->updateTransfers(byte_limit);
    if (txc >= 0)
    {
      transferred += txc;
    }
    else
    {
      error = true;
    }
  }

  return (!error) ? transferred : -transferred;
}


bool TcpServer::sendServerInfo(const ServerInfoMessage &info)
{
  TES_UNUSED(info);
  return false;
}


unsigned TcpServer::referenceResource(const ResourcePtr &resource)
{
  if (!_active)
  {
    return 0;
  }

  const std::lock_guard<Lock> guard(_lock);
  unsigned last_count = 0;
  for (const auto &con : _connections)
  {
    last_count = con->referenceResource(resource);
  }
  return last_count;
}


unsigned TcpServer::releaseResource(const ResourcePtr &resource)
{
  if (!_active)
  {
    return 0;
  }

  const std::lock_guard<Lock> guard(_lock);
  unsigned last_count = 0;
  for (const auto &con : _connections)
  {
    last_count = con->releaseResource(resource);
  }
  return last_count;
}


int TcpServer::send(const PacketWriter &packet, bool allow_collation)
{
  return send(packet.data(), packet.packetSize(), allow_collation);
}


int TcpServer::send(const CollatedPacket &collated)
{
  if (!_active)
  {
    return 0;
  }

  int sent = 0;
  bool failed = false;
  const std::lock_guard<Lock> guard(_lock);
  for (const auto &con : _connections)
  {
    sent = con->send(collated);
    if (sent == -1)
    {
      failed = true;
    }
  }

  return (!failed) ? sent : -1;
}


int TcpServer::send(const uint8_t *data, int byte_count, bool allow_collation)
{
  if (!_active)
  {
    return 0;
  }

  int sent = 0;
  bool failed = false;
  const std::lock_guard<Lock> guard(_lock);
  for (const auto &con : _connections)
  {
    sent = con->send(data, byte_count, allow_collation);
    if (sent == -1)
    {
      failed = true;
    }
  }

  return (!failed) ? sent : -1;
}


std::shared_ptr<ConnectionMonitor> TcpServer::connectionMonitor()
{
  return _monitor;
}


unsigned TcpServer::connectionCount() const
{
  const std::lock_guard<Lock> guard(_lock);
  return static_cast<unsigned>(_connections.size());
}


std::shared_ptr<Connection> TcpServer::connection(unsigned index)
{
  const std::lock_guard<Lock> guard(_lock);
  if (index < _connections.size())
  {
    return _connections[index];
  }
  return {};
}


std::shared_ptr<const Connection> TcpServer::connection(unsigned index) const
{
  const std::lock_guard<Lock> guard(_lock);
  if (index < _connections.size())
  {
    return _connections[index];
  }
  return {};
}


void TcpServer::updateConnections(const std::vector<std::shared_ptr<Connection>> &connections,
                                  const std::function<void(Server &, Connection &)> &callback)
{
  if (!_active)
  {
    return;
  }

  const std::lock_guard<Lock> guard(_lock);
  std::vector<std::shared_ptr<Connection>> new_connections;

  if (!connections.empty())
  {
    new_connections.reserve(32);
    for (const auto &con : connections)
    {
      bool existing = false;
      for (const auto &exist : _connections)
      {
        if (exist == con)
        {
          existing = true;
          break;
        }
      }

      if (!existing)
      {
        new_connections.push_back(con);
      }
    }
  }

  _connections.clear();
  std::for_each(connections.begin(), connections.end(),
                [this](const std::shared_ptr<Connection> &con) { _connections.push_back(con); });

  // Send server info to new connections.
  for (const auto &con : new_connections)
  {
    con->sendServerInfo(_server_info);
    if (callback)
    {
      (callback)(*this, *con);
    }
  }
}
}  // namespace tes
