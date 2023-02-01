//
// author: Kazys Stepanas
//
#include "MutableMesh.h"

#include "SimpleMesh.h"

//
#include <3escore/Connection.h>
#include <3escore/PacketWriter.h>
#include <3escore/Rotation.h>
#include <3escore/Transform.h>

#include <vector>

using namespace tes;

namespace tes
{
struct VertexChange
{
  union
  {
    float position[3];
    float normal[3];
    float uv[2];
    uint32_t colour;
  };
  unsigned componentFlag = 0;
  unsigned writeIndex;
};

struct IndexChange
{
  unsigned indexValue;
  unsigned writeIndex;
};

/// Data members for MutableMesh
struct MutableMeshImp
{
  /// Current mesh.
  SimpleMesh mesh;
  std::vector<VertexChange> vertexChanges;
  std::vector<IndexChange> indexChanges;
  Transform newTransform;
  unsigned newVertexCount = ~0;
  unsigned newIndexCount = ~0;
  uint32_t tint = 0xffffffffu;
  bool transformDirty = false;
  bool tintDirty = false;
  /// Is an update required?
  bool dirty = false;

  MutableMeshImp(uint32_t id, DrawType drawType, unsigned components)
    : mesh(id, 0u, 0u, drawType, components)
  {}
};
}  // namespace tes

MutableMesh::MutableMesh(uint32_t id, DrawType drawType, unsigned components)
  : _imp(new MutableMeshImp(id, drawType, components))
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
  _imp->newTransform = transform;
  _imp->transformDirty = _imp->dirty = true;
}

void MutableMesh::setTint(uint32_t tint)
{
  _imp->tint = tint;
  _imp->tintDirty = _imp->dirty = true;
}

void MutableMesh::setVertexCount(const UIntArg &count)
{
  _imp->newVertexCount = count;
  _imp->dirty = true;
}

void MutableMesh::setIndexCount(const UIntArg &count)
{
  _imp->newIndexCount = count;
  _imp->dirty = true;
}

unsigned MutableMesh::pendingVertexCount() const
{
  return (_imp->newVertexCount != ~0u) ? _imp->newVertexCount : _imp->mesh.vertexCount();
}

unsigned MutableMesh::pendingIndexCount() const
{
  return (_imp->newIndexCount != ~0u) ? _imp->newIndexCount : _imp->mesh.indexCount();
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
  unsigned targetIndex = at;
  const unsigned vertexCount = pendingVertexCount();

  VertexChange delta;
  delta.componentFlag = SimpleMesh::Vertex;
  for (unsigned i = 0; i < count && targetIndex < vertexCount; ++i)
  {
    delta.writeIndex = targetIndex;
    delta.position[0] = v->x();
    delta.position[1] = v->y();
    delta.position[2] = v->z();
    _imp->vertexChanges.push_back(delta);
    ++v;
    ++targetIndex;
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
  unsigned targetIndex = at;
  const unsigned indexCount = pendingIndexCount();

  IndexChange delta;
  for (unsigned i = 0; i < count && targetIndex < indexCount; ++i)
  {
    delta.writeIndex = targetIndex;
    delta.indexValue = *idx;
    _imp->indexChanges.push_back(delta);
    ++idx;
    ++targetIndex;
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
  unsigned targetIndex = at;
  const unsigned vertexCount = pendingVertexCount();

  VertexChange delta;
  delta.componentFlag = SimpleMesh::Normal;
  for (unsigned i = 0; i < count && targetIndex < vertexCount; ++i)
  {
    delta.writeIndex = targetIndex;
    delta.normal[0] = n->x();
    delta.normal[1] = n->y();
    delta.normal[2] = n->z();
    _imp->vertexChanges.push_back(delta);
    ++n;
    ++targetIndex;
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
  unsigned targetIndex = at;
  const unsigned vertexCount = pendingVertexCount();

  VertexChange delta;
  delta.componentFlag = SimpleMesh::Colour;
  for (unsigned i = 0; i < count && targetIndex < vertexCount; ++i)
  {
    delta.writeIndex = targetIndex;
    delta.colour = *c;
    _imp->vertexChanges.push_back(delta);
    ++c;
    ++targetIndex;
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
  unsigned targetIndex = at;
  const unsigned vertexCount = pendingVertexCount();

  VertexChange delta;
  delta.componentFlag = SimpleMesh::Uv;
  for (unsigned i = 0; i < count && targetIndex < vertexCount; ++i)
  {
    delta.writeIndex = targetIndex;
    delta.uv[0] = *uvs;
    ++uvs;
    delta.uv[1] = *uvs;
    ++uvs;
    _imp->vertexChanges.push_back(delta);
    ++targetIndex;
    ++modified;
  }

  return modified;
}

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
  PacketWriter packet(buffer.data(), (uint16_t)buffer.size());
  MeshRedefineMessage msg;
  MeshComponentMessage cmpmsg;
  MeshFinaliseMessage finalmsg;

  // Work out how many vertices we'll have after all modifications are done.
  const unsigned newVertexCount = pendingVertexCount();
  const unsigned newIndexCount = pendingIndexCount();

  const Transform transform = (_imp->transformDirty) ? _imp->newTransform : _imp->mesh.transform();

  msg.meshId = _imp->mesh.id();
  msg.vertexCount = newVertexCount;
  msg.indexCount = newIndexCount;
  msg.drawType = _imp->mesh.drawType(0);

  packet.reset(tes::MtMesh, tes::MeshRedefineMessage::MessageId);
  if (transform.preferDoublePrecision())
  {
    msg.flags |= McfDoublePrecision;
    ObjectAttributesd attributes;
    attributes.identity();
    attributes.colour = (_imp->tintDirty) ? _imp->tint : _imp->mesh.tint();
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
    attributes.colour = (_imp->tintDirty) ? _imp->tint : _imp->mesh.tint();
    attributes.position[0] = float(transform.position().x());
    attributes.position[1] = float(transform.position().y());
    attributes.position[2] = float(transform.position().z());
    attributes.rotation[0] = float(transform.rotation().x());
    attributes.rotation[1] = float(transform.rotation().y());
    attributes.rotation[2] = float(transform.rotation().z());
    attributes.rotation[3] = float(transform.rotation().w());
    attributes.scale[0] = float(transform.scale().x());
    attributes.scale[1] = float(transform.scale().y());
    attributes.scale[2] = float(transform.scale().z());
    msg.write(packet, attributes);
  }

  packet.finalise();
  con->send(packet);

  cmpmsg.meshId = _imp->mesh.id();

  // It would be nice to sort additions/removals to support block updates,
  // however, changes may be interleaved so we have to preserve order.

  if (!_imp->vertexChanges.empty())
  {
    // Process new vertices.
    for (size_t i = 0; i < _imp->vertexChanges.size(); ++i)
    {
      const VertexChange &vertexDef = _imp->vertexChanges[i];

      if (vertexDef.componentFlag & SimpleMesh::Vertex)
      {
        packet.reset(tes::MtMesh, tes::MmtVertex);
        cmpmsg.write(packet);
        DataBuffer writeBuffer(vertexDef.position, 1, 3);
        writeBuffer.write(packet, 0, vertexDef.writeIndex);
        packet.finalise();
        con->send(packet);
      }

      if (vertexDef.componentFlag & SimpleMesh::Colour)
      {
        packet.reset(tes::MtMesh, tes::MmtVertexColour);
        cmpmsg.write(packet);
        DataBuffer writeBuffer(&vertexDef.colour, 1);
        writeBuffer.write(packet, 0, vertexDef.writeIndex);
        packet.finalise();
        con->send(packet);
      }

      if (vertexDef.componentFlag & SimpleMesh::Normal)
      {
        packet.reset(tes::MtMesh, tes::MmtNormal);
        cmpmsg.write(packet);
        DataBuffer writeBuffer(vertexDef.normal, 1, 3);
        writeBuffer.write(packet, 0, vertexDef.writeIndex);
        packet.finalise();
        con->send(packet);
      }

      if (vertexDef.componentFlag & SimpleMesh::Uv)
      {
        packet.reset(tes::MtMesh, tes::MmtUv);
        cmpmsg.write(packet);
        DataBuffer writeBuffer(vertexDef.uv, 1, 2);
        writeBuffer.write(packet, 0, vertexDef.writeIndex);
        packet.finalise();
        con->send(packet);
      }
    }
  }

  if (!_imp->indexChanges.empty())
  {
    // Process new indices.
    for (size_t i = 0; i < _imp->indexChanges.size(); ++i)
    {
      const IndexChange &indexDef = _imp->indexChanges[i];
      packet.reset(tes::MtMesh, tes::MmtIndex);
      cmpmsg.write(packet);
      DataBuffer writeBuffer(&indexDef.indexValue, 1);
      writeBuffer.write(packet, 0, indexDef.writeIndex);
      packet.finalise();
      con->send(packet);
    }
  }

  migratePending();

  // Finalise the modifications.
  finalmsg.meshId = _imp->mesh.id();
  // Rely on EDL shader.
  finalmsg.flags = 0;  // tes::MffCalculateNormals;
  packet.reset(tes::MtMesh, finalmsg.MessageId);
  finalmsg.write(packet);
  packet.finalise();
  con->send(packet);
}


void MutableMesh::migratePending()
{
  const unsigned newVertexCount = pendingVertexCount();
  const unsigned newIndexCount = pendingIndexCount();

  _imp->mesh.setVertexCount(newVertexCount);
  _imp->mesh.setIndexCount(newIndexCount);

  if (_imp->transformDirty)
  {
    _imp->mesh.setTransform(_imp->newTransform);
  }

  if (_imp->tintDirty)
  {
    _imp->mesh.setTint(_imp->tint);
  }

  for (size_t i = 0; i < _imp->vertexChanges.size(); ++i)
  {
    const VertexChange &vertexDef = _imp->vertexChanges[i];

    if (vertexDef.componentFlag & SimpleMesh::Vertex)
    {
      _imp->mesh.setVertex(vertexDef.writeIndex, vertexDef.position);
    }

    if (vertexDef.componentFlag & SimpleMesh::Colour)
    {
      _imp->mesh.setColour(vertexDef.writeIndex, vertexDef.colour);
    }

    if (vertexDef.componentFlag & SimpleMesh::Normal)
    {
      _imp->mesh.setNormal(vertexDef.writeIndex, vertexDef.normal);
    }

    if (vertexDef.componentFlag & SimpleMesh::Uv)
    {
      _imp->mesh.setUv(vertexDef.writeIndex, vertexDef.uv[0], vertexDef.uv[1]);
    }
  }

  for (size_t i = 0; i < _imp->indexChanges.size(); ++i)
  {
    const IndexChange &indexDef = _imp->indexChanges[i];
    _imp->mesh.setIndex(indexDef.writeIndex, indexDef.indexValue);
  }

  _imp->vertexChanges.clear();
  _imp->indexChanges.clear();

  _imp->newVertexCount = _imp->newIndexCount = ~0u;

  _imp->transformDirty = false;
  _imp->dirty = false;
}
