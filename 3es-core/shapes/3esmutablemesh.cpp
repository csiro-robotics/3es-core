//
// author: Kazys Stepanas
//
#include "3esmutablemesh.h"

#include "3esconnection.h"
#include "3espacketwriter.h"
#include "3esrotation.h"
#include "3essimplemesh.h"

#include <vector>

using namespace tes;

namespace tes
{
struct VertexChange
{
  union
  {
    float position[3];
    float  normal[3];
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
  Matrix4f newTransform;
  unsigned newVertexCount = ~0;
  unsigned newIndexCount = ~0;
  bool transformDirty = false;
  /// Is an update required?
  bool dirty = false;

  MutableMeshImp(uint32_t id, DrawType drawType, unsigned components)
    : mesh(id, drawType, components)
  {
  }
};
} // namespace tes

MutableMesh::MutableMesh(uint32_t id, DrawType drawType, unsigned components)
    : _imp(new MutableMeshImp(id, drawType, components))
{
}

MutableMesh::~MutableMesh()
{
  delete _imp;
}

const SimpleMesh &MutableMesh::meshResource() const
{
  return _imp->mesh;
}

void MutableMesh::setTransform(const Matrix4f &transform)
{
  _imp->newTransform = transform;
  _imp->transformDirty = true;
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
  if ((_imp->mesh.components() | SimpleMesh::Vertex) == 0)
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
    delta.position[0] = v->x;
    delta.position[1] = v->y;
    delta.position[2] = v->z;
    _imp->vertexChanges.push_back(delta);
    ++v;
    ++targetIndex;
    ++modified;
  }

  return modified;
}

unsigned MutableMesh::setIndices(const UIntArg &at, const uint32_t *idx, const UIntArg &count)
{
  if ((_imp->mesh.components() | SimpleMesh::Index) == 0)
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
  if ((_imp->mesh.components() | SimpleMesh::Normal) == 0)
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
    delta.normal[0] = n->x;
    delta.normal[1] = n->y;
    delta.normal[2] = n->z;
    _imp->vertexChanges.push_back(delta);
    ++n;
    ++targetIndex;
    ++modified;
  }

  return modified;
}

unsigned MutableMesh::setColours(const UIntArg &at, const uint32_t *c, const UIntArg &count)
{
  if ((_imp->mesh.components() | SimpleMesh::Colour) == 0)
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
  if ((_imp->mesh.components() | SimpleMesh::Uv) == 0)
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
  tes::PacketWriter packet(buffer.data(), (uint16_t)buffer.size());
  tes::MeshRedefineMessage msg;
  tes::MeshComponentMessage cmpmsg;
  tes::MeshFinaliseMessage finalmsg;

  // Work out how many vertices we'll have after all modifications are done.
  const unsigned newVertexCount = pendingVertexCount();
  const unsigned newIndexCount = pendingIndexCount();

  const Matrix4f transform = (_imp->transformDirty) ? _imp->newTransform : _imp->mesh.transform();
  Vector3f translation;
  Vector3f scale;
  Quaternionf rotation;

  transformToQuaternionTranslation(transform, rotation, translation, &scale);

  msg.meshId = _imp->mesh.id();
  msg.vertexCount = newVertexCount;
  msg.indexCount = newIndexCount;
  msg.drawType = _imp->mesh.drawType(0);
  msg.attributes.identity();
  msg.attributes.colour = _imp->mesh.tint();
  msg.attributes.position[0] = translation.x;
  msg.attributes.position[1] = translation.y;
  msg.attributes.position[2] = translation.z;
  msg.attributes.rotation[0] = rotation.x;
  msg.attributes.rotation[1] = rotation.y;
  msg.attributes.rotation[2] = rotation.z;
  msg.attributes.rotation[3] = rotation.w;
  msg.attributes.scale[0] = scale.x;
  msg.attributes.scale[1] = scale.y;
  msg.attributes.scale[2] = scale.z;

  packet.reset(tes::MtMesh, tes::MeshRedefineMessage::MessageId);
  msg.write(packet);

  packet.finalise();
  con->send(packet);

  cmpmsg.meshId = _imp->mesh.id();
  cmpmsg.reserved = 0;
  cmpmsg.count = 1;

  // It would be nice to sort additions/removals to support block updates,
  // however, changes may be interleaved so we have to preserve order.

  if (!_imp->vertexChanges.empty())
  {
    // Process new vertices.
    for (size_t i = 0; i < _imp->vertexChanges.size(); ++i)
    {
      const VertexChange &vertexDef = _imp->vertexChanges[i];
      cmpmsg.offset = vertexDef.writeIndex;
      cmpmsg.count = 1;

      if (vertexDef.componentFlag | SimpleMesh::Vertex)
      {
        packet.reset(tes::MtMesh, tes::MmtVertex);
        cmpmsg.write(packet);

        // Write the vertex value.
        packet.writeArray<float>(vertexDef.position, 3);

        packet.finalise();
        con->send(packet);
      }

      if (vertexDef.componentFlag | SimpleMesh::Colour)
      {
        packet.reset(tes::MtMesh, tes::MmtVertexColour);
        cmpmsg.write(packet);

        // Write the vertex value.
        packet.writeElement<uint32_t>(vertexDef.colour);

        packet.finalise();
        con->send(packet);
      }

      if (vertexDef.componentFlag | SimpleMesh::Normal)
      {
        packet.reset(tes::MtMesh, tes::MmtNormal);
        cmpmsg.write(packet);

        // Write the vertex value.
        packet.writeArray<float>(vertexDef.normal, 3);

        packet.finalise();
        con->send(packet);
      }

      if (vertexDef.componentFlag | SimpleMesh::Uv)
      {
        packet.reset(tes::MtMesh, tes::MmtUv);
        cmpmsg.write(packet);

        // Write the vertex value.
        packet.writeArray<float>(vertexDef.uv, 2);

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
      cmpmsg.offset = indexDef.writeIndex;
      cmpmsg.count = 1;

      packet.reset(tes::MtMesh, tes::MmtIndex);
      cmpmsg.write(packet);

      // Write the vertex value.
      packet.writeElement<unsigned>(indexDef.indexValue);

      packet.finalise();
      con->send(packet);
    }
  }

  migratePending();

  // Finalise the modifications.
  finalmsg.meshId = _imp->mesh.id();
  // Rely on EDL shader.
  finalmsg.flags = 0; // tes::MbfCalculateNormals;
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

  for (size_t i = 0; i < _imp->vertexChanges.size(); ++i)
  {
    const VertexChange &vertexDef = _imp->vertexChanges[i];

    if (vertexDef.componentFlag | SimpleMesh::Vertex)
    {
      _imp->mesh.setVertex(vertexDef.writeIndex, vertexDef.position);
    }

    if (vertexDef.componentFlag | SimpleMesh::Colour)
    {
      _imp->mesh.setColour(vertexDef.writeIndex, vertexDef.colour);
    }

    if (vertexDef.componentFlag | SimpleMesh::Normal)
    {
      _imp->mesh.setNormal(vertexDef.writeIndex, vertexDef.normal);
    }

    if (vertexDef.componentFlag | SimpleMesh::Uv)
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
