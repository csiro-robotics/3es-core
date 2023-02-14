//
// Author Kazys Stepanas
#include "CollatedPacket.h"

#include "Connection.h"
#include "CoreUtil.h"
#include "Crc.h"
#include "Endian.h"
#include "Log.h"
#include "Maths.h"
#include "Messages.h"
#include "PacketWriter.h"
#include "Throw.h"

#include "private/CollatedPacketZip.h"

#include "shapes/Shape.h"

#include <algorithm>
#include <cstring>

namespace tes
{
namespace
{
void writeMessageHeader(uint8_t *buffer, unsigned uncompressed_size, unsigned payload_size,
                        bool compressed)
{
  auto *header = reinterpret_cast<PacketHeader *>(buffer);
  std::memset(header, 0, sizeof(PacketHeader));
  auto *message = reinterpret_cast<CollatedPacketMessage *>(buffer + sizeof(PacketHeader));
  std::memset(message, 0, sizeof(CollatedPacketMessage));

  // Keep header in network byte order.
  header->marker = networkEndianSwapValue(kPacketMarker);
  header->version_major = networkEndianSwapValue(kPacketVersionMajor);
  header->version_minor = networkEndianSwapValue(kPacketVersionMinor);
  header->routing_id = MtCollatedPacket;
  networkEndianSwap(header->routing_id);
  header->message_id = 0;
  header->payload_size = static_cast<uint16_t>(payload_size + sizeof(CollatedPacketMessage));
  networkEndianSwap(header->payload_size);
  header->payload_offset = 0;
  header->flags = 0;

  message->flags = (compressed) ? CPFCompress : 0;
  networkEndianSwap(message->flags);
  message->reserved = 0;
  message->uncompressed_bytes = uncompressed_size;
  networkEndianSwap(message->uncompressed_bytes);
}
}  // namespace

const size_t CollatedPacket::Overhead =
  sizeof(PacketHeader) + sizeof(CollatedPacketMessage) + sizeof(PacketWriter::CrcType);
const unsigned CollatedPacket::InitialCursorOffset =
  sizeof(PacketHeader) + sizeof(CollatedPacketMessage);


CollatedPacket::CollatedPacket(bool compress, uint16_t buffer_size)
{
  init(compress, buffer_size, kMaxPacketSize);
}


CollatedPacket::CollatedPacket(unsigned buffer_size, unsigned max_packet_size)
{
  init(false, buffer_size, max_packet_size);
}


CollatedPacket::~CollatedPacket() = default;


void CollatedPacket::setCompressionLevel(int level)
{
  if (ClNone <= level && level < ClLevels)
  {
    _compression_level = static_cast<uint16_t>(level);
  }
}


int CollatedPacket::compressionLevel() const
{
  return _compression_level;
}


void CollatedPacket::reset()
{
  _cursor = _final_packet_cursor = 0;
  _finalised = false;
}


int CollatedPacket::add(const PacketWriter &packet)
{
  if (!_active)
  {
    return 0;
  }

  const auto *packet_buffer = reinterpret_cast<const uint8_t *>(&packet.packet());
  const uint16_t packet_bytes = packet.packetSize();
  return add(packet_buffer, packet_bytes);
}


int CollatedPacket::add(const uint8_t *buffer, uint16_t byte_count)
{
  if (!_active)
  {
    return 0;
  }

  if (byte_count <= 0)
  {
    return 0;
  }

  if (_finalised)
  {
    return -1;
  }

  // Check total size capacity.
  if (collatedBytes() + byte_count + Overhead > _max_packet_size)
  {
    // Too many bytes to collate.
    return -1;
  }

  if (_buffer.size() < collatedBytes() + byte_count + Overhead)
  {
    // Buffer too small.
    expand(byte_count + Overhead, _buffer, _max_packet_size);
  }

  std::copy(buffer, buffer + byte_count, _buffer.begin() + _cursor);
  _cursor += byte_count;

  return byte_count;
}


bool CollatedPacket::finalise()
{
  if (!_active || _finalised)
  {
    return false;
  }

  if (collatedBytes() == 0)
  {
    _final_packet_cursor = 0;
    _finalised = true;
    return true;
  }

  _final_buffer.resize(_buffer.size() + Overhead);

  // Finalise the packet. If possible, we try compress the buffer. If that is smaller then we use
  // the compressed result. Otherwise we use compressed data.
  bool compressed_data = false;
#ifdef TES_ZLIB
  if (compressionEnabled() && collatedBytes())
  {
    unsigned compressed_bytes = 0;

    // Z_BEST_COMPRESSION
    // params: stream,level, method, window bits, memLevel, strategy
    const int gzip_compression_level = tes::kTesToGZipCompressionLevel[_compression_level];
    deflateInit2(&_zip->stream, gzip_compression_level, Z_DEFLATED,
                 // NOLINTNEXTLINE(hicpp-signed-bitwise)
                 CollatedPacketZip::WindowBits | CollatedPacketZip::GZipEncoding, 8,
                 Z_DEFAULT_STRATEGY);
    _zip->stream.next_out = reinterpret_cast<Bytef *>(_final_buffer.data() + InitialCursorOffset);
    _zip->stream.avail_out = static_cast<uInt>(_final_buffer.size() - Overhead);

    int zip_ret = 0;
    _zip->stream.avail_in = collatedBytes();
    _zip->stream.next_in = reinterpret_cast<Bytef *>(_buffer.data());
    zip_ret = deflate(&_zip->stream, Z_FINISH);
    deflateEnd(&_zip->stream);

    if (zip_ret == Z_STREAM_END)
    {
      // Compressed ok. Check size.
      // Update _cursor to reflect the number of bytes to write.
      compressed_bytes = static_cast<unsigned>(_zip->stream.total_out);
      _zip->stream.total_out = 0;

      if (compressed_bytes < collatedBytes())
      {
        // Compression is good. Smaller than uncompressed data.
        compressed_data = true;
        // Write uncompressed header.
        writeMessageHeader(_final_buffer.data(), collatedBytes(), compressed_bytes, true);
        _final_packet_cursor = InitialCursorOffset + compressed_bytes;
      }
      else
      {
        log::error("Compression failure. Collated ", collatedBytes(), " compressed to ",
                   compressed_bytes);
      }
    }
  }
#endif  // TES_ZLIB

  if (!compressed_data)
  {
    // No or failed compression. Write uncompressed.
    writeMessageHeader(_final_buffer.data(), collatedBytes(), collatedBytes(), false);
    std::memcpy(_final_buffer.data() + InitialCursorOffset, _buffer.data(), collatedBytes());
    _final_packet_cursor = InitialCursorOffset + collatedBytes();
  }

  // Calculate the CRC
  auto *crc_ptr =
    reinterpret_cast<PacketWriter::CrcType *>(_final_buffer.data() + _final_packet_cursor);
  *crc_ptr = crc16(_final_buffer.data(), _final_packet_cursor);
  networkEndianSwap(*crc_ptr);
  _final_packet_cursor += static_cast<unsigned>(sizeof(*crc_ptr));
  _finalised = true;
  return true;
}


const uint8_t *CollatedPacket::buffer(unsigned &byte_count) const
{
  byte_count = _final_packet_cursor;
  return _final_buffer.data();
}


//-----------------------------------------------------------------------------
// Connection methods.
//-----------------------------------------------------------------------------
void CollatedPacket::close()
{
  // Not supported.
}


void CollatedPacket::setActive(bool active)
{
  _active = active;
}


bool CollatedPacket::active() const
{
  return _active;
}


const char *CollatedPacket::address() const
{
  return "CollatedPacket";
}


uint16_t CollatedPacket::port() const
{
  return 0;
}


bool CollatedPacket::isConnected() const
{
  return true;
}


// FIXME(KS): reduce the complexity of the create() function.
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
int CollatedPacket::create(const Shape &shape)
{
  if (!_active)
  {
    return 0;
  }

  // Start by trying to write directly into the packet.
  int written = 0;
  bool wrote_message = false;  // Create message written?
  bool expanded = false;
  const unsigned initial_cursor = _cursor;

  PacketWriter writer(_buffer.data() + _cursor,
                      static_cast<uint16_t>(std::min<size_t>(
                        _buffer.size() - _cursor - sizeof(PacketWriter::CrcType), 0xffffu)));
  // Keep trying to write the packet while we don't have a fatal error.
  // Supports resizing the buffer.
  while (!wrote_message && written != -1)
  {
    wrote_message = shape.writeCreate(writer);
    if (wrote_message)
    {
      if (writer.finalise())
      {
        _cursor += writer.packetSize();
        written += writer.packetSize();
      }
      else
      {
        written = -1;
      }
    }
    else if (!expanded)
    {
      // Try resize.
      expand(1024u, _buffer, _max_packet_size);
      expanded = true;
      writer = PacketWriter(_buffer.data() + _cursor,
                            static_cast<uint16_t>(std::min<size_t>(
                              _buffer.size() - _cursor - sizeof(PacketWriter::CrcType), 0xffffu)));
    }
    else
    {
      written = -1;
    }
  }

  if (wrote_message && shape.isComplex())
  {
    // More to write. Support buffer expansion.
    bool complete = false;
    unsigned progress = 0;
    int res = 0;

    while (!complete && written != -1)
    {
      writer = PacketWriter(_buffer.data() + _cursor,
                            static_cast<uint16_t>(std::min<size_t>(
                              _buffer.size() - _cursor - sizeof(PacketWriter::CrcType), 0xffffu)));
      res = shape.writeData(writer, progress);

      if (res >= 0)
      {
        // Good write.
        if (writer.finalise())
        {
          // Good finalise.
          _cursor += writer.packetSize();
          written += writer.packetSize();
          writer =
            PacketWriter(_buffer.data() + _cursor,
                         static_cast<uint16_t>(std::min<size_t>(
                           _buffer.size() - _cursor - sizeof(PacketWriter::CrcType), 0xffffu)));
        }
        else
        {
          // Failed to finalise.
          written = -1;
        }

        complete = res == 0;
      }
      else
      {
        // Failed to write. Try resize.
        if (_buffer.size() < maxPacketSize())
        {
          expand(1024u, _buffer, _max_packet_size);
          writer =
            PacketWriter(_buffer.data() + _cursor,
                         static_cast<uint16_t>(std::min<size_t>(
                           _buffer.size() - _cursor - sizeof(PacketWriter::CrcType), 0xffffu)));
        }
        else
        {
          // Can't expand any more. Abort.
          written = -1;
        }
      }
    }
  }

  // Reset on error.
  if (written == -1)
  {
    _cursor = initial_cursor;
  }

  return written;
}


int CollatedPacket::destroy(const Shape &shape)
{
  if (!_active)
  {
    return 0;
  }

  // Start by trying to write directly into the packet.
  int written = 0;
  bool wrote_message = false;  // Create message written?
  bool expanded = false;
  const unsigned initial_cursor = _cursor;

  PacketWriter writer(_buffer.data() + _cursor,
                      static_cast<uint16_t>(std::min<size_t>(
                        _buffer.size() - _cursor - sizeof(PacketWriter::CrcType), 0xffffu)));
  // Keep trying to write the packet while we don't have a fatal error.
  // Supports resizing the buffer.
  while (wrote_message && written != -1)
  {
    wrote_message = shape.writeDestroy(writer);
    if (wrote_message)
    {
      if (writer.finalise())
      {
        _cursor += writer.packetSize();
        written += writer.packetSize();
      }
      else
      {
        written = -1;
      }
    }
    else if (!expanded)
    {
      // Try resize.
      expand(1024u, _buffer, _max_packet_size);
      expanded = true;
      writer = PacketWriter(_buffer.data() + _cursor,
                            static_cast<uint16_t>(std::min<size_t>(
                              _buffer.size() - _cursor - sizeof(PacketWriter::CrcType), 0xffffu)));
    }
    else
    {
      written = -1;
    }
  }

  // Reset on error.
  if (written == -1)
  {
    _cursor = initial_cursor;
  }

  return written;
}


int CollatedPacket::update(const Shape &shape)
{
  if (!_active)
  {
    return 0;
  }

  // Start by trying to write directly into the packet.
  int written = 0;
  bool wrote_message = false;  // Create message written?
  bool expanded = false;
  const unsigned initial_cursor = _cursor;

  PacketWriter writer(_buffer.data() + _cursor,
                      static_cast<uint16_t>(std::min<size_t>(
                        _buffer.size() - _cursor - sizeof(PacketWriter::CrcType), 0xffffu)));
  // Keep trying to write the packet while we don't have a fatal error.
  // Supports resizing the buffer.
  while (wrote_message && written != -1)
  {
    wrote_message = shape.writeUpdate(writer);
    if (wrote_message)
    {
      if (writer.finalise())
      {
        _cursor += writer.packetSize();
        written += writer.packetSize();
      }
      else
      {
        written = -1;
      }
    }
    else if (!expanded)
    {
      // Try resize.
      expand(1024u, _buffer, _max_packet_size);
      expanded = true;
      writer = PacketWriter(_buffer.data() + _cursor,
                            static_cast<uint16_t>(std::min<size_t>(
                              _buffer.size() - _cursor - sizeof(PacketWriter::CrcType), 0xffffu)));
    }
    else
    {
      written = -1;
    }
  }

  // Reset on error.
  if (written == -1)
  {
    _cursor = initial_cursor;
  }

  return written;
}


int CollatedPacket::updateTransfers(unsigned /*byte_limit*/)
{
  return -1;
}


int CollatedPacket::updateFrame(float dt, bool flush)
{
  TES_UNUSED(dt);
  TES_UNUSED(flush);
  // Not supported
  return -1;
}


unsigned tes::CollatedPacket::referenceResource(const ResourcePtr &resource)
{
  TES_UNUSED(resource);
  return 0;
}


unsigned tes::CollatedPacket::releaseResource(const ResourcePtr &resource)
{
  TES_UNUSED(resource);
  return 0;
}


bool CollatedPacket::sendServerInfo(const ServerInfoMessage &info)
{
  if (!_active)
  {
    return false;
  }

  // Start by trying to write directly into the packet.
  int written = 0;
  bool wrote_message = false;  // Create message written?
  bool expanded = false;
  const unsigned initial_cursor = _cursor;

  PacketWriter writer(_buffer.data() + _cursor,
                      static_cast<uint16_t>(std::min<size_t>(
                        _buffer.size() - _cursor - sizeof(PacketWriter::CrcType), 0xffffu)));
  writer.reset(MtServerInfo, 0);
  // Keep trying to write the packet while we don't have a fatal error.
  // Supports resizing the buffer.
  while (wrote_message && written != -1)
  {
    wrote_message = info.write(writer);
    if (wrote_message)
    {
      if (writer.finalise())
      {
        _cursor += writer.packetSize();
        written += writer.packetSize();
      }
      else
      {
        written = -1;
      }
    }
    else if (!expanded)
    {
      // Try resize.
      expand(1024u, _buffer, _max_packet_size);
      expanded = true;
      writer = PacketWriter(_buffer.data() + _cursor,
                            static_cast<uint16_t>(std::min<size_t>(
                              _buffer.size() - _cursor - sizeof(PacketWriter::CrcType), 0xffffu)));
    }
    else
    {
      written = -1;
    }
  }

  // Reset on error.
  if (written == -1)
  {
    _cursor = initial_cursor;
  }

  return written != -1;
}


int CollatedPacket::send(const PacketWriter &packet, bool /*allow_collation*/)
{
  return send(packet.data(), packet.packetSize(), false);
}


int CollatedPacket::send(const uint8_t *data, int byte_count, bool /*allow_collation*/)
{
  if (!_active)
  {
    return 0;
  }

  if (byte_count > 0xffff)
  {
    return -1;
  }

  return add(data, static_cast<uint16_t>(byte_count));
}


int CollatedPacket::send(const CollatedPacket &collated)
{
  TES_UNUSED(collated);
  TES_THROW(Exception("CollatedPacket::send(CollatedPacket) not supported"), -1);
}


//-----------------------------------------------------------------------------
// Private
//-----------------------------------------------------------------------------
void CollatedPacket::init(bool compress, unsigned buffer_size, unsigned max_packet_size)
{
  if (buffer_size == 0)
  {
    buffer_size = 16 * 1024;
  }
  _buffer.resize(buffer_size);
  _final_buffer.clear();
  _cursor = _final_packet_cursor = 0;
  _max_packet_size = max_packet_size;

#ifdef TES_ZLIB
  if (compress)
  {
    _zip = std::make_unique<CollatedPacketZip>(false);
  }
#endif  // TES_ZLIB
}


void CollatedPacket::expand(unsigned expand_by, std::vector<uint8_t> &buffer,
                            unsigned max_packet_size)
{
  // Buffer too small.
  const auto new_buffer_size = std::min(
    nextLog2(static_cast<unsigned>(buffer.size() + expand_by + Overhead)), max_packet_size);
  buffer.resize(new_buffer_size);
}
}  // namespace tes
