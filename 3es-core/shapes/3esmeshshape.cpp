//
// author: Kazys Stepanas
//
#include "3esmeshshape.h"

#include "3esmeshresource.h"
#include "3esmeshset.h"

#include <3escoreutil.h>
#include <3espacketwriter.h>

#include <algorithm>

namespace tes
{

namespace
{
// Helper for automating data sending.
struct DataPhase
{
  // The SendDataType.
  MeshShape::SendDataType info_type;
  DataStreamType send_type;
  // Data pointer. May have null context (skipped).
  const DataBuffer *stream;
};
}  // namespace


MeshShape::MeshShape::Resource::Resource(MeshShape &shape, uint32_t resource_id)
  : _shape(shape)
  , _resource_id(resource_id)
{}


uint32_t MeshShape::Resource::id() const
{
  return _resource_id;
}


Resource *MeshShape::Resource::clone() const
{
  return new MeshShape::Resource(_shape, _resource_id);
}


Transform MeshShape::Resource::transform() const
{
  return _shape.transform();
}


uint32_t MeshShape::Resource::tint() const
{
  return _shape.attributes().colour;
}


uint8_t MeshShape::Resource::drawType(int stream) const
{
  return _shape.drawType();
}


unsigned MeshShape::Resource::vertexCount(int stream) const
{
  (void)stream;
  return _shape.vertices().count();
}


unsigned MeshShape::Resource::indexCount(int stream) const
{
  (void)stream;
  return _shape.indices().count();
}


DataBuffer MeshShape::Resource::vertices(int stream) const
{
  (void)stream;
  return _shape.vertices();
}


DataBuffer MeshShape::Resource::indices(int stream) const
{
  (void)stream;
  return _shape.indices();
}


DataBuffer MeshShape::Resource::normals(int stream) const
{
  (void)stream;
  return _shape.normals();
}


DataBuffer MeshShape::Resource::uvs(int stream) const
{
  (void)stream;
  return {};
}


DataBuffer MeshShape::Resource::colours(int stream) const
{
  (void)stream;
  return _shape.colours();
}


bool MeshShape::Resource::readCreate(PacketReader &packet)
{
  (void)packet;
  return false;
}


bool MeshShape::Resource::readTransfer(int messageType, PacketReader &packet)
{
  (void)messageType;
  (void)packet;
  return false;
}


MeshShape::MeshShape(const MeshShape &other)
  : Shape(other)
{
  other.onClone(this);
}


MeshShape::MeshShape(MeshShape &&other)
  : Shape(other)
  , _vertices(std::move(other._vertices))
  , _normals(std::move(other._normals))
  , _colours(std::move(other._colours))
  , _indices(std::move(other._indices))
  , _quantisationUnit(std::exchange(other._quantisationUnit, 0.0))
  , _drawScale(std::exchange(other._drawScale, 0.0f))
  , _drawType(std::exchange(other._drawType, DtPoints))
{}


MeshShape::~MeshShape()
{}


MeshShape &MeshShape::setNormals(const DataBuffer &normals)
{
  setCalculateNormals(false);
  _normals = normals;
  return *this;
}


MeshShape &MeshShape::setUniformNormal(const Vector3f &normal)
{
  setCalculateNormals(false);
  Vector3f *n = new Vector3f(normal);
  _normals = std::move(DataBuffer(n->v, 1, 3, 3, false));
  _normals.duplicate();
  return *this;
}


template <typename T>
void expandVertices(DataBuffer &vertices, DataBuffer &indices)
{
  if (vertices.isValid())
  {
    // First unpack all vertices and stop indexing.
    T *verts = new T[vertices.componentCount() * indices.count()];
    T *dst = verts;
    for (unsigned i = 0; i < indices.count(); ++i)
    {
      for (unsigned j = 0; j < vertices.componentCount(); ++j)
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
    tes::expandVertices<double>(_vertices, _indices);
  }
  else
  {
    tes::expandVertices<float>(_vertices, _indices);
  }

  return *this;
}


MeshShape &MeshShape::duplicateArrays()
{
  _vertices.duplicate();
  _normals.duplicate();
  _colours.duplicate();
  _indices.duplicate();
  return *this;
}


bool MeshShape::writeCreate(PacketWriter &packet) const
{
  bool ok = Shape::writeCreate(packet);
  uint32_t count = _vertices.count();
  ok = packet.writeElement(count) == sizeof(count) && ok;
  count = _indices.count();
  ok = packet.writeElement(count) == sizeof(count) && ok;
  ok = packet.writeElement(_drawScale) == sizeof(_drawScale) && ok;
  uint8_t drawType = _drawType;
  ok = packet.writeElement(drawType) == sizeof(drawType) && ok;
  return ok;
}


int MeshShape::writeData(PacketWriter &packet, unsigned &progressMarker) const
{
  bool ok = true;
  DataMessage msg;
  msg.id = _data.id;
  packet.reset(routingId(), DataMessage::MessageId);
  ok = msg.write(packet);

  // Send vertices or indices?
  uint32_t offset;

  // Resolve what we are currently sending.
  unsigned phaseIndex = 0;
  unsigned previousPhaseOffset = 0;

  // Order to send data in and information required to automate sending.
  const DataPhase phases[] = { { SDT_Vertices, _vertices.type(), &_vertices },
                               { SDT_Indices, _indices.type(), &_indices },
                               { SDT_Normals, _normals.type(), &_normals },
                               { SDT_Colours, _colours.type(), &_colours } };

  // While progressMarker is greater than or equal to the sum of the previous phase counts and the current phase count.
  // Also terminate of out of phases.
  while (phaseIndex < sizeof(phases) / sizeof(phases[0]) &&
         progressMarker >= previousPhaseOffset + phases[phaseIndex].stream->count())
  {
    previousPhaseOffset += phases[phaseIndex].stream->count();
    ++phaseIndex;
  }

  offset = progressMarker - previousPhaseOffset;

  bool done = false;
  unsigned writeCount = 0;
  switch (phaseIndex)
  {
  case SDT_Vertices:
    ok = packet.writeElement(uint16_t(phaseIndex)) == sizeof(uint16_t) && ok;
    if (_quantisationUnit > 0)
    {
      writeCount = _vertices.writePacked(packet, offset, _quantisationUnit);
    }
    else
    {
      writeCount = _vertices.write(packet, offset);
    }
    break;
  case SDT_Indices:
    ok = packet.writeElement(uint16_t(phaseIndex)) == sizeof(uint16_t) && ok;
    writeCount = _indices.write(packet, offset);
    break;
  case SDT_Normals:
    ok = packet.writeElement(uint16_t(phaseIndex)) == sizeof(uint16_t) && ok;
    if (_quantisationUnit > 0)
    {
      writeCount = _normals.writePacked(packet, offset, 1.0f / float(0xffff));
    }
    else
    {
      writeCount = _normals.write(packet, offset);
    }
    break;
  case SDT_Colours:
    ok = packet.writeElement(uint16_t(phaseIndex)) == sizeof(uint16_t) && ok;
    writeCount = _colours.write(packet, offset);
    break;

  default:
    // Either all done or no data to send.
    ok = packet.writeElement(uint16_t(SDT_End)) == sizeof(uint16_t) && ok;
    // Write zero offset (4-bytes) and count (2-bytes) for consistency.
    ok = packet.writeElement(uint32_t(0)) == sizeof(uint32_t) && ok;
    ok = packet.writeElement(uint16_t(0)) == sizeof(uint16_t) && ok;
    done = true;
    break;
  }

  progressMarker += writeCount;
  ok = done || writeCount > 0;

  if (!ok)
  {
    // Write failure.
    return -1;
  }

  // Return 1 while there is more data to process.
  return (!done) ? 1 : 0;
}


bool MeshShape::readCreate(PacketReader &packet)
{
  if (!Shape::readCreate(packet))
  {
    return false;
  }

  uint32_t vertexCount = 0;
  uint32_t indexCount = 0;
  uint8_t drawType = 0;
  bool ok = true;

  ok = ok && packet.readElement(vertexCount) == sizeof(vertexCount);
  ok = ok && packet.readElement(indexCount) == sizeof(indexCount);

  _vertices.set(static_cast<float *>(nullptr), 0, 3);
  _normals.set(static_cast<float *>(nullptr), 0, 3);
  _indices.set(static_cast<uint32_t *>(nullptr), 0);
  _colours.set(static_cast<uint32_t *>(nullptr), 0);

  ok = ok && packet.readElement(_drawScale) == sizeof(_drawScale);

  ok = ok && packet.readElement(drawType) == sizeof(drawType);
  _drawType = (DrawType)drawType;

  return ok;
}


bool MeshShape::readData(PacketReader &packet)
{
  DataMessage msg;
  uint16_t dataType = 0;
  bool ok = true;

  ok = ok && msg.read(packet);
  ok = ok && packet.readElement(dataType) == sizeof(dataType);

  // Record and mask out end flags.
  dataType = dataType;

  unsigned endReadCount = 0;
  switch (dataType)
  {
  case SDT_Vertices:
    ok = _vertices.read(packet) > 0 && ok;
    break;

  case SDT_Indices:
    ok = _indices.read(packet) > 0 && ok;
    ok = ok && endReadCount != ~0u;
    break;

    // Normals handled together.
  case SDT_Normals:
    ok = _normals.read(packet) > 0 && ok;
    break;

  case SDT_Colours:
    ok = _colours.read(packet) > 0 && ok;
    break;
  case SDT_End:
    // Ensure we have zero offset and count.
    {
      uint32_t offset{};
      uint16_t count{};
      ok = ok && packet.readElement(offset) == sizeof(offset);
      ok = ok && packet.readElement(count) == sizeof(count);
      ok = ok && offset == 0 && count == 0;
    }
    break;
  default:
    // Unknown data type.
    ok = false;
    break;
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
  copy->_vertices = DataBuffer(_vertices);
  copy->_vertices.duplicate();
  copy->_normals = DataBuffer(_indices);
  copy->_normals.duplicate();
  copy->_indices = DataBuffer(_indices);
  copy->_indices.duplicate();
  copy->_colours = DataBuffer(_indices);
  copy->_colours.duplicate();
  copy->_quantisationUnit = _quantisationUnit;
  copy->_drawScale = _drawScale;
  copy->_drawType = _drawType;
}
}  // namespace tes
