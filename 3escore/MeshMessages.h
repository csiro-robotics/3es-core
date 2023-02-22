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
/// |           | [float32]     | Optional draw scale, present when @c McfDrawScale flag is set   |
/// | Destroy   | uint32        | MeshResource ID                                                 |
/// | Finalise  | uint32        | MeshResource ID                                                 |
/// |           | uint16        | Finalisation flags: @c MeshFinaliseFlag                         |
/// | Component | uint32        | MeshResource ID                                                 |
/// |           | *DataBuffer*  | The following data are written by a @c DataBuffer object.       |
/// |           | uint32        | Offset of the first data item                                   |
/// |           | uint16        | Count - the number of elements                                  |
/// |           | uint8         | Component count - the number of components in each element      |
/// |           | uint8         | Content type. See @c DataStreamType                             |
/// |           | [float32|64]  | Optional payload scale : @c DctPackedFloat16/DctPackedFloat32   |
/// |           | element*      | Array of count elements. Type varies.                           |
/// | Material  | uint32        | MeshResource ID                                                 |
/// |           | uint32        | Material ID                                                     |
///
/// The @c Component message above refers to of the data content messages (see below).
///
/// The @c DataBuffer section begins with an index offset, which  specifies the first index of the
/// incoming data, which allows the data streams to be sent in blocks. The element type is given by
/// @c DataStreamType , noting that @c DctPackedFloat16 and @c DctPackedFloat32 types are preceeded
/// by a single precision ( @c DctPackedFloat16 ) or double precision ( @c DctPackedFloat32 )
/// floating point quantisation factor. The component count details the number of components or
/// channels per element. Each component matches the content type ( @c DataStreamType ).
///
/// The table below identifies data type for each component. The data type may be a specific, fixed
/// type, or a general type supporting different packing. Any array notation indicates the number of
/// items used to pack a single component. For example, each vertex is represented by 3 `Real`
/// values. The second table maps these general types to the supported @c DataStreamType
/// values. Note that a client may not respect double precision values.
///
/// | Component Message | Component Type  |
/// | ----------------- | --------------- |
/// | Vertex            | Real[3]         |
/// | Vertex colour     | uint32          |
/// | Index             | uint            |
/// | Normal            | Real[2]         |
/// | UV                | float32[2]      |
///
/// | Component Type  | @c DataStreamType                                             |
/// | --------------- | ------------------------------------------------------------- |
/// | Real            | `DctFloat32, DctFloat64, DctPackedFloat16, DctPackedFloat32`  |
/// | uint            | `DctUInt8, DctUInt16, DctUInt32`                              |
/// | int             | `DctInt8, DctUInt16, DctUInt32`                               |
/// | uint32          | `DctInt32`                                                    |
/// | float32         | `DctFloat32, DctPackedFloat16`                                |
///
/// @par Additional notes
/// By default, one of the following materials are chosen:
///
/// - Lit with vertex colour if normals are specified or calculated.
/// - Unlit with vertex colour otherwise.
///
/// Vertex colours are initialised to white.

namespace tes
{
/// @ingroup meshmsg
/// Flag values for @c MeshCreateMessage .
enum MeshCreateFlag : unsigned
{
  /// Indicates the use of double precision floating point values.
  McfDoublePrecision = (1u << 0u),
  /// Draw scale is present in the creation message. This is a float32 value immediately following
  /// the creation message.
  McfDrawScale = (1u << 1u),
};

/// @ingroup meshmsg
/// Flag values for @c MeshFinaliseMessage .
enum MeshFinaliseFlag : unsigned
{
  /// Calculate normals on receive. Overwrites normals if present.
  MffCalculateNormals = (1u << 0u),
  /// Automatically colour vertices using a relative colour spectrum along the X axis.
  ///
  /// Ignored if explicit colour values are present.
  MffColourByX = (1u << 1u),
  /// Automatically colour vertices using a relative colour spectrum along the Y axis.
  ///
  /// Ignored if explicit colour values are present.
  MffColourByY = (1u << 2u),
  /// Automatically colour vertices using a relative colour spectrum along the Z axis.
  ///
  /// Semantically: colour by height if Z is up.
  ///
  /// Ignored if explicit colour values are present.
  MffColourByZ = (1u << 3u),
};

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
