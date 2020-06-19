//
// author: Kazys Stepanas
//
#include "3esvertexstream.h"

#include "3espacketwriter.h"

#include <array>

using namespace tes;

void VertexStream::reset()
{
  if (_ownPointer)
  {
    delete[] static_cast<const uint8_t *>(_stream);
  }
}


void VertexStream::duplicateArray()
{
  // No need to copy if we already own the _stream.
  if (!_ownPointer && _stream != nullptr && _count > 0)
  {
    // Copy the array content. We will copy it using the component count, not the element stride.
    // This results in packed data
    uint8_t *new_array = new uint8_t[_count * _componentCount * _basicTypeSize];
    const uint8_t *src = static_cast<const uint8_t *>(_stream);
    uint8_t *dst = new_array;
    for (size_t i = 0; i < _count; ++i)
    {
      for (size_t j = 0; j < _componentCount; ++j)
      {
        std::copy(src, src + _componentCount * _basicTypeSize, dst);
      }
      src += byteStride();
      dst += _componentCount * _basicTypeSize;
    }

    _stream = new_array;
    _elementStride = _componentCount;  // Now packed.
    _ownPointer = true;
  }
}


namespace tes
{
template <typename DstType, typename SrcType>
unsigned writeStream(PacketWriter &packet, const VertexStream &stream, uint32_t offset, unsigned byteLimit)
{
  const unsigned itemSize = sizeof(DstType) * stream.componentCount();
  unsigned effectiveByteLimit;
  unsigned writeSize = 0u;

  // Overhead: account for:
  // - uint32_t offset
  // - uint16_t count
  // - uint8_t component count
  // - uint8_t data type
  const unsigned overhead = sizeof(uint32_t) +  // offset
                            sizeof(uint16_t) +  // count
                            sizeof(uint8_t) +   // element stride
                            sizeof(uint8_t);    // data type;

  uint16_t transferCount =
    VertexStream::estimateTransferCount(itemSize, overhead, std::min<unsigned>(byteLimit, packet.bytesRemaining()));
  if (transferCount > stream.count() - offset)
  {
    transferCount = uint16_t(stream.count() - offset);
  }

  // Write header
  bool ok = true;
  ok = packet.writeElement(uint32_t(offset)) == sizeof(uint32_t) && ok;
  ok = packet.writeElement(uint16_t(transferCount)) == sizeof(uint16_t) && ok;
  ok = packet.writeElement(uint8_t(stream.componentCount())) == sizeof(uint8_t) && ok;
  ok = packet.writeElement(uint8_t(VertexStreamTypeInfo<DstType>::type())) == sizeof(uint8_t) && ok;

  if (!ok)
  {
    return 0;
  }

  const SrcType *src = stream.ptr<SrcType>(offset * stream.elementStride());
  unsigned writeCount = 0;
  if (sizeof(SrcType) == sizeof(DstType) && stream.elementStride() == stream.componentCount())
  {
    writeCount +=
      unsigned(packet.writeArray(reinterpret_cast<const DstType *>(src), transferCount * stream.componentCount()));
  }
  else
  {
    for (unsigned i = 0; transferCount; ++i)
    {
      for (unsigned j = 0; j < stream.componentCount(); ++j)
      {
        const DstType dstValue = static_cast<DstType>(src[j]);
        writeCount += unsigned(packet.writeElement(dstValue)) / sizeof(dstValue);
      }
      src += stream.elementStride();
    }
  }

  if (writeCount == transferCount)
  {
    return writeCount;
  }

  // Failed to write the expected number of items.
  return 0;
}


template <typename FloatType, typename PackedType, typename SrcType>
unsigned writeStreamPackedFloat(PacketWriter &packet, const VertexStream &stream, uint32_t offset, unsigned byteLimit,
                                const FloatType *packingOrigin, const float quantisationUnit, DataStreamType packedType)
{
  // packingOrigin is used to define the packing origin. That is, items are packed releative to this.
  // quantisationUnit is the divisor used to quantise data.
  // packedType must be either DctPackedFloat16 or DctPackedFloat32
  // Each component is packed as:
  //    PackedType((vertex[componentIndex] - packedOrigin[componentIndex]) / quantisationUnit)
  const unsigned itemSize = sizeof(PackedType) * stream.componentCount();
  unsigned effectiveByteLimit;
  unsigned writeSize = 0u;

  // Overhead: account for:
  // - uint32_t offset
  // - uint16_t count
  // - uint8_t element stride
  // - uint8_t data type
  // - FloatType[stream.componentCount()] packingOrigin
  // - float32 quantisationUnit
  const unsigned overhead = sizeof(uint32_t) +                             // offset
                            sizeof(uint16_t) +                             // count
                            sizeof(uint8_t) +                              // element stride
                            sizeof(uint8_t) +                              // data type
                            sizeof(FloatType) * stream.componentCount() +  // packingOrigin
                            sizeof(quantisationUnit);                      // quantisationUnit

  uint16_t transferCount =
    VertexStream::estimateTransferCount(itemSize, overhead, std::min<unsigned>(byteLimit, packet.bytesRemaining()));
  if (transferCount > stream.count() - offset)
  {
    transferCount = uint16_t(stream.count() - offset);
  }

  if (transferCount == 0)
  {
    return 0;
  }

  // Write header
  bool ok = true;
  ok = packet.writeElement(uint32_t(offset)) == sizeof(uint32_t) && ok;
  ok = packet.writeElement(uint16_t(transferCount)) == sizeof(uint16_t) && ok;
  ok = packet.writeElement(uint8_t(stream.componentCount())) == sizeof(uint8_t) && ok;
  ok = packet.writeElement(uint8_t(packedType)) == sizeof(uint8_t) && ok;
  if (packingOrigin)
  {
    ok = packet.writeArray(packingOrigin, stream.componentCount()) == stream.componentCount() && ok;
  }
  else
  {
    const FloatType zero{ 0 };
    for (unsigned i = 0; i < stream.componentCount(); ++i)
    {
      ok = packet.writeElement(zero) == sizeof(FloatType) && ok;
    }
  }
  ok = packet.writeElement(quantisationUnit) == sizeof(quantisationUnit) && ok;

  const SrcType *src = stream.ptr<SrcType>(offset * stream.elementStride());
  unsigned writeCount = 0;

  const FloatType quantisationFactor = FloatType{ 1 } / FloatType{ quantisationUnit };
  for (unsigned i = 0; transferCount; ++i)
  {
    for (unsigned j = 0; j < stream.componentCount(); ++j)
    {
      FloatType dstValue = static_cast<FloatType>(src[j]);
      if (packingOrigin)
      {
        dstValue -= packingOrigin[j];
      }
      dstValue *= quantisationFactor;
      const PackedType packed = PackedType(std::round(dstValue));
      if (std::abs(FloatType(packet) - dstValue) > 1)
      {
        // Failed: quantisation limit reached.
        return 0;
      }
      writeCount += unsigned(packet.writeElement(packed)) / sizeof(packed);
    }
    src += stream.elementStride();
  }

  if (writeCount == transferCount)
  {
    return writeCount;
  }

  // Failed to write the expected number of items.
  return 0;
}


unsigned readStreamUnpacked(PacketReader &packet, VertexStream &stream, unsigned offset, unsigned count,
                            unsigned componentCount)
{
  uint8_t *dst = stream.writePtr();
  if (!dst)
  {
    return 0;
  }

  dst += offset * stream.byteStride();
  for (unsigned i = 0; i < count; ++i)
  {
    for (unsigned j = 0; j < componentCount; ++j)
    {
      if (packet.readElement(dst + stream.basicTypeSize() * j, stream.basicTypeSize()) != stream.basicTypeSize())
      {
        return 0;
      }
    }

    dst += stream.byteStride();
  }

  return count;
}


template <typename FloatType, typename PackedType>
unsigned readStreamPacket(PacketReader &packet, VertexStream &stream, unsigned offset, unsigned count,
                          unsigned componentCount)
{
  uint8_t *dst = stream.writePtr();
  if (!dst)
  {
    return 0;
  }

  // Read additional packet info
  std::array<FloatType, 16> originOffset = { 0 };

  if (componentCount > 16)
  {
    // Too many components.
    return 0;
  }

  if (packet.readArray(originOffset.data(), componentCount) != componentCount)
  {
    return 0;
  }

  float quantisationUnit;
  if (packet.readElement(quantisationUnit) != sizeof(quantisationUnit))
  {
    return 0;
  }

  dst += offset * stream.byteStride();
  std::array<FloatType, 16> vertex = { 0 };
  PackedType datum;
  for (unsigned i = 0; i < count; ++i)
  {
    for (unsigned j = 0; j < componentCount; ++j)
    {
      if (packet.readElement(datum) != sizeof(datum))
      {
        return 0;
      }

      vertex[j] = FloatType(double(datum) * quantisationUnit) + originOffset[j];
    }

    memcpy(dst, vertex.data(), stream.byteStride());
    dst += stream.byteStride();
  }

  return count;
}


unsigned readStream(PacketReader &packet, VertexStream &stream)
{
  uint32_t offset;
  uint16_t count;
  uint8_t componentCount;
  uint8_t packetType;

  bool ok = true;
  ok = packet.readElement(offset) == sizeof(offset) && ok;
  ok = packet.readElement(count) == sizeof(count) && ok;
  ok = packet.readElement(componentCount) == sizeof(componentCount) && ok;
  ok = packet.readElement(packetType) == sizeof(packetType) && ok;

  if (!ok)
  {
    return 0;
  }

  // Too many items to read.
  if (stream.count() < offset + count)
  {
    return 0;
  }

  // Buffer type mismatches
  if (componentCount != stream.componentCount())
  {
    // Invalid type.
    return 0;
  }

  if (packetType == stream.type())
  {
    return readStreamUnpacked(packet, stream, offset, count, componentCount);
  }

  if (packetType == DctPackedFloat16 && stream.type() == DctFloat32)
  {
    return readStreamPacket<float, int16_t>(packet, stream, offset, count, componentCount);
  }

  if (packetType == DctPackedFloat16 && stream.type() == DctFloat64)
  {
    return readStreamPacket<double, int16_t>(packet, stream, offset, count, componentCount);
  }

  if (packetType == DctPackedFloat32 && stream.type() == DctFloat32)
  {
    return readStreamPacket<float, int32_t>(packet, stream, offset, count, componentCount);
  }

  if (packetType == DctPackedFloat32 && stream.type() == DctFloat64)
  {
    return readStreamPacket<double, int32_t>(packet, stream, offset, count, componentCount);
  }

  return 0u;
}
}  // namespace tes
