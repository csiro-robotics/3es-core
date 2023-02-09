//
// author: Kazys Stepanas
//
#include "MutableMesh.h"

#include "SimpleMesh.h"

//
#include <3escore/Connection.h>
#include <3escore/CoreUtil.h>
#include <3escore/PacketWriter.h>
#include <3escore/Rotation.h>
#include <3escore/Transform.h>

#include <vector>

namespace tes
{
struct VertexChange
{
  union
  {
    std::array<float, 3> position;
    std::array<float, 3> normal;
    std::array<float, 2> uv;
    uint32_t colour;
  };
  unsigned component_flag = 0;
  unsigned write_index;
};

struct IndexChange
{
  unsigned index_value;
  unsigned write_index;
};

/// Data members for MutableMesh
struct MutableMeshImp
{
  /// Current mesh.
  SimpleMesh mesh;
  std::vector<VertexChange> vertex_changes;
  std::vector<IndexChange> index_changes;
  Transform new_transform;
  unsigned new_vertex_count = ~0u;
  unsigned new_index_count = ~0u;
  uint32_t tint = 0xffffffffu;
  bool transform_dirty = false;
  bool tint_dirty = false;
  /// Is an update required?
  bool dirty = false;

  MutableMeshImp(uint32_t id, DrawType draw_type, unsigned components)
    : mesh(id, 0u, 0u, draw_type, components)
  {}
};

MutableMesh::MutableMesh(uint32_t id, DrawType draw_type, unsigned components)
  : _imp(new MutableMeshImp(id, draw_type, components))
{}

MutableMesh::~MutableMesh()
{
  delete _imp;
}

const SimpleMesh &MutableMesh::meshResource() const
{
  return _imp->mesh;
}

void MutableMesh::setTransform(const Transform &transform)
{
  _imp->new_transform = transform;
  _imp->transform_dirty = _imp->dirty = true;
}

void MutableMesh::setTint(uint32_t tint)
{
  _imp->tint = tint;
  _imp->tint_dirty = _imp->dirty = true;
}

void MutableMesh::setVertexCount(const UIntArg &count)
{
  _imp->new_vertex_count = count;
  _imp->dirty = true;
}

void MutableMesh::setIndexCount(const UIntArg &count)
{
  _imp->new_index_count = count;
  _imp->dirty = true;
}

unsigned MutableMesh::pendingVertexCount() const
{
  return (_imp->new_vertex_count != ~0u) ? _imp->new_vertex_count : _imp->mesh.vertexCount();
}

unsigned MutableMesh::pendingIndexCount() const
{
  return (_imp->new_index_count != ~0u) ? _imp->new_index_count : _imp->mesh.indexCount();
}

// void MutableMesh::eraseVertices(const UIntArg &index, const UIntArg &count)
// {
//   _imp->dirty = true;


// void MutableMesh::eraseIndices(const UIntArg &index, const UIntArg &count)
// {
//   _imp->dirty = true;
// }

unsigned MutableMesh::setVertices(const UIntArg &at, const Vector3f *v, const UIntArg &count)
{
  if ((_imp->mesh.components() & SimpleMesh::Vertex) == 0)
  {
    // Unsupported component.
    return 0;
  }

  _imp->dirty = true;
  unsigned modified = 0;
  unsigned target_index = at;
  const unsigned vertex_count = pendingVertexCount();

  VertexChange delta;
  delta.component_flag = SimpleMesh::Vertex;
  for (unsigned i = 0; i < count && target_index < vertex_count; ++i)
  {
    delta.write_index = target_index;
    delta.position[0] = v->x();
    delta.position[1] = v->y();
    delta.position[2] = v->z();
    _imp->vertex_changes.push_back(delta);
    ++v;
    ++target_index;
    ++modified;
  }

  return modified;
}

unsigned MutableMesh::setIndices(const UIntArg &at, const uint32_t *idx, const UIntArg &count)
{
  if ((_imp->mesh.components() & SimpleMesh::Index) == 0)
  {
    // Unsupported component.
    return 0;
  }

  _imp->dirty = true;
  unsigned modified = 0;
  unsigned target_index = at;
  const unsigned index_count = pendingIndexCount();

  IndexChange delta;
  for (unsigned i = 0; i < count && target_index < index_count; ++i)
  {
    delta.write_index = target_index;
    delta.index_value = *idx;
    _imp->index_changes.push_back(delta);
    ++idx;
    ++target_index;
    ++modified;
  }

  return modified;
}

unsigned MutableMesh::setNormals(const UIntArg &at, const Vector3f *n, const UIntArg &count)
{
  if ((_imp->mesh.components() & SimpleMesh::Normal) == 0)
  {
    // Unsupported component.
    return 0;
  }

  _imp->dirty = true;
  unsigned modified = 0;
  unsigned target_index = at;
  const unsigned vertex_count = pendingVertexCount();

  VertexChange delta;
  delta.component_flag = SimpleMesh::Normal;
  for (unsigned i = 0; i < count && target_index < vertex_count; ++i)
  {
    delta.write_index = target_index;
    delta.normal[0] = n->x();
    delta.normal[1] = n->y();
    delta.normal[2] = n->z();
    _imp->vertex_changes.push_back(delta);
    ++n;
    ++target_index;
    ++modified;
  }

  return modified;
}

unsigned MutableMesh::setColours(const UIntArg &at, const uint32_t *c, const UIntArg &count)
{
  if ((_imp->mesh.components() & SimpleMesh::Colour) == 0)
  {
    // Unsupported component.
    return 0;
  }

  _imp->dirty = true;
  unsigned modified = 0;
  unsigned target_index = at;
  const unsigned vertex_count = pendingVertexCount();

  VertexChange delta;
  delta.component_flag = SimpleMesh::Colour;
  for (unsigned i = 0; i < count && target_index < vertex_count; ++i)
  {
    delta.write_index = target_index;
    delta.colour = *c;
    _imp->vertex_changes.push_back(delta);
    ++c;
    ++target_index;
    ++modified;
  }

  return modified;
}

unsigned MutableMesh::setUvs(const UIntArg &at, const float *uvs, const UIntArg &count)
{
  if ((_imp->mesh.components() & SimpleMesh::Uv) == 0)
  {
    // Unsupported component.
    return 0;
  }

  _imp->dirty = true;
  unsigned modified = 0;
  unsigned target_index = at;
  const unsigned vertex_count = pendingVertexCount();

  VertexChange delta;
  delta.component_flag = SimpleMesh::Uv;
  for (unsigned i = 0; i < count && target_index < vertex_count; ++i)
  {
    delta.write_index = target_index;
    delta.uv[0] = *uvs;
    ++uvs;
    delta.uv[1] = *uvs;
    ++uvs;
    _imp->vertex_changes.push_back(delta);
    ++target_index;
    ++modified;
  }

  return modified;
}


// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void MutableMesh::update(Connection *con)
{
  if (!con || !_imp->dirty)
  {
    if (_imp->dirty)
    {
      // Null connection => no work to do. Just migrate to the mesh.
      migratePending();
    }

    return;
  }

  // Send mesh redefinition message.
  std::vector<uint8_t> buffer(0xffffu);
  PacketWriter packet(buffer.data(), int_cast<uint16_t>(buffer.size()));
  MeshRedefineMessage msg = {};
  MeshComponentMessage component_msg = {};
  MeshFinaliseMessage final_msg = {};

  // Work out how many vertices we'll have after all modifications are done.
  const unsigned new_vertex_count = pendingVertexCount();
  const unsigned new_index_count = pendingIndexCount();

  const Transform transform =
    (_imp->transform_dirty) ? _imp->new_transform : _imp->mesh.transform();

  msg.mesh_id = _imp->mesh.id();
  msg.vertex_count = new_vertex_count;
  msg.index_count = new_index_count;
  msg.draw_type = _imp->mesh.drawType(0);

  packet.reset(tes::MtMesh, tes::MeshRedefineMessage::MessageId);
  if (transform.preferDoublePrecision())
  {
    msg.flags |= McfDoublePrecision;
    ObjectAttributesd attributes;
    attributes.identity();
    attributes.colour = (_imp->tint_dirty) ? _imp->tint : _imp->mesh.tint();
    attributes.position[0] = transform.position().x();
    attributes.position[1] = transform.position().y();
    attributes.position[2] = transform.position().z();
    attributes.rotation[0] = transform.rotation().x();
    attributes.rotation[1] = transform.rotation().y();
    attributes.rotation[2] = transform.rotation().z();
    attributes.rotation[3] = transform.rotation().w();
    attributes.scale[0] = transform.scale().x();
    attributes.scale[1] = transform.scale().y();
    attributes.scale[2] = transform.scale().z();
    msg.write(packet, attributes);
  }
  else
  {
    ObjectAttributesf attributes;
    attributes.identity();
    attributes.colour = (_imp->tint_dirty) ? _imp->tint : _imp->mesh.tint();
    attributes.position[0] = static_cast<float>(transform.position().x());
    attributes.position[1] = static_cast<float>(transform.position().y());
    attributes.position[2] = static_cast<float>(transform.position().z());
    attributes.rotation[0] = static_cast<float>(transform.rotation().x());
    attributes.rotation[1] = static_cast<float>(transform.rotation().y());
    attributes.rotation[2] = static_cast<float>(transform.rotation().z());
    attributes.rotation[3] = static_cast<float>(transform.rotation().w());
    attributes.scale[0] = static_cast<float>(transform.scale().x());
    attributes.scale[1] = static_cast<float>(transform.scale().y());
    attributes.scale[2] = static_cast<float>(transform.scale().z());
    msg.write(packet, attributes);
  }

  packet.finalise();
  con->send(packet);

  component_msg.mesh_id = _imp->mesh.id();

  // It would be nice to sort additions/removals to support block updates,
  // however, changes may be interleaved so we have to preserve order.

  if (!_imp->vertex_changes.empty())
  {
    // Process new vertices.
    for (const auto &vertex_def : _imp->vertex_changes)
    {
      if (vertex_def.component_flag & SimpleMesh::Vertex)
      {
        packet.reset(tes::MtMesh, tes::MmtVertex);
        component_msg.write(packet);
        const DataBuffer write_buffer(vertex_def.position.data(), 1, 3);
        write_buffer.write(packet, 0, vertex_def.write_index);
        packet.finalise();
        con->send(packet);
      }

      if (vertex_def.component_flag & SimpleMesh::Colour)
      {
        packet.reset(tes::MtMesh, tes::MmtVertexColour);
        component_msg.write(packet);
        const DataBuffer write_buffer(&vertex_def.colour, 1);
        write_buffer.write(packet, 0, vertex_def.write_index);
        packet.finalise();
        con->send(packet);
      }

      if (vertex_def.component_flag & SimpleMesh::Normal)
      {
        packet.reset(tes::MtMesh, tes::MmtNormal);
        component_msg.write(packet);
        const DataBuffer write_buffer(vertex_def.normal.data(), 1, 3);
        write_buffer.write(packet, 0, vertex_def.write_index);
        packet.finalise();
        con->send(packet);
      }

      if (vertex_def.component_flag & SimpleMesh::Uv)
      {
        packet.reset(tes::MtMesh, tes::MmtUv);
        component_msg.write(packet);
        const DataBuffer write_buffer(vertex_def.uv.data(), 1, 2);
        write_buffer.write(packet, 0, vertex_def.write_index);
        packet.finalise();
        con->send(packet);
      }
    }
  }

  if (!_imp->index_changes.empty())
  {
    // Process new indices.
    for (const auto &index_def : _imp->index_changes)
    {
      packet.reset(tes::MtMesh, tes::MmtIndex);
      component_msg.write(packet);
      const DataBuffer write_buffer(&index_def.index_value, 1);
      write_buffer.write(packet, 0, index_def.write_index);
      packet.finalise();
      con->send(packet);
    }
  }

  migratePending();

  // Finalise the modifications.
  final_msg.mesh_id = _imp->mesh.id();
  // Rely on EDL shader.
  final_msg.flags = 0;  // tes::MffCalculateNormals;
  packet.reset(tes::MtMesh, final_msg.MessageId);
  final_msg.write(packet);
  packet.finalise();
  con->send(packet);
}


void MutableMesh::migratePending()
{
  const unsigned new_vertex_count = pendingVertexCount();
  const unsigned new_index_count = pendingIndexCount();

  _imp->mesh.setVertexCount(new_vertex_count);
  _imp->mesh.setIndexCount(new_index_count);

  if (_imp->transform_dirty)
  {
    _imp->mesh.setTransform(_imp->new_transform);
  }

  if (_imp->tint_dirty)
  {
    _imp->mesh.setTint(_imp->tint);
  }

  for (const auto &vertex_def : _imp->vertex_changes)
  {
    if (vertex_def.component_flag & SimpleMesh::Vertex)
    {
      _imp->mesh.setVertex(vertex_def.write_index, vertex_def.position);
    }

    if (vertex_def.component_flag & SimpleMesh::Colour)
    {
      _imp->mesh.setColour(vertex_def.write_index, vertex_def.colour);
    }

    if (vertex_def.component_flag & SimpleMesh::Normal)
    {
      _imp->mesh.setNormal(vertex_def.write_index, vertex_def.normal);
    }

    if (vertex_def.component_flag & SimpleMesh::Uv)
    {
      _imp->mesh.setUv(vertex_def.write_index, vertex_def.uv[0], vertex_def.uv[1]);
    }
  }

  for (const auto &index_def : _imp->index_changes)
  {
    _imp->mesh.setIndex(index_def.write_index, index_def.index_value);
  }

  _imp->vertex_changes.clear();
  _imp->index_changes.clear();

  _imp->new_vertex_count = _imp->new_index_count = ~0u;

  _imp->transform_dirty = false;
  _imp->dirty = false;
}
}  // namespace tes
