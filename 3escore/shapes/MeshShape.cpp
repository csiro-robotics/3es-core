//
// author: Kazys Stepanas
//
#include "MeshShape.h"

#include "MeshResource.h"

#include <3escore/CoreUtil.h>
#include <3escore/PacketWriter.h>

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


std::shared_ptr<tes::Resource> MeshShape::Resource::clone() const
{
  return std::make_shared<MeshShape::Resource>(_shape, _resource_id);
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
  (void)stream;
  return _shape.drawType();
}


float MeshShape::Resource::drawScale(int stream) const
{
  (void)stream;
  return _shape.drawScale();
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


bool MeshShape::Resource::readTransfer(int message_type, PacketReader &packet)
{
  (void)message_type;
  (void)packet;
  return false;
}


MeshShape::MeshShape(const MeshShape &other)
  : Shape(other)
{
  other.onClone(*this);
}


MeshShape::MeshShape(MeshShape &&other) noexcept
  : Shape(other)
  , _vertices(std::move(other._vertices))
  , _normals(std::move(other._normals))
  , _colours(std::move(other._colours))
  , _indices(std::move(other._indices))
  , _quantisation_unit(std::exchange(other._quantisation_unit, 0.0))
  , _draw_scale(std::exchange(other._draw_scale, 0.0f))
  , _draw_type(std::exchange(other._draw_type, DtPoints))
{}


MeshShape::~MeshShape() = default;


MeshShape &MeshShape::setNormals(const DataBuffer &normals)
{
  setCalculateNormals(false);
  _normals = normals;
  return *this;
}


MeshShape &MeshShape::setUniformNormal(const Vector3f &normal)
{
  setCalculateNormals(false);
  const std::array<Vector3f, 1> source = { normal };
  _normals = std::move(DataBuffer(source));
  _normals.duplicate();
  return *this;
}


template <typename T>
void expandVertices(DataBuffer &vertices, DataBuffer &indices)
{
  if (vertices.isValid())
  {
    // First unpack all vertices and stop indexing.
    const size_t element_count = static_cast<size_t>(vertices.componentCount()) * indices.count();
    T *verts = new T[element_count];
    // Set vertices to hold the new pointer as soon as we can for RAII
    vertices.set(verts, indices.count(), vertices.componentCount(), vertices.componentCount(),
                 true);

    T *dst = verts;
    for (unsigned i = 0; i < indices.count(); ++i)
    {
      for (unsigned j = 0; j < vertices.componentCount(); ++j)
      {
        const unsigned vind = *indices.ptr<unsigned>(i);
        *dst = vertices.ptr<T>(vind)[j];
      }
    }
  }
}


MeshShape &MeshShape::expandVertices()
{
  if (_indices.count() == 0)
  {
    return duplicateArrays();
  }

  // Drop index / in favour of an expanded vertex array. We will end up owning all array memory with
  // a null index array.

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
  ok = packet.writeElement(_draw_scale) == sizeof(_draw_scale) && ok;
  const uint8_t draw_type = _draw_type;
  ok = packet.writeElement(draw_type) == sizeof(draw_type) && ok;
  return ok;
}


int MeshShape::writeData(PacketWriter &packet, unsigned &progress_marker) const
{
  bool ok = true;
  DataMessage msg;
  msg.id = _data.id;
  packet.reset(routingId(), DataMessage::MessageId);
  ok = msg.write(packet);

  // Send vertices or indices?
  uint32_t offset;

  // Resolve what we are currently sending.
  unsigned phase_index = 0;
  unsigned previous_phase_offset = 0;

  // Order to send data in and information required to automate sending.
  const std::array<DataPhase, 4> phases = { DataPhase{ SDTVertices, _vertices.type(), &_vertices },
                                            DataPhase{ SDTIndices, _indices.type(), &_indices },
                                            DataPhase{ SDTNormals, _normals.type(), &_normals },
                                            DataPhase{ SDTColours, _colours.type(), &_colours } };

  // While progress_marker is greater than or equal to the sum of the previous phase counts and the
  // current phase count. Also terminate of out of phases.
  while (phase_index < phases.size() &&
         progress_marker >= previous_phase_offset + phases[phase_index].stream->count())
  {
    previous_phase_offset += phases[phase_index].stream->count();
    ++phase_index;
  }

  offset = progress_marker - previous_phase_offset;

  bool done = false;
  unsigned write_count = 0;
  switch (phase_index)
  {
  case SDTVertices:
    ok = packet.writeElement(static_cast<uint16_t>(phase_index)) == sizeof(uint16_t) && ok;
    if (_quantisation_unit > 0)
    {
      write_count = _vertices.writePacked(packet, offset, _quantisation_unit);
    }
    else
    {
      write_count = _vertices.write(packet, offset);
    }
    break;
  case SDTIndices:
    ok = packet.writeElement(static_cast<uint16_t>(phase_index)) == sizeof(uint16_t) && ok;
    write_count = _indices.write(packet, offset);
    break;
  case SDTNormals:
    ok = packet.writeElement(static_cast<uint16_t>(phase_index)) == sizeof(uint16_t) && ok;
    if (_quantisation_unit > 0)
    {
      write_count = _normals.writePacked(packet, offset, 1.0f / static_cast<float>(0xffff));
    }
    else
    {
      write_count = _normals.write(packet, offset);
    }
    break;
  case SDTColours:
    ok = packet.writeElement(static_cast<uint16_t>(phase_index)) == sizeof(uint16_t) && ok;
    write_count = _colours.write(packet, offset);
    break;

  default:
    // Either all done or no data to send.
    ok = packet.writeElement(static_cast<uint16_t>(SDTEnd)) == sizeof(uint16_t) && ok;
    // Write zero offset (4-bytes) and count (2-bytes) for consistency.
    ok = packet.writeElement(static_cast<uint32_t>(0)) == sizeof(uint32_t) && ok;
    ok = packet.writeElement(static_cast<uint16_t>(0)) == sizeof(uint16_t) && ok;
    done = true;
    break;
  }

  progress_marker += write_count;
  ok = (done || write_count > 0) && ok;

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

  uint32_t vertex_count = 0;
  uint32_t index_count = 0;
  uint8_t draw_type = 0;
  bool ok = true;

  ok = ok && packet.readElement(vertex_count) == sizeof(vertex_count);
  ok = ok && packet.readElement(index_count) == sizeof(index_count);
  ok = ok && packet.readElement(_draw_scale) == sizeof(_draw_scale);
  ok = ok && packet.readElement(draw_type) == sizeof(draw_type);
  _draw_type = static_cast<DrawType>(draw_type);

  _vertices.set(static_cast<float *>(nullptr), 0, 3);
  _normals.set(static_cast<float *>(nullptr), 0, 3);
  _indices.set(static_cast<uint32_t *>(nullptr), 0);
  _colours.set(static_cast<uint32_t *>(nullptr), 0);

  return ok;
}


bool MeshShape::readData(PacketReader &packet)
{
  DataMessage msg;
  uint16_t data_type = 0;
  bool ok = true;

  ok = ok && msg.read(packet);
  ok = ok && packet.readElement(data_type) == sizeof(data_type);

  // Record and mask out end flags.
  data_type = data_type;

  switch (data_type)
  {
  case SDTVertices:
    ok = _vertices.read(packet) > 0 && ok;
    break;

  case SDTIndices:
    ok = _indices.read(packet) > 0 && ok;
    break;

    // Normals handled together.
  case SDTNormals:
    ok = _normals.read(packet) > 0 && ok;
    break;

  case SDTColours:
    ok = _colours.read(packet) > 0 && ok;
    break;
  case SDTEnd:
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


std::shared_ptr<Shape> MeshShape::clone() const
{
  auto triangles = std::make_shared<MeshShape>();
  onClone(*triangles);
  triangles->_data = _data;
  return triangles;
}


void MeshShape::onClone(MeshShape &copy) const
{
  Shape::onClone(copy);
  copy._vertices = DataBuffer(_vertices);
  copy._vertices.duplicate();
  copy._normals = DataBuffer(_indices);
  copy._normals.duplicate();
  copy._indices = DataBuffer(_indices);
  copy._indices.duplicate();
  copy._colours = DataBuffer(_indices);
  copy._colours.duplicate();
  copy._quantisation_unit = _quantisation_unit;
  copy._draw_scale = _draw_scale;
  copy._draw_type = _draw_type;
}
}  // namespace tes
