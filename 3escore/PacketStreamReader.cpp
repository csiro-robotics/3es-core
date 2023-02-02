//
// author: Kazys Stepanas
//
#include "PacketStreamReader.h"

#include "PacketReader.h"

#include <cstring>

namespace tes
{
PacketStreamReader::PacketStreamReader(std::shared_ptr<std::istream> stream)
  : _stream(std::exchange(stream, nullptr))
{
  auto packetMarker = networkEndianSwapValue(tes::kPacketMarker);
  std::memcpy(_markerBytes.data(), &packetMarker, sizeof(packetMarker));
  _buffer.reserve(_chunkSize);
}


PacketStreamReader::~PacketStreamReader() = default;


void PacketStreamReader::setStream(std::shared_ptr<std::istream> stream)
{
  std::swap(stream, _stream);
  _buffer.clear();
}


const PacketHeader *PacketStreamReader::extractPacket()
{
  if (!_stream || _stream->eof())
  {
    return nullptr;
  }

  consume();

  // Read a chunk from the stream.
  if (_buffer.size() < _chunkSize)
  {
    readMore(_chunkSize - _buffer.size());
  }

  // Scan for the buffer start.
  for (size_t i = 0; i < _buffer.size(); ++i)
  {
    if (checkMarker(_buffer, i))
    {
      // Marker found. Shift down to comsume trash at the start of the buffer.
      if (i > 0)
      {
        std::memmove(_buffer.data(), _buffer.data() + i, _buffer.size() - i);
        _buffer.resize(_buffer.size() - i);
      }

      if (_buffer.size() < sizeof(PacketHeader))
      {
        readMore(sizeof(PacketHeader) - _buffer.size());
        if (_buffer.size() < sizeof(PacketHeader))
        {
          // Can't read sufficient bytes. Abort.
          return nullptr;
        }
      }

      // Check the packet size and work out how much more to read.
      const auto targetSize = calcExpectedSize();
      // Read the full payload.
      if (_buffer.size() < targetSize)
      {
        readMore(targetSize - _buffer.size());
        if (_buffer.size() < targetSize)
        {
          // Failed to read enough.
          return nullptr;
        }
      }

      // We have our packet.
      // Mark to consume on next call.
      return reinterpret_cast<const PacketHeader *>(_buffer.data());
    }
  }

  return nullptr;
}


void PacketStreamReader::seek(std::istream::pos_type position)
{
  _buffer.clear();
  if (_stream)
  {
    _stream->clear();
    _stream->seekg(position);
  }
}


size_t PacketStreamReader::readMore(size_t moreCount)
{
  static_assert(sizeof(*_buffer.data()) == sizeof(char));
  auto haveCount = _buffer.size();
  _buffer.resize(haveCount + moreCount);
  _stream->read(reinterpret_cast<char *>(_buffer.data()) + haveCount, moreCount);
  auto readCount = _stream->gcount();
  _buffer.resize(haveCount + readCount);
  return readCount;
}


bool PacketStreamReader::checkMarker(std::vector<uint8_t> &buffer, size_t i)
{
  if (_buffer[i] == _markerBytes[0])
  {
    for (unsigned j = 1; j < unsigned(_markerBytes.size()) && i + j < buffer.size(); ++j)
    {
      if (_buffer[i + j] != _markerBytes[j])
      {
        return false;
      }
    }
  }
  return true;
}


void PacketStreamReader::consume()
{
  if (_buffer.size() < sizeof(PacketHeader))
  {
    // Not possible. Too small.
    return;
  }

  if (!checkMarker(_buffer, 0))
  {
    // Not at a valid packet.
    return;
  }

  auto targetSize = calcExpectedSize();
  if (_buffer.size() >= targetSize)
  {
    // Consume.
    std::memmove(_buffer.data(), _buffer.data() + targetSize, _buffer.size() - targetSize);
    _buffer.resize(_buffer.size() - targetSize);
  }
}

size_t PacketStreamReader::calcExpectedSize()
{
  const PacketHeader *header = reinterpret_cast<const PacketHeader *>(_buffer.data());
  auto payloadSize = networkEndianSwapValue(header->payload_size);
  if ((networkEndianSwapValue(header->flags) & PFNoCrc) == 0)
  {
    payloadSize += sizeof(PacketReader::CrcType);
  }
  return sizeof(PacketHeader) + payloadSize;
}
}  // namespace tes
