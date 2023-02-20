//
// author Kazys Stepanas
//
#include "OccupancyMesh.h"

#include "p2p.h"

#ifdef TES_ENABLE
#include <3escore/Colour.h>
#include <3escore/ServerApi.h>
#include <3escore/TransferProgress.h>

using namespace tes;

struct OccupancyMeshDetail
{
  std::vector<tes::Vector3f> vertices;
  // Define the render extents for the voxels.
  std::vector<tes::Vector3f> normals;
  std::vector<uint32_t> colours;
  // std::vector<uint32_t> indices;
  /// Tracks indices of unused vertices in the vertex array.
  std::vector<uint32_t> unused_vertex_list;
  /// Maps voxel keys to their vertex indices.
  KeyToIndexMap voxel_index_map;
};

namespace
{
// bool validateVertex(const Vector3f &v)
// {
//   for (int i = 0; i < 3; ++i)
//   {
//     if (std::abs(v[i]) > 1e6f)
//     {
//       return false;
//     }
//   }
//   return true;
// }


uint32_t nodeColour(const octomap::OcTree::NodeType *node, const octomap::OcTree &map)
{
  const float intensity =
    float((node->getOccupancy() - map.getOccupancyThres()) / (1.0 - map.getOccupancyThres()));
  // const float intensity = (node) ? float(node->getOccupancy()) : 0;
  const int c = int(255 * intensity);
  return tes::Colour(c, c, c).colour32();
}
}  // namespace

OccupancyMesh::OccupancyMesh(unsigned mesh_id, octomap::OcTree &map)
  : _map(map)
  , _id(mesh_id)
  , _detail(std::make_unique<OccupancyMeshDetail>())
{
  // Expose the mesh resource.
  referenceResource(g_tes_server, shared_from_this());
}


OccupancyMesh::~OccupancyMesh()
{
  releaseResource(g_tes_server, shared_from_this());
}

uint32_t OccupancyMesh::id() const
{
  return _id;
}


tes::Transform OccupancyMesh::transform() const
{
  return tes::Transform::identity(false);
}


uint32_t OccupancyMesh::tint() const
{
  return 0xFFFFFFFFu;
}


uint8_t OccupancyMesh::drawType(int stream) const
{
  TES_UNUSED(stream);
  return tes::DtVoxels;
}


float OccupancyMesh::drawScale(int stream) const
{
  TES_UNUSED(stream);
  return static_cast<float>(_map.getResolution());
}


unsigned OccupancyMesh::vertexCount(int stream) const
{
  TES_UNUSED(stream);
  return (unsigned)_detail->vertices.size();
}


unsigned OccupancyMesh::indexCount(int stream) const
{
  TES_UNUSED(stream);
  // return (unsigned)_detail->indices.size();
  return 0;
}


DataBuffer OccupancyMesh::vertices(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer(_detail->vertices);
}


DataBuffer OccupancyMesh::indices(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer();
}

DataBuffer OccupancyMesh::normals(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer(_detail->normals);
}


DataBuffer OccupancyMesh::uvs(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer();
}


DataBuffer OccupancyMesh::colours(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer(_detail->colours);
}

std::shared_ptr<tes::Resource> OccupancyMesh::clone() const
{
  auto copy = std::make_shared<OccupancyMesh>(_id, _map);
  *copy->_detail = *_detail;
  return copy;
}


int OccupancyMesh::transfer(tes::PacketWriter &packet, unsigned byte_limit,
                            tes::TransferProgress &progress) const
{
  // Build the voxel set if required.
  if (_detail->voxel_index_map.empty())
  {
    _detail->vertices.clear();
    _detail->colours.clear();
    for (auto node = _map.begin_leafs(); node != _map.end(); ++node)
    {
      if (_map.isNodeOccupied(*node))
      {
        // Add voxel.
        _detail->voxel_index_map.insert(
          std::make_pair(node.getKey(), uint32_t(_detail->vertices.size())));
        _detail->vertices.push_back(p2p(_map.keyToCoord(node.getKey())));
        // Normals represent voxel half extents.
        _detail->normals.push_back(Vector3f(float(0.5f * _map.getResolution())));
        _detail->colours.push_back(nodeColour(&*node, _map));
      }
    }
  }

  return tes::MeshResource::transfer(packet, byte_limit, progress);
}


void OccupancyMesh::update(const UnorderedKeySet &newly_occupied, const UnorderedKeySet &newly_free,
                           const UnorderedKeySet &touched_occupied)
{
  if (newly_occupied.empty() && newly_free.empty() && touched_occupied.empty())
  {
    // Nothing to do.
    return;
  }

  if (!g_tes_server || g_tes_server->connectionCount() == 0)
  {
    // No-one to send to.
    _detail->vertices.clear();
    _detail->normals.clear();
    _detail->colours.clear();
    //_detail->indices.clear();
    _detail->unused_vertex_list.clear();
    _detail->voxel_index_map.clear();
    return;
  }

  // Start by removing freed nodes.
  size_t initial_unused_vertex_count = _detail->unused_vertex_list.size();
  std::vector<uint32_t> modified_vertices;
  for (const auto &key : newly_free)
  {
    // Resolve the index for this voxel.
    auto voxelLookup = _detail->voxel_index_map.find(key);
    if (voxelLookup != _detail->voxel_index_map.end())
    {
      // Invalidate the voxel.
      _detail->colours[voxelLookup->second] = 0u;
      _detail->unused_vertex_list.push_back(voxelLookup->second);
      modified_vertices.push_back(voxelLookup->second);
      _detail->voxel_index_map.erase(voxelLookup);
    }
  }

  // Now added occupied nodes, initially from the free list.
  size_t processed_occupied_count = 0;
  auto occupied_iter = newly_occupied.begin();
  while (!_detail->unused_vertex_list.empty() && occupied_iter != newly_occupied.end())
  {
    const uint32_t vertex_index = _detail->unused_vertex_list.back();
    const octomap::OcTreeKey key = *occupied_iter;
    const octomap::OcTree::NodeType *node = _map.search(key);
    const bool mark_as_modified = _detail->unused_vertex_list.size() <= initial_unused_vertex_count;
    _detail->unused_vertex_list.pop_back();
    ++occupied_iter;
    ++processed_occupied_count;
    _detail->vertices[vertex_index] = p2p(_map.keyToCoord(key));
    // validateVertex(_detail->vertices[vertex_index]);
    _detail->colours[vertex_index] = nodeColour(node, _map);
    _detail->voxel_index_map.insert(std::make_pair(key, vertex_index));
    // Only mark as modified if this vertex wasn't just invalidate by removal.
    // It will already be on the list otherwise.
    if (mark_as_modified)
    {
      modified_vertices.push_back(vertex_index);
    }
  }

  // Send messages for individually changed voxels.
  // Start a mesh redefinition message.
  std::vector<uint8_t> buffer(0xffffu);
  tes::PacketWriter packet(buffer.data(), (uint16_t)buffer.size());
  tes::MeshRedefineMessage msg = {};
  tes::MeshComponentMessage cmp_msg = {};
  tes::MeshFinaliseMessage final_msg = {};
  tes::ObjectAttributesd attributes = {};

  // Work out how many vertices we'll have after all modifications are done.
  size_t old_vertex_count = _detail->vertices.size();
  size_t new_vertex_count = _detail->vertices.size();
  if (newly_occupied.size() - processed_occupied_count > _detail->unused_vertex_list.size())
  {
    // We have more occupied vertices than available in the free list.
    // This means we will add new vertices.
    new_vertex_count +=
      newly_occupied.size() - processed_occupied_count - _detail->unused_vertex_list.size();
  }

  msg.mesh_id = _id;
  msg.vertex_count = (uint32_t)new_vertex_count;
  msg.index_count = 0;
  msg.draw_type = drawType(0);
  attributes.identity();

  packet.reset(tes::MtMesh, tes::MeshRedefineMessage::MessageId);
  msg.write(packet, attributes);

  packet.finalise();
  g_tes_server->send(packet);

  // Next update changed triangles.
  cmp_msg.mesh_id = id();

  const float quantisation_unit = 0.001f;
  // Update modified vertices, one at a time.
  for (uint32_t vertex_index : modified_vertices)
  {
    packet.reset(tes::MtMesh, tes::MmtVertex);
    cmp_msg.write(packet);
    DataBuffer data_buffer(_detail->vertices.data() + vertex_index, 1u);
    data_buffer.writePacked(packet, 0, quantisation_unit);
    if (packet.finalise())
    {
      g_tes_server->send(packet);
    }

    // Send colour and position update.
    packet.reset(tes::MtMesh, tes::MmtVertexColour);
    cmp_msg.write(packet);
    data_buffer = DataBuffer(_detail->colours.data() + vertex_index, 1u);
    data_buffer.write(packet, 0);
    if (packet.finalise())
    {
      g_tes_server->send(packet);
    }
  }

  // Add remaining vertices and send a bulk modification message.
  for (; occupied_iter != newly_occupied.end(); ++occupied_iter, ++processed_occupied_count)
  {
    const uint32_t vertex_index = uint32_t(_detail->vertices.size());
    const octomap::OcTreeKey key = *occupied_iter;
    _detail->voxel_index_map.insert(std::make_pair(key, vertex_index));
    //_detail->indices.push_back(uint32_t(_detail->vertices.size()));
    _detail->vertices.push_back(p2p(_map.keyToCoord(key)));
    // validateVertex(_detail->vertices.back());
    // Normals represent voxel half extents.
    _detail->normals.push_back(Vector3f(float(0.5f * _map.getResolution())));
    _detail->colours.push_back(0xffffffffu);
  }

  // Send bulk messages for new vertices.
  if (old_vertex_count != new_vertex_count)
  {
    const uint16_t transferLimit = 5001;
    // Send colour and position update.
    uint32_t offset = uint32_t(old_vertex_count);

    while (offset < new_vertex_count)
    {
      auto count = uint16_t(std::min<size_t>(transferLimit, new_vertex_count - offset));

      packet.reset(tes::MtMesh, tes::MmtVertex);
      cmp_msg.write(packet);
      DataBuffer(&_detail->vertices[offset], count)
        .writePacked(packet, 0, quantisation_unit, 0, offset);
      if (packet.finalise())
      {
        g_tes_server->send(packet);
      }

      packet.reset(tes::MtMesh, tes::MmtNormal);
      cmp_msg.write(packet);
      DataBuffer(&_detail->normals[offset], count)
        .writePacked(packet, 0, quantisation_unit, 0, offset);
      if (packet.finalise())
      {
        g_tes_server->send(packet);
      }

      packet.reset(tes::MtMesh, tes::MmtVertexColour);
      cmp_msg.write(packet);
      DataBuffer(&_detail->colours[offset], count).write(packet, 0, 0, offset);
      if (packet.finalise())
      {
        g_tes_server->send(packet);
      }

      // Calculate next batch.
      offset += count;
    }
  }

  // Update colours for touched occupied
  if (!touched_occupied.empty())
  {
    for (auto key : touched_occupied)
    {
      const octomap::OcTree::NodeType *node = _map.search(key);
      auto index_search = _detail->voxel_index_map.find(key);
      if (node && index_search != _detail->voxel_index_map.end())
      {
        const unsigned voxel_index = index_search->second;
        _detail->colours[voxel_index] = nodeColour(node, _map);

        packet.reset(tes::MtMesh, tes::MmtVertexColour);
        cmp_msg.write(packet);
        DataBuffer(&_detail->colours[voxel_index], 1).write(packet, 0, 0, voxel_index);
        if (packet.finalise())
        {
          g_tes_server->send(packet);
        }
      }
    }
  }

  // Finalise the modifications.
  final_msg.mesh_id = _id;
  // Rely on EDL shader.
  final_msg.flags = 0;  // tes::MffCalculateNormals;
  packet.reset(tes::MtMesh, final_msg.MessageId);
  final_msg.write(packet);
  packet.finalise();
  g_tes_server->send(packet);
}

#endif  // TES_ENABLE
