//
// author: Kazys Stepanas
//
#ifndef TES_CORE_MESH_MESSAGES_H
#define TES_CORE_MESH_MESSAGES_H

#include "CoreConfig.h"

#include "Messages.h"

#include "PacketReader.h"
#include "PacketWriter.h"

/// @ingroup tescpp
/// @defgroup meshmsg MeshResource Messages
/// Defines the set of messages used to construct mesh objects.
///
/// A mesh object is defined via a series of messages. This allows meshes to be defined over a
/// number of updates, limiting per frame communications.
///
/// MeshResource instantiation supports the following messages:
/// - Create : instantiates a new, empty mesh object and the draw type.
/// - Destroy : destroys an existing mesh object.
/// - Vertex : adds vertices to a mesh object.
/// - Vertex colour : adds vertex colours.
/// - Index : Defines the vertex indices. Usage depends on draw type.
/// - Normal : adds normals.
/// - UV : Adds UV coordinates.
/// - Set material : Sets the material for the mesh object.
/// - Finalise : Finalises the mesh object.
///
/// Within a @c PacketHeader, the mesh message is arranged as follows:
/// - PacketHeader header
/// - uint16 Message type = @c MtMesh
/// - uint16 @c MeshMessageType
///
/// A valid mesh definition requires at least the following messages: Create, Vertex, Index,
/// Finalise. Additional vertex streams, normals, etc can be added with the complete set of
/// messages.
///
/// Each mesh definition specifies one of the following draw modes or primitive types:
/// - DtPoints
/// - DtLines
/// - DtLineLoop
/// - DtLineStrip
/// - DtTriangles
/// - DtTriangleStrip
/// - DtTriangleFan
///
/// A mesh object defined through the @c MeshHandler doe snot support any child or sub-objects.
/// These sorts of relationships are defined in the mesh renderer. Note the precision of the float
/// values in the creat mesage varies and depends on the @c McfDoublePrecision flag.
///
/// @par Message Formats
/// | Message   | Data Type     | Semantics                                                       |
/// | --------- | ------------- | --------------------------------------------------------------- |
/// | Create    | uint32        | Unique mesh ID                                                  |
/// |           | uint32        | Vertex count                                                    |
/// |           | uint32        | Index count                                                     |
/// |           | uint16        | @c MeshCreateFlag values                                        |
/// |           | uint8         | Draw type                                                       |
/// |           | uint32        | MeshResource tint                                               |
/// |           | float32|64[3] | Position part of the mesh transform                             |
/// |           | float32|64[4] | Quaternion rotation for mesh transform                          |
/// |           | float32|64[3] | Scale factor part of mesh transform                             |
/// | Destroy   | uint32        | MeshResource ID                                                 |
/// | Finalise  | uint32        | MeshResource ID                                                 |
/// | Component | uint32        | MeshResource ID                                                 |
/// |           | uint32        | Offset of the first data item                                   |
/// |           | uint32        | Reserved (e.g., stream index support)                           |
/// |           | uint16        | Count                                                           |
/// |           | uint16        | The @c MeshComponentPayloadType                                 |
/// |           | [float32|64]  | Optional payload scale : @c McetPackedFloat16/McetPackedFloat32 |
/// |           | element*      | Array of count elements. Type varies.                           |
/// | Material  | uint32        | MeshResource ID                                                 |
/// |           | uint32 | Material ID                                                            |
///
/// The @c Component message above refers to of the data content messages. The offset specifies the
/// first index of the incoming data, which allows the data streams to be sent in blocks. The
/// element type is given by
/// @c MeshComponentMessage::elementType , noting that @c McetPackedFloat16 and @c McetPackedFloat32
/// types are preceeded by a single precision ( @c McetPackedFloat16 ) or double precision ( @c
/// McetPackedFloat32 ) floating point scale factor. The table below identifies data type for each
/// component. The data type may be a specific, fixed type, or a general type supporting different
/// packing. Any array notation indicates the number of items used to pack a single component. For
/// example, each vertex is represented by 3 `Real` values. The second table maps these general
/// types to the supported @c MeshComponentElementType values. Note that a client may not respect
/// double precision values.
///
/// | Component Message | Component Type  |
/// | ----------------- | --------------- |
/// | Vertex            | Real[3]         |
/// | Vertex colour     | uint32          |
/// | Index             | uint            |
/// | Normal            | Real[2]         |
/// | UV                | float32[2]      |
///
/// | Component Type  | @c MeshComponentElementType                                       |
/// | --------------- | ----------------------------------------------------------------- |
/// | Real            | `McetFloat32, McetFloat64, McetPackedFloat16, McetPackedFloat32`  |
/// | uint            | `McetInt8, McetInt16, McetInt32`                                  |
/// | int             | `McetUInt16, McetUInt32`                                          |
/// | uint32          | `McetInt32`                                                       |
/// | float32         | `McetFloat32, McetPackedFloat16`                                  |
///
/// @par Additional notes
/// By default, one of the following materials are chosen:
/// - Lit with vertex colour if normals are specified or calculated.
/// - Unlit with vertex colour otherwise.
/// Vertex colours are initialised to white.

namespace tes
{
/// @ingroup meshmsg
/// Flag values for @c MeshCreateMessage .
enum MeshCreateFlag : unsigned
{
  /// Indicates the use of double precision floating point values.
  McfDoublePrecision = (1u << 0u),
};

/// @ingroup meshmsg
/// Flag values for @c MeshFinaliseMessage .
enum MeshFinaliseFlag : unsigned
{
  /// Calculate normals on receive. Overwrites normals if present.
  MffCalculateNormals = (1u << 0u),
};

/// @ingroup meshmsg
/// The possible @c MeshComponentMessage::elementType values. Identifies the the data type use dto
/// pack the payload.
enum MeshComponentElementType : unsigned
{
  McetInt8,     ///< Elements packed using 8-bit signed integers.
  McetUInt8,    ///< Elements packed using 8-bit unsigned integers.
  McetInt16,    ///< Elements packed using 16-bit signed integers.
  McetUInt16,   ///< Elements packed using 16-bit unsigned integers.
  McetInt32,    ///< Elements packed using 32-bit signed integers.
  McetUInt32,   ///< Elements packed using 32-bit unsigned integers.
  McetFloat32,  ///< Elements packed using single precision floating point values.
  McetFloat64,  ///< Elements packed using double precision floating point values.
  /// Elements packed using 16-bit signed integers used to quantise single precision floating point
  /// values. The quantisation scale factor immeidately preceeds the data array as a 32-bit floating
  /// point value.
  McetPackedFloat16,
  /// Elements packed using 32-bit signed integers used to quantise double precision floating point
  /// values. The quantisation scale factor immeidately preceeds the data array as a 64-bit floating
  /// point value.
  McetPackedFloat32,
};

// clang-format off
template <typename T> inline uint8_t meshComponentElementType() { return 0xffu; }
template <> inline uint8_t meshComponentElementType<int8_t>() { return McetInt8; }
template <> inline uint8_t meshComponentElementType<uint8_t>() { return McetUInt8; }
template <> inline uint8_t meshComponentElementType<int16_t>() { return McetInt16; }
template <> inline uint8_t meshComponentElementType<uint16_t>() { return McetUInt16; }
template <> inline uint8_t meshComponentElementType<int32_t>() { return McetInt32; }
template <> inline uint8_t meshComponentElementType<uint32_t>() { return McetUInt32; }
template <> inline uint8_t meshComponentElementType<float>() { return McetFloat32; }
template <> inline uint8_t meshComponentElementType<double>() { return McetFloat64; }
// clang-format on

/// @ingroup meshmsg
/// Defines the messageIDs for mesh message routing.
enum MeshMessageType : unsigned
{
  MmtInvalid,
  MmtDestroy,
  MmtCreate,
  /// Add vertices
  MmtVertex,
  /// Add indices
  MmtIndex,
  /// Add vertex colours.
  MmtVertexColour,
  /// Add normals
  MmtNormal,
  /// Add UV coordinates.
  MmtUv,
  /// Define the material for this mesh.
  /// Extension. NYI.
  MmtSetMaterial,
  /// Redefine the core aspects of the mesh. This invalidates the mesh
  /// requiring re-finalisation, but allows the creation parameters to
  /// be redefined. Component messages (vertex, index, colour, etc) can
  /// also be changed after this message, but before a second @c MmtFinalise.
  MmtRedefine,
  /// Finalise and build the mesh
  MmtFinalise
};

/// @ingroup meshmsg
/// Defines the primitives for a mesh.
enum DrawType : unsigned
{
  DtPoints,
  DtLines,
  DtTriangles,
  /// Geometry shader based voxels. Vertices define the voxel centres, the normals define half
  /// extents.
  DtVoxels,
  // DtQuads,
  // DtLineLoop,
};

/// @ingroup meshmsg
/// MeshResource creation message. This is immediately followed @c ObjectAttributes&lt;Real&gt; in
/// either single precision - @c McfDoublePrecision clear - or double precision - @c
/// McfDoublePrecision set.
///
/// Supports the following @c MeshFlag values:
/// - @c McfDoublePrecision
struct MeshCreateMessage
{
  /// ID for this message.
  enum : unsigned
  {
    MessageId = MmtCreate
  };

  uint32_t mesh_id;       ///< Mesh resource ID.
  uint32_t vertex_count;  ///< Total count.
  uint32_t index_count;   ///< Total index count.
  uint16_t flags;         ///< @c MeshCreateFlag values
  uint8_t draw_type;      ///< Topology: see @c DrawType.

  /// Read this message from @p reader.
  ///
  /// This fails if the precision of @p Real does not match the @c McfDoublePrecision flag.
  /// @param reader The data source.
  /// @return True on success.
  template <typename Real>
  inline bool read(PacketReader &reader, ObjectAttributes<Real> &attributes)
  {
    bool ok = true;
    ok = reader.readElement(mesh_id) == sizeof(mesh_id) && ok;
    ok = reader.readElement(vertex_count) == sizeof(vertex_count) && ok;
    ok = reader.readElement(index_count) == sizeof(index_count) && ok;
    ok = reader.readElement(flags) == sizeof(flags) && ok;
    ok = reader.readElement(draw_type) == sizeof(draw_type) && ok;
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    ok = attributes.read(reader, flags & McfDoublePrecision) && ok;
    return ok;
  }

  /// Write this message to @p writer.
  ///
  /// This fails if the precision of @p Real does not match the @c McfDoublePrecision flag.
  /// @param writer The target buffer.
  /// @return True on success.
  template <typename Real>
  inline bool write(PacketWriter &writer, const ObjectAttributes<Real> &attributes) const
  {
    bool ok = true;
    ok = writer.writeElement(mesh_id) == sizeof(mesh_id) && ok;
    ok = writer.writeElement(vertex_count) == sizeof(vertex_count) && ok;
    ok = writer.writeElement(index_count) == sizeof(index_count) && ok;
    ok = writer.writeElement(flags) == sizeof(flags) && ok;
    ok = writer.writeElement(draw_type) == sizeof(draw_type) && ok;
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    ok = attributes.write(writer, flags & McfDoublePrecision) && ok;
    return ok;
  }
};

/// @ingroup meshmsg
/// MeshResource redefinition message.
struct MeshRedefineMessage : MeshCreateMessage
{
  /// ID for this message.
  enum : unsigned
  {
    MessageId = MmtRedefine
  };
};

/// @ingroup meshmsg
/// MeshResource destruction message.
struct MeshDestroyMessage
{
  /// ID for this message.
  enum : unsigned
  {
    MessageId = MmtDestroy
  };

  uint32_t mesh_id;

  /// Read this message from @p reader.
  /// @param reader The data source.
  /// @return True on success.
  inline bool read(PacketReader &reader)
  {
    bool ok = true;
    ok = reader.readElement(mesh_id) == sizeof(mesh_id);
    return ok;
  }

  /// Write this message to @p writer.
  /// @param writer The target buffer.
  /// @return True on success.
  inline bool write(PacketWriter &writer) const
  {
    bool ok = true;
    ok = writer.writeElement(mesh_id) == sizeof(mesh_id);
    return ok;
  }
};

/// @ingroup meshmsg
/// Message structure for adding vertices, colours, indices, or UVs.
struct MeshComponentMessage
{
  uint32_t mesh_id;

  /// Read this message from @p reader.
  /// @param reader The data source.
  /// @return True on success.
  inline bool read(PacketReader &reader)
  {
    bool ok = true;
    ok = reader.readElement(mesh_id) == sizeof(mesh_id) && ok;
    return ok;
  }

  /// Write this message to @p writer.
  /// @param writer The target buffer.
  /// @return True on success.
  inline bool write(PacketWriter &writer) const
  {
    bool ok = true;
    ok = writer.writeElement(mesh_id) == sizeof(mesh_id) && ok;
    return ok;
  }
};

/// @ingroup meshmsg
/// Not ready for use.
struct Material
{
  /// ID for this message.
  enum : unsigned
  {
    MessageId = MmtSetMaterial
  };

  uint32_t mesh_id;
  uint32_t material_id;
  uint16_t flags;  ///< Reserved for flags. Not used yet.

  /// Read this message from @p reader.
  /// @param reader The data source.
  /// @return True on success.
  inline bool read(PacketReader &reader)
  {
    bool ok = true;
    ok = reader.readElement(mesh_id) == sizeof(mesh_id) && ok;
    ok = reader.readElement(material_id) == sizeof(material_id) && ok;
    ok = reader.readElement(flags) == sizeof(flags) && ok;
    return ok;
  }

  /// Write this message to @p writer.
  /// @param writer The target buffer.
  /// @return True on success.
  inline bool write(PacketWriter &writer) const
  {
    bool ok = true;
    ok = writer.writeElement(mesh_id) == sizeof(mesh_id) && ok;
    ok = writer.writeElement(material_id) == sizeof(material_id) && ok;
    ok = writer.writeElement(flags) == sizeof(flags) && ok;
    return ok;
  }
};

/// @ingroup meshmsg
/// Message to finalise a mesh, ready for use.
///
/// Supports the following @c MeshFlag values:
/// - @c MffCalculateNormals
struct MeshFinaliseMessage
{
  /// ID for this message.
  enum : unsigned
  {
    MessageId = MmtFinalise
  };

  uint32_t mesh_id;
  uint16_t flags;  ///< @c MeshFinaliseFlag values

  /// Read this message from @p reader.
  /// @param reader The data source.
  /// @return True on success.
  inline bool read(PacketReader &reader)
  {
    bool ok = true;
    ok = reader.readElement(mesh_id) == sizeof(mesh_id) && ok;
    ok = reader.readElement(flags) == sizeof(flags) && ok;
    return ok;
  }

  /// Write this message to @p writer.
  /// @param writer The target buffer.
  /// @return True on success.
  inline bool write(PacketWriter &writer) const
  {
    bool ok = true;
    ok = writer.writeElement(mesh_id) == sizeof(mesh_id) && ok;
    ok = writer.writeElement(flags) == sizeof(flags) && ok;
    return ok;
  }
};
}  // namespace tes

#endif  // TES_CORE_MESH_MESSAGES_H
