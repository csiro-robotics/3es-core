//
// author Kazys Stepanas
//
#include "Occupancy.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

// Disable various OCTOMAP warnings
#ifdef _MSC_VER
// C4267: conversion from size_t to unsigned int
// 4244: conversion from double to float
#pragma warning(disable : 4244 4267)
#endif  // _MSC_VER
#include <octomap/octomap.h>

#ifdef TES_ENABLE
#include <3escore/shapes/MeshResource.h>
#endif  // TES_ENABLE

#include <memory>

using KeyToIndexMap = std::unordered_map<octomap::OcTreeKey, uint32_t, octomap::OcTreeKey::KeyHash>;
using UnorderedKeySet = std::unordered_set<octomap::OcTreeKey, octomap::OcTreeKey::KeyHash>;
using KeyArray = std::vector<octomap::OcTreeKey>;
class OccupancyMesh;

#ifdef TES_ENABLE
struct OccupancyMeshDetail;

/// Defines and maintains a 3rd Eye Scene mesh resource based on an octomap.
///
/// Renders as a point cloud of occupied voxels.
class OccupancyMesh : public tes::MeshResource, std::enable_shared_from_this<OccupancyMesh>
{
public:
  OccupancyMesh(unsigned mesh_id, octomap::OcTree &map);
  ~OccupancyMesh();

  uint32_t id() const final;
  tes::Transform transform() const final;
  uint32_t tint() const final;
  uint8_t drawType(int stream) const final;

  unsigned vertexCount(int stream) const final;
  unsigned indexCount(int stream) const final;

  tes::DataBuffer vertices(int stream) const final;
  tes::DataBuffer indices(int stream) const final;
  tes::DataBuffer normals(int stream) const final;
  tes::DataBuffer uvs(int stream) const final;
  tes::DataBuffer colours(int stream) const final;

  std::shared_ptr<tes::Resource> clone() const final;

  int transfer(tes::PacketWriter &packet, unsigned byteLimit,
               tes::TransferProgress &progress) const final;

  /// Updates noted changes to the debug view.
  /// @param occupiedChange Keys of voxels which have become occupied from free or uncertain since
  /// the last update.
  /// @param newlyFree Keys of voxels which have become free from occupied since the last update.
  /// @param touchedOccupied Keys of voxels which have changed occupied probability.
  void update(const UnorderedKeySet &newlyOccupied, const UnorderedKeySet &newlyFree,
              const UnorderedKeySet &touchedOccupied);

private:
  typedef uint32_t IndexType;

  octomap::OcTree &_map;
  uint32_t _id;

  std::unique_ptr<OccupancyMeshDetail> _detail;
};

#endif  // TES_ENABLE
