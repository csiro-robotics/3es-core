//
// author: Kazys Stepanas
//
#include "CollatedPacketDecoder.h"

#include "CoreUtil.h"
#include "Messages.h"
#include "PacketBuffer.h"
#include "PacketHeader.h"
#include "PacketReader.h"

#include "private/CollatedPacketZip.h"

#include <vector>

namespace tes
{
namespace
{
constexpr size_t kDefaultBufferSize =
  4u * 1024u;  // NOLINT(bugprone-implicit-widening-of-multiplication-result)
}  // namespace

unsigned getPacketSize(const uint8_t *bytes)
{
  const PacketReader reader(reinterpret_cast<const PacketHeader *>(bytes));
  if (reader.marker() != kPacketMarker)
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
  unsigned target_bytes = 0;   // Number of bytes to decode.
  unsigned decoded_bytes = 0;  // Number decoded.
  unsigned stream_bytes = 0;   // Number of bytes in stream.
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

    if (buffer.size() < kDefaultBufferSize)
    {
      buffer.resize(kDefaultBufferSize);
    }

    PacketReader reader(packet);
    if (reader.routingId() == MtCollatedPacket)
    {
      CollatedPacketMessage msg;
      if (!msg.read(reader))
      {
        return false;
      }

      if (!initStream(msg.flags, msg.uncompressed_bytes, reader.payload() + reader.tell(),
                      reader.payloadSize() - reader.tell()))
      {
        return false;
      }
    }
    else
    {
      initStream(0, 0, nullptr, 0);
      target_bytes = reader.payloadSize();
    }
    return true;
  }

  void finishCurrent()
  {
    packet = nullptr;
    stream = nullptr;
    zip.reset();
  }

  bool initStream(unsigned message_flags, unsigned target_decode_bytes, const uint8_t *bytes,
                  unsigned byte_count)
  {
    zip.reset();
    stream = bytes;
    stream_bytes = byte_count;
    target_bytes = target_decode_bytes;
    decoded_bytes = 0;

    if (buffer.size() < target_decode_bytes)
    {
      buffer.resize(target_decode_bytes);
    }

    ok = false;
    if (message_flags & CPFCompress)  // NOLINT(hicpp-signed-bitwise)
    {
#ifdef TES_ZLIB
      ok = true;
      compressed = true;
      zip.stream.zalloc = Z_NULL;
      zip.stream.zfree = Z_NULL;
      zip.stream.opaque = Z_NULL;
      zip.stream.avail_in = 0;
      zip.stream.next_in = Z_NULL;
      ok = inflateInit2(&zip.stream,
                        CollatedPacketZip::WindowBits | CollatedPacketZip::GZipEncoding) == Z_OK;
      zip.stream.avail_in = stream_bytes;
      // NOLINTNEXTLINE(google-readability-casting)
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

  [[nodiscard]] const PacketHeader *nextPacket()
  {
    if (decoded_bytes >= target_bytes)
    {
      return nullptr;
    }

    if (compressed)
    {
      return nextPacketCompressed();
    }

    const unsigned packet_size = getPacketSize(stream + decoded_bytes);
    if (packet_size == 0)
    {
      // Validation failed.
      return nullptr;
    }

    const auto *next_packet = reinterpret_cast<const PacketHeader *>(stream + decoded_bytes);
    decoded_bytes += packet_size;

    if (decoded_bytes == target_bytes)
    {
      // Nothing more to decode.
      finishCurrent();
    }

    return next_packet;
  }

private:
  [[nodiscard]] const PacketHeader *nextPacketCompressed()
  {
#ifdef TES_ZLIB
    // Deflate into the buffer.
    int status = 0;
    // Decode just one header.
    zip.stream.avail_out = sizeof(PacketHeader);
    zip.stream.next_out = buffer.data();
    status = inflate(&zip.stream, Z_NO_FLUSH);
    if (status == Z_STREAM_ERROR || status == Z_NEED_DICT || status == Z_DATA_ERROR ||
        status == Z_MEM_ERROR)
    {
      return nullptr;
    }

    if (zip.stream.avail_out != 0)
    {
      // Failed to read header.
      return nullptr;
    }

    // Validate the header. Use a PacketReader to ensure endian swap as needed.
    const unsigned packet_size = getPacketSize(buffer.data());
    if (packet_size == 0)
    {
      // Validation failed.
      return nullptr;
    }

    // Validate read buffer size.
    if (buffer.size() < packet_size)
    {
      buffer.resize(packet_size);
      zip.stream.next_out = buffer.data() + sizeof(PacketHeader);
    }

    // Inflate remaining packet bytes.
    zip.stream.avail_out = int_cast<uInt>(packet_size - sizeof(PacketHeader));
    status = inflate(&zip.stream, Z_NO_FLUSH);

    if (status == Z_STREAM_ERROR || status == Z_NEED_DICT || status == Z_DATA_ERROR ||
        status == Z_MEM_ERROR)
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
    if (reader.packetSize() != zip.stream.total_out - decoded_bytes)
    {
      return nullptr;
    }

    decoded_bytes = int_cast<unsigned>(zip.stream.total_out);

    if (!reader.checkCrc())
    {
      return nullptr;
    }

    if (decoded_bytes == target_bytes)
    {
      // Nothing more to decode.
      finishCurrent();
    }

    return reinterpret_cast<const PacketHeader *>(buffer.data());
#else   // TES_ZLIB
    // Compression not supported.
    return nullptr;
#endif  // TES_ZLIB
  }
};

CollatedPacketDecoder::CollatedPacketDecoder(const PacketHeader *packet)
{
  setPacket(packet);
}


CollatedPacketDecoder::~CollatedPacketDecoder() = default;


unsigned CollatedPacketDecoder::decodedBytes() const
{
  return (_detail) ? _detail->decoded_bytes : 0u;
}


unsigned CollatedPacketDecoder::targetBytes() const
{
  return (_detail) ? _detail->target_bytes : 0u;
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
      _detail = std::make_unique<CollatedPacketDecoderDetail>();
    }

    return _detail->init(packet);
  }
  if (_detail)
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
    _detail->decoded_bytes = _detail->target_bytes;
    _detail->packet = nullptr;
    return next;
  }

  return nullptr;
}
}  // namespace tes
