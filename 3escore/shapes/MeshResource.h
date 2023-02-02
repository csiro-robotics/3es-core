//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SHAPES_MESH_RESOURCE_H
#define TES_CORE_SHAPES_MESH_RESOURCE_H

#include <3escore/CoreConfig.h>

#include <3escore/DataBuffer.h>
#include <3escore/Resource.h>
#include <3escore/Transform.h>

namespace tes
{
struct MeshCreateMessage;
struct MeshComponentMessage;
template <typename real>
struct ObjectAttributes;

/// Represents a mesh part or object. These are visualised via @c MeshSet, which may contain several
/// @c MeshResource parts.
///
/// @todo Update to support double precision vertices and normals including quantised transfer.
/// The mesh creation already does respect the @c transform() flag @c
/// Transform::preferDoublePrecision() .
class TES_CORE_API MeshResource : public Resource
{
public:
  /// Virtual destructor.
  virtual ~MeshResource() {}

  /// Estimate the number of elements which can be transferred at the given @p byteLimit.
  /// @param elementSize The byte size of each element.
  /// @param byteLimit The maximum number of bytes to transfer. Note: a hard limit of 0xffff,
  ///   packet header size and CRC is enforced. Zero indices use of the hard limit.
  /// @param Additional byte overhead to account for.
  static uint16_t estimateTransferCount(size_t elementSize, unsigned byteLimit,
                                        unsigned overhead = 0);

  /// Returns @c MtMesh
  uint16_t typeId() const override;

  virtual Transform transform() const = 0;
  virtual uint32_t tint() const = 0;

  /// Returns the @c DrawType of the mesh.
  /// @param stream Reserved for future use.
  virtual uint8_t drawType(int stream = 0) const = 0;

  /// Returns the number of vertices in the mesh.
  /// @param stream Reserved for future use.
  /// @return The number of vertices.
  virtual unsigned vertexCount(int stream = 0) const = 0;

  /// Returns the number of indices in the mesh.
  /// @param stream Reserved for future use.
  /// @return The number of indices.
  virtual unsigned indexCount(int stream = 0) const = 0;

  /// Returns a pointer to the vertex stream. Each element is taken
  /// as a triple of single precision floats: (x, y, z).
  /// @param stream Reserved for future use.
  /// @return A @c DataBuffer wrapper around the vertex memory. Must be of type [ @c DctFloat32, @c
  /// DctFloat64, @c DctPackedFloat16, @c DctPackedFloat32 ] with a @c componentCount() of 3.
  virtual DataBuffer vertices(int stream = 0) const = 0;

  /// Returns a pointer to the index stream. Supports different index widths.
  /// Expects @c indexCount(stream) elements or null if no indices.
  /// @param stream Reserved for future use.
  /// @return A @c DataBuffer wrapper around the vertex memory. Must be an integer type.
  virtual DataBuffer indices(int stream = 0) const = 0;

  /// Returns a pointer to the normal stream. Each element is taken
  /// as a triple of single precision floats: (x, y, z). Expects
  /// @c vertexColour(stream) elements or null if no normals.
  /// @param stream Reserved for future use.
  /// @return A @c DataBuffer wrapper around the vertex memory. Must be of type [ @c DctFloat32, @c
  /// DctFloat64, @c DctPackedFloat16, @c DctPackedFloat32 ] with a @c componentCount() of 3.
  virtual DataBuffer normals(int stream = 0) const = 0;

  /// Returns a pointer to the UV stream. Each element is taken
  /// as a pair of single precision floats: (u, v). Expects
  /// @c vertexCount(stream) elements or null if no UVs.
  /// @param stream Reserved for future use.
  /// @return A @c DataBuffer wrapper around the vertex memory. Must be of type [ @c DctFloat32, @c
  /// DctFloat64, @c DctPackedFloat16, @c DctPackedFloat32 ] with a @c componentCount() of 2.
  virtual DataBuffer uvs(int stream = 0) const = 0;

  /// Returns a pointer to the colour stream. Each element is taken
  /// 32-bit integer. Expects  @c vertexCount(stream) elements or null
  /// if no vertex colours.
  ///
  /// @param stream Reserved for future use.
  /// @return A @c DataBuffer wrapper around the vertex colour memory. Must be of type @c DctUInt32
  /// with 1 component per element or @c DctUInt8 with 4 components (RGBA) per element.
  ///
  /// @todo Investigate supporting @c Vector4 based colours.
  virtual DataBuffer colours(int stream = 0) const = 0;

  /// Populate a mesh creation packet.
  /// @param packet A packet to populate and send.
  /// @return Zero on success, an error code otherwise.
  int create(PacketWriter &packet) const override;

  /// Populate a mesh destroy packet.
  /// @param packet A packet to populate and send.
  /// @return Zero on success, an error code otherwise.
  int destroy(PacketWriter &packet) const override;

  /// Populate the next mesh data packet.
  ///
  /// The @c progress.phase is used to track which data array currently being transfered,
  /// from the various @c MeshMessageType values matching components (e.g., vertices, indices).
  /// The @p progress.progress value is used to track how many have been transfered.
  ///
  /// @param packet A packet to populate and send.
  /// @param byteLimit A nominal byte limit on how much data a single @p transfer() call may add.
  /// @param[in,out] progress A progress marker tracking how much has already been transfered, and
  ///     updated to indicate what has been added to @p packet.
  /// @return Zero on success, an error code otherwise.
  int transfer(PacketWriter &packet, unsigned byteLimit, TransferProgress &progress) const override;

  bool readCreate(PacketReader &packet) override;

  // Must peek the meshId before calling this method. Mesh id must match this object.
  bool readTransfer(int messageType, PacketReader &packet) override;

protected:
  virtual void nextPhase(TransferProgress &progress) const;

  virtual bool processCreate(const MeshCreateMessage &msg,
                             const ObjectAttributes<double> &attributes);
  virtual bool processVertices(const MeshComponentMessage &msg, unsigned offset,
                               const DataBuffer &stream);
  virtual bool processIndices(const MeshComponentMessage &msg, unsigned offset,
                              const DataBuffer &stream);
  /// Process colour stream update.
  ///
  /// The @p stream may be a @c uint32_t stream, component per element, or a @c uint8_t stream with
  /// 4 components per element.
  ///
  /// @param msg The component message details.
  /// @param offset Index offset to read data into.
  /// @param stream The data buffer to read from.
  /// @return True on successfully processing the colours.
  virtual bool processColours(const MeshComponentMessage &msg, unsigned offset,
                              const DataBuffer &stream);
  virtual bool processNormals(const MeshComponentMessage &msg, unsigned offset,
                              const DataBuffer &stream);
  virtual bool processUVs(const MeshComponentMessage &msg, unsigned offset,
                          const DataBuffer &stream);
};
}  // namespace tes

#endif  // TES_CORE_SHAPES_MESH_RESOURCE_H
