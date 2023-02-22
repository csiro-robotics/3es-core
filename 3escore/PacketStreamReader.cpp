//
// author: Kazys Stepanas
//
#include "PacketStreamReader.h"

#include "CoreUtil.h"
#include "PacketReader.h"

#include <cstring>

namespace tes
{
PacketStreamReader::PacketStreamReader()
{
  const auto packet_marker = networkEndianSwapValue(tes::kPacketMarker);
  std::memcpy(_marker_bytes.data(), &packet_marker, sizeof(packet_marker));
  _buffer.reserve(_chunk_size);
}

PacketStreamReader::PacketStreamReader(std::shared_ptr<std::istream> stream)
  : PacketStreamReader()
{
  _stream = std::move(stream);
}


PacketStreamReader::~PacketStreamReader() = default;


void PacketStreamReader::setStream(std::shared_ptr<std::istream> stream)
{
  std::swap(stream, _stream);
  _buffer.clear();
}


const PacketHeader *PacketStreamReader::extractPacket()
{
  if (!_stream)
  {
    return nullptr;
  }

  consume();

  // Read a chunk from the stream.
  if (_buffer.empty())
  {
    if (readMore(_chunk_size - _buffer.size()) == 0)
    {
      // We have no data to read more.
      return nullptr;
    }
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
      const auto target_size = calcExpectedSize();
      // Read the full payload.
      if (_buffer.size() < target_size)
      {
        readMore(target_size - _buffer.size());
        if (_buffer.size() < target_size)
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


size_t PacketStreamReader::readMore(size_t more_count)
{
  static_assert(sizeof(*_buffer.data()) == sizeof(char));
  if (isEof())
  {
    return 0;
  }

  auto have_count = _buffer.size();
  _buffer.resize(have_count + more_count);
  const auto read_count = _stream->readsome(reinterpret_cast<char *>(_buffer.data()) + have_count,
                                            int_cast<unsigned>(more_count));
  _buffer.resize(have_count + read_count);
  return read_count;
}


bool PacketStreamReader::checkMarker(std::vector<uint8_t> &buffer, size_t i)
{
  for (size_t j = 0; j < _marker_bytes.size() && i + j < buffer.size(); ++j)
  {
    if (_buffer[i + j] != _marker_bytes[j])
    {
      return false;
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

  auto target_size = calcExpectedSize();
  if (_buffer.size() >= target_size)
  {
    // Consume.
    std::memmove(_buffer.data(), _buffer.data() + target_size, _buffer.size() - target_size);
    _buffer.resize(_buffer.size() - target_size);
  }
}

size_t PacketStreamReader::calcExpectedSize()
{
  const auto *header = reinterpret_cast<const PacketHeader *>(_buffer.data());
  auto payload_size = networkEndianSwapValue(header->payload_size);
  if ((networkEndianSwapValue(header->flags) & PFNoCrc) == 0)
  {
    payload_size += sizeof(PacketReader::CrcType);
  }
  return sizeof(PacketHeader) + payload_size;
}
}  // namespace tes
