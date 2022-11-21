//
// author: Kazys Stepanas
//
#include "3escollatedpacketdecoder.h"

#include "3esmessages.h"
#include "3espacketbuffer.h"
#include "3espacketheader.h"
#include "3espacketreader.h"

#include "private/3escollatedpacketzip.h"

#include <vector>

using namespace tes;

namespace
{
size_t DefaultBufferSize = 4 * 1024u;
}

namespace tes
{
unsigned getPacketSize(const uint8_t *bytes)
{
  PacketReader reader(reinterpret_cast<const PacketHeader *>(bytes));
  if (reader.marker() != PacketMarker)
  {
    // Invalid marker bytes. Fail.
    return 0;
  }

  // Validate read buffer size.
  return reader.packetSize();
}

struct CollatedPacketDecoderDetail
{
  std::vector<uint8_t> buffer;
  unsigned targetBytes = 0;   // Number of bytes to decode.
  unsigned decodedBytes = 0;  // Number decoded.
  unsigned streamBytes = 0;   // Number of bytes in stream.
  const PacketHeader *packet = nullptr;
  const uint8_t *stream = nullptr;
  CollatedPacketZip zip = CollatedPacketZip(true);
  bool compressed = false;
  bool ok = false;

  bool init(const PacketHeader *packet)
  {
    this->packet = packet;
    if (!packet)
    {
      initStream(0, 0, nullptr, 0);
      return false;
    }

    if (buffer.size() < DefaultBufferSize)
    {
      buffer.resize(DefaultBufferSize);
    }

    PacketReader reader(packet);
    if (reader.routingId() == MtCollatedPacket)
    {
      CollatedPacketMessage msg;
      if (!msg.read(reader))
      {
        return false;
      }

      if (!initStream(msg.flags, msg.uncompressedBytes, reader.payload() + reader.tell(),
                      reader.payloadSize() - reader.tell()))
      {
        return false;
      }
    }
    else
    {
      initStream(0, 0, nullptr, 0);
      targetBytes = reader.payloadSize();
    }
    return true;
  }

  void finishCurrent()
  {
    packet = nullptr;
    stream = nullptr;
    zip.reset();
  }

  bool initStream(unsigned messageFlags, unsigned targetDecodeBytes, const uint8_t *bytes, unsigned byteCount)
  {
    zip.reset();
    stream = bytes;
    streamBytes = byteCount;
    targetBytes = targetDecodeBytes;
    decodedBytes = 0;

    if (buffer.size() < targetDecodeBytes)
    {
      buffer.resize(targetDecodeBytes);
    }

    ok = false;
    if (messageFlags & CPFCompress)
    {
#ifdef TES_ZLIB
      ok = true;
      compressed = true;
      zip.stream.zalloc = Z_NULL;
      zip.stream.zfree = Z_NULL;
      zip.stream.opaque = Z_NULL;
      zip.stream.avail_in = 0;
      zip.stream.next_in = Z_NULL;
      ok = inflateInit2(&zip.stream, CollatedPacketZip::WindowBits | CollatedPacketZip::GZipEncoding) == Z_OK;
      zip.stream.avail_in = streamBytes;
      zip.stream.next_in = (z_const Bytef *)stream;
#endif  // TES_ZLIB
    }
    else
    {
      ok = true;
      compressed = false;
    }

    if (!ok)
    {
      // Failed stream.
      initStream(0, 0, nullptr, 0);
      // Initialising like this will set ok to true. Force failure.
      ok = false;
    }

    return ok;
  }

  const PacketHeader *nextPacket()
  {
    if (decodedBytes >= targetBytes)
    {
      return nullptr;
    }

    if (compressed)
    {
#ifdef TES_ZLIB
      // Deflate into the buffer.
      int status = 0;
      // Decode just one header.
      zip.stream.avail_out = sizeof(PacketHeader);
      zip.stream.next_out = buffer.data();
      status = inflate(&zip.stream, Z_NO_FLUSH);
      if (status == Z_STREAM_ERROR || status == Z_NEED_DICT || status == Z_DATA_ERROR || status == Z_MEM_ERROR)
      {
        return nullptr;
      }

      if (zip.stream.avail_out != 0)
      {
        // Failed to read header.
        return nullptr;
      }

      // Validate the header. Use a PacketReader to ensure endian swap as needed.
      unsigned packetSize = getPacketSize(buffer.data());
      if (packetSize == 0)
      {
        // Validation failed.
        return nullptr;
      }

      // Validate read buffer size.
      if (buffer.size() < packetSize)
      {
        buffer.resize(packetSize);
        zip.stream.next_out = buffer.data() + sizeof(PacketHeader);
      }

      // Inflate remaining packet bytes.
      zip.stream.avail_out = uInt(packetSize - sizeof(PacketHeader));
      status = inflate(&zip.stream, Z_NO_FLUSH);

      if (status == Z_STREAM_ERROR || status == Z_NEED_DICT || status == Z_DATA_ERROR || status == Z_MEM_ERROR)
      {
        return nullptr;
      }

      if (zip.stream.avail_out)
      {
        // Failed to decode target bytes.
        return nullptr;
      }

      PacketReader reader(reinterpret_cast<const PacketHeader *>(buffer.data()));

      // Now check the packet.
      if (reader.packetSize() != zip.stream.total_out - decodedBytes)
      {
        return nullptr;
      }

      decodedBytes = unsigned(zip.stream.total_out);

      if (!reader.checkCrc())
      {
        return nullptr;
      }

      if (decodedBytes == targetBytes)
      {
        // Nothing more to decode.
        finishCurrent();
      }

      return reinterpret_cast<const PacketHeader *>(buffer.data());
#else   // TES_ZLIB
        // Compression unsupported.
      return nullptr;
#endif  // TES_ZLIB
    }
    else
    {
      unsigned packetSize = getPacketSize(stream + decodedBytes);
      if (packetSize == 0)
      {
        // Validation failed.
        return nullptr;
      }

      const PacketHeader *nextPacket = reinterpret_cast<const PacketHeader *>(stream + decodedBytes);
      decodedBytes += packetSize;

      if (decodedBytes == targetBytes)
      {
        // Nothing more to decode.
        finishCurrent();
      }

      return nextPacket;
    }
  }
};
}  // namespace tes

CollatedPacketDecoder::CollatedPacketDecoder(const PacketHeader *packet)
  : _detail(nullptr)
{
  setPacket(packet);
}


CollatedPacketDecoder::~CollatedPacketDecoder()
{
  delete _detail;
}


unsigned CollatedPacketDecoder::decodedBytes() const
{
  return (_detail) ? _detail->decodedBytes : 0u;
}


unsigned CollatedPacketDecoder::targetBytes() const
{
  return (_detail) ? _detail->targetBytes : 0u;
}


bool CollatedPacketDecoder::decoding() const
{
  return _detail && _detail->packet;
}


bool CollatedPacketDecoder::setPacket(const PacketHeader *packet)
{
  if (packet)
  {
    if (!_detail)
    {
      _detail = new CollatedPacketDecoderDetail;
    }

    return _detail->init(packet);
  }
  else if (_detail)
  {
    _detail->init(nullptr);
  }

  return false;
}


const PacketHeader *CollatedPacketDecoder::next()
{
  if (_detail)
  {
    if (_detail->stream)
    {
      // We have a collated packet. Start decoding.
      return _detail->nextPacket();
    }

    const PacketHeader *next = _detail->packet;
    _detail->decodedBytes = _detail->targetBytes;
    _detail->packet = nullptr;
    return next;
  }

  return nullptr;
}
