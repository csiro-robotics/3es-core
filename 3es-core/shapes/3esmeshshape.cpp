//
// author: Kazys Stepanas
//
#include "3esmeshshape.h"

#include "3esmeshresource.h"
#include "3esmeshset.h"

#include <3escoreutil.h>
#include <3espacketwriter.h>

#include <algorithm>

using namespace tes;

namespace
{
// Helper for automating data sending.
struct DataPhase
{
  // The SendDataType.
  uint16_t type;
  // Number of things to send. May be zero.
  unsigned itemCount;
  // Data pointer. May be null with zero itemCount.
  const uint8_t *dataSrc;
  // Byte stride between elements.
  size_t dataStrideBytes;
  // Base data item size, requiring endian swap.
  // See tupleSize.
  size_t dataSizeByte;
  // Number of data items in each stride. tupleSize * dataSizeBytes must be <= dataStrideBytes.
  //
  // Usage by example.
  // For a Vector3 data type of 3 packed floats:
  // - tupleSize = 3
  // - dataSizeBytes = sizeof(float)
  // - dataStrideBytes = tupleSize * dataSizeBytes = 12
  //
  // For a Vector3 data type aligned to 16 bytes (3 floats):
  // - tupleSize = 3
  // - dataSizeBytes = sizeof(float)
  // - dataStrideBytes = 16
  unsigned tupleSize;
};


unsigned readElements(PacketReader &stream, unsigned offset, unsigned itemCount, uint8_t *dstPtr,
                      size_t elementSizeBytes, unsigned elementCount, unsigned tupleSize = 1)
{
  if (offset > elementCount)
  {
    return ~0u;
  }

  if (itemCount == 0)
  {
    return offset + itemCount;
  }

  if (offset + itemCount > elementCount)
  {
    itemCount = elementCount - itemCount;
  }

  offset *= tupleSize;
  itemCount *= tupleSize;

  uint8_t *dst = const_cast<uint8_t *>(dstPtr);
  dst += offset * elementSizeBytes;
  size_t readCount = stream.readArray(dst, elementSizeBytes, itemCount);
  if (readCount != itemCount)
  {
    return ~0u;
  }

  return unsigned((readCount + offset) / tupleSize);
}
}  // namespace

MeshShape::~MeshShape()
{
  releaseArrays();
}


MeshShape &MeshShape::setNormals(const VertexStream &normals)
{
  setCalculateNormals(false);
  _normals = normals;
  return *this;
}


MeshShape &MeshShape::setUniformNormal(const Vector3f &normal)
{
  setCalculateNormals(false);
  Vector3f *n = new Vector3f(normal);
  _normals = std::move(VertexStream(n->v, 1, 3, 3, false));
  _normals.duplicateArray();
  return *this;
}


template <typename T>
void expandVertices(VertexStream &vertices, VertexStream &indices)
{
  if (vertices.isValid())
  {
    // First unpack all vertices and stop indexing.
    T *verts = new T[vertices.componentCount() * _indices.count()];
    T *dst = verts;
    for (unsigned i = 0; i < indices.count(); ++i)
    {
      const unsigned vind = *indices.ptr<unsigned>(i);
      for (unsigned j = 0; j < vertices.componentCount(), ++j)
      {
        const unsigned vind = *indices.ptr<unsigned>(i);
        *dst = vertices.ptr<T>(vind)[j];
      }
    }
    vertices.set(verts, indices.count(), vertices.componentCount(), vertices.componentCount(), true);
  }
}


MeshShape &MeshShape::expandVertices()
{
  if (_indices.count() == 0)
  {
    return duplicateArrays();
  }

  // Drop index / in favour of an expanded vertex array. We will end up owning all array memory with a null
  // index array.

  if (_vertices.type() == DctFloat64)
  {
    ::expandVertices<double>(_vertices, _indices);
  }
  else
  {
    ::expandVertices<float>(_vertices, _indices);
  }

  return *this;
}


MeshShape &MeshShape::duplicateArrays()
{
  _vertices.duplicateArray();
  _normals.duplicateArray();

  if (!_ownPointers)
  {
    if (_indices && _indexCount)
    {
      indices = new uint32_t[_indexCount];
      memcpy(indices, _indices, sizeof(*indices) * _indexCount);
    }

    if (_colours && _vertexStride)
    {
      colours = new uint32_t[_vertexCount];
      memcpy(colours, _colours, sizeof(*colours) * _vertexCount);
    }

    _vertices = vertices;
    _vertexStride = 3;
    _indices = indices;
    _colours = colours;

    _ownPointers = true;
  }

  if (!_ownNormals && _normals)
  {
    float *normals = nullptr;

    if (_normalsCount)
    {
      normals = new float[3 * _normalsCount];
      if (_vertexStride == 3)
      {
        memcpy(normals, _normals, sizeof(*normals) * 3 * _normalsCount);
      }
      else
      {
        float *dst = normals;
        const float *src = _normals;
        for (unsigned i = 0; i < _normalsCount; ++i, dst += 3, src += _vertexStride)
        {
          dst[0] = src[0];
          dst[1] = src[1];
          dst[2] = src[2];
        }
      }
    }

    _normals = normals;
    _normalsStride = 3;
    _ownNormals = true;
  }

  return *this;
}


bool MeshShape::writeCreate(PacketWriter &stream) const
{
  bool ok = Shape::writeCreate(stream);
  uint32_t count = _vertexCount;
  ok = stream.writeElement(count) == sizeof(count) && ok;
  count = _indexCount;
  ok = stream.writeElement(count) == sizeof(count) && ok;
  uint8_t drawType = _drawType;
  ok = stream.writeElement(drawType) == sizeof(drawType) && ok;
  ok = stream.writeElement(_drawScale) == sizeof(_drawScale) && ok;
  return ok;
}


int MeshShape::writeData(PacketWriter &stream, unsigned &progressMarker) const
{
  bool ok = true;
  DataMessage msg;
  // Local byte overhead needs to account for the size of sendType, offset and itemCount.
  // Use a larger value as I haven't got the edge cases quite right yet.
  const size_t localByteOverhead = 100;
  msg.id = _data.id;
  stream.reset(routingId(), DataMessage::MessageId);
  ok = msg.write(stream);

  // Send vertices or indices?
  uint32_t offset;
  uint32_t itemCount;
  uint16_t sendType;

  // Resolve what we are currently sending.
  unsigned phaseIndex = 0;
  unsigned previousPhaseOffset = 0;

  // Order to send data in and information required to automate sending.
  const uint16_t normalsSendType = (_normalsCount == 1) ? SDT_UniformNormal : SDT_Normals;
  const DataPhase phases[] = {
    { normalsSendType, _normalsCount, (const uint8_t *)_normals, _normalsStride * sizeof(*_normals), sizeof(*_normals),
      3 },
    { SDT_Colours, (_colours) ? _vertexCount : 0, (const uint8_t *)_colours, sizeof(*_colours), sizeof(*_colours), 1 },
    { SDT_Vertices, _vertexCount, (const uint8_t *)_vertices, _vertexStride * sizeof(*_vertices), sizeof(*_vertices),
      3 },
    { SDT_Indices, _indexCount, (const uint8_t *)_indices, sizeof(*_indices), sizeof(*_indices), 1 }
  };

  // While progressMarker is greater than or equal to the sum of the previous phase counts and the current phase count.
  // Also terminate of out of phases.
  while (phaseIndex < sizeof(phases) / sizeof(phases[0]) &&
         progressMarker >= previousPhaseOffset + phases[phaseIndex].itemCount)
  {
    previousPhaseOffset += phases[phaseIndex].itemCount;
    ++phaseIndex;
  }

  bool done = false;
  // Check if we have anything to send.
  if (phaseIndex < sizeof(phases) / sizeof(phases[0]))
  {
    const DataPhase &phase = phases[phaseIndex];
    // Send part of current phase.
    const unsigned maxItemCout = MeshResource::estimateTransferCount(phase.dataSizeByte * phase.tupleSize, 0,
                                                                     sizeof(DataMessage) + localByteOverhead);
    offset = progressMarker - previousPhaseOffset;
    itemCount = uint32_t(std::min<uint32_t>(phase.itemCount - offset, maxItemCout));

    sendType = phase.type | SDT_ExpectEnd;
    ok = stream.writeElement(sendType) == sizeof(sendType) && ok;
    ok = stream.writeElement(offset) == sizeof(offset) && ok;
    ok = stream.writeElement(itemCount) == sizeof(itemCount) && ok;

    const uint8_t *src = phase.dataSrc + offset * phase.dataStrideBytes;
    if (phase.dataStrideBytes == phase.dataSizeByte * phase.tupleSize)
    {
      ok = stream.writeArray(src, phase.dataSizeByte, itemCount * phase.tupleSize) == itemCount * phase.tupleSize && ok;
    }
    else
    {
      for (unsigned i = 0; i < itemCount; ++i, src += phase.dataStrideBytes)
      {
        ok = stream.writeArray(src, phase.dataSizeByte, phase.tupleSize) == phase.tupleSize && ok;
      }
    }

    progressMarker += itemCount;
  }
  else
  {
    // Either all done or no data to send.
    // In the latter case, we need to populate the message anyway.
    offset = itemCount = 0;
    sendType = SDT_ExpectEnd | SDT_End;
    ok = stream.writeElement(sendType) == sizeof(sendType) && ok;
    ok = stream.writeElement(offset) == sizeof(offset) && ok;
    ok = stream.writeElement(itemCount) == sizeof(itemCount) && ok;

    done = true;
  }

  if (!ok)
  {
    return -1;
  }
  // Return 1 while there is more data to process.
  return (!done) ? 1 : 0;
}


bool MeshShape::readCreate(PacketReader &stream)
{
  if (!Shape::readCreate(stream))
  {
    return false;
  }

  uint32_t vertexCount = 0;
  uint32_t indexCount = 0;
  uint8_t drawType = 0;
  bool ok = true;

  ok = ok && stream.readElement(vertexCount) == sizeof(vertexCount);
  ok = ok && stream.readElement(indexCount) == sizeof(indexCount);

  if (ok)
  {
    if (!_ownPointers)
    {
      _vertices = nullptr;
      _indices = nullptr;
      _colours = nullptr;
      _vertexCount = _indexCount = 0;
    }

    _ownPointers = true;
    if (_vertexCount < vertexCount || _vertexStride != 3)
    {
      delete[] _vertices;
      _vertices = new float[3 * vertexCount];
      _vertexStride = 3;
    }

    if (_indexCount < indexCount)
    {
      delete[] _indices;
      _indices = new unsigned[indexCount];
    }

    _vertexCount = vertexCount;
    _indexCount = indexCount;
  }

  if (_ownNormals)
  {
    delete[] _normals;
  }

  // Normals may or may not come. We find out in writeData().
  // _normalCount will either be 1 (uniform normals) or match _vertexCount.
  // Depends on SendDataType in readData()
  _normals = nullptr;
  _normalsCount = 0;
  _ownNormals = false;

  ok = ok && stream.readElement(drawType) == sizeof(drawType);
  _drawType = (DrawType)drawType;

  // Legacy support.
  if (stream.versionMajor() > 0 || stream.versionMajor() == 0 && stream.versionMinor() >= 2)
  {
    ok = ok && stream.readElement(_drawScale) == sizeof(_drawScale);
  }
  else
  {
    _drawScale = 0.0f;
  }

  return ok;
}


bool MeshShape::readData(PacketReader &stream)
{
  DataMessage msg;
  uint32_t offset = 0;
  uint32_t itemCount = 0;
  uint16_t dataType = 0;
  bool ok = true;

  ok = ok && msg.read(stream);

  ok = ok && stream.readElement(dataType) == sizeof(dataType);
  ok = ok && stream.readElement(offset) == sizeof(offset);
  ok = ok && stream.readElement(itemCount) == sizeof(itemCount);

  // Record and mask out end flags.
  uint16_t endFlags = (dataType & (SDT_ExpectEnd | SDT_End));
  dataType = uint16_t(dataType & ~endFlags);

  // Can only read if we own the pointers.
  if (!_ownPointers)
  {
    return false;
  }

  // FIXME: resolve the 'const' pointer casting. Reading was a retrofit.
  bool complete = false;
  unsigned endReadCount = 0;
  switch (dataType)
  {
  case SDT_Vertices:
    endReadCount = readElements(stream, offset, itemCount, (uint8_t *)_vertices, sizeof(*_vertices), _vertexCount, 3);
    ok = ok && endReadCount != ~0u;

    // Expect end marker.
    if (endFlags & SDT_End)
    {
      // Done.
      complete = true;
    }

    // Check for completion.
    if (!(endFlags & SDT_ExpectEnd))
    {
      complete = endReadCount == _vertexCount;
    }
    break;

  case SDT_Indices:
    endReadCount = readElements(stream, offset, itemCount, (uint8_t *)_indices, sizeof(*_indices), _indexCount);
    ok = ok && endReadCount != ~0u;
    break;

    // Normals handled together.
  case SDT_Normals:
  case SDT_UniformNormal:
    if (!_normals)
    {
      _normalsCount = (dataType == SDT_Normals) ? _vertexCount : 1;
      _normalsStride = 3;
      if (_normalsCount)
      {
        _normals = new float[3 * _normalsCount];
        _ownNormals = true;
      }
    }

    endReadCount = readElements(stream, offset, itemCount, (uint8_t *)_normals, sizeof(*_normals), _normalsCount, 3);
    ok = ok && endReadCount != ~0u;
    break;

  case SDT_Colours:
    if (!_colours && _vertexCount)
    {
      _colours = new uint32_t[_vertexCount];
    }

    endReadCount = readElements(stream, offset, itemCount, (uint8_t *)_colours, sizeof(*_colours), _vertexCount);
    ok = ok && endReadCount != ~0u;
    break;
  default:
    // Unknown data type.
    ok = false;
    break;
  }

  if (complete)
  {
    // Nothing in the test code.
  }

  return ok;
}


Shape *MeshShape::clone() const
{
  MeshShape *triangles = new MeshShape();
  onClone(triangles);
  triangles->_data = _data;
  return triangles;
}


void MeshShape::onClone(MeshShape *copy) const
{
  Shape::onClone(copy);
  copy->_vertices = nullptr;
  copy->_indices = nullptr;
  copy->_normals = nullptr;
  copy->_colours = nullptr;
  copy->_vertexCount = _vertexCount;
  copy->_normalsCount = _normalsCount;
  copy->_indexCount = _indexCount;
  copy->_vertexStride = 3;
  copy->_normalsStride = 3;
  copy->_drawType = _drawType;
  copy->_ownPointers = true;
  copy->_ownNormals = true;
  if (_vertexCount)
  {
    float *vertices = new float[3 * _vertexCount];
    if (_vertexStride == 3)
    {
      memcpy(vertices, _vertices, sizeof(*vertices) * _vertexCount * 3);
    }
    else
    {
      const float *src = _vertices;
      float *dst = vertices;
      for (unsigned i = 0; i < _vertexCount; ++i)
      {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        src += _vertexStride;
        dst += 3;
      }
    }
    copy->_vertices = vertices;
  }

  if (_indexCount)
  {
    unsigned *indices = new unsigned[_indexCount];
    memcpy(indices, _indices, sizeof(*indices) * _indexCount);
    copy->_indices = indices;
  }

  if (_normalsCount)
  {
    float *normals = new float[3 * _normalsCount];
    if (_normalsStride == 3)
    {
      memcpy(normals, _normals, sizeof(*normals) * _normalsCount * 3);
    }
    else
    {
      const float *src = _normals;
      float *dst = normals;
      for (unsigned i = 0; i < _normalsCount; ++i)
      {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        src += _normalsStride;
        dst += 3;
      }
    }
    copy->_normals = normals;
  }

  if (_colours && _vertexCount)
  {
    uint32_t *colours = new uint32_t[_vertexCount];
    memcpy(colours, _colours, sizeof(*_colours) * _vertexCount);
    copy->_colours = colours;
  }
}


void MeshShape::releaseArrays()
{
  if (_ownNormals)
  {
    delete[] _normals;
    _normals = nullptr;
  }

  if (_ownPointers)
  {
    delete[] _vertices;
    _vertices = nullptr;
    delete[] _indices;
    _indices = nullptr;
    delete[] _colours;
    _colours = nullptr;
  }
}
