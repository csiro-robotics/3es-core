//
// author: Kazys Stepanas
//
#include "ResourcePacker.h"

#include "PacketWriter.h"
#include "Resource.h"
#include "TransferProgress.h"

#include <algorithm>
#include <cstdio>

namespace tes
{
ResourcePacker::ResourcePacker()
  : _progress(std::make_unique<TransferProgress>())
{
  _progress->reset();
}


ResourcePacker::~ResourcePacker()
{
  cancel();
}


void ResourcePacker::transfer(const Resource *resource)
{
  cancel();
  _resource = resource;
}


void ResourcePacker::cancel()
{
  _progress->reset();
  _resource = nullptr;
  _started = false;
}


bool ResourcePacker::nextPacket(PacketWriter &packet, unsigned byte_limit)
{
  if (!_resource)
  {
    return false;
  }

  if (!_started)
  {
    _resource->create(packet);
    _started = true;
    return true;
  }

  if (_resource->transfer(packet, byte_limit, *_progress) != 0)
  {
    _progress->failed = true;
    _resource = nullptr;
    _progress->reset();
    return false;
  }

  if (_progress->complete || _progress->failed)
  {
    _last_completed_id = _resource->uniqueKey();
    _resource = nullptr;
    _progress->reset();
  }

  return true;
}
}  // namespace tes
