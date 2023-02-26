//
// author: Kazys Stepanas
//
#ifndef TES_CORE_MESSAGES_H
#define TES_CORE_MESSAGES_H

#include "CoreConfig.h"

#include "PacketReader.h"
#include "PacketWriter.h"

#include <cinttypes>
#include <cstring>

namespace tes
{
// NOLINTBEGIN(modernize-use-auto, modernize-avoid-c-arrays, modernize-loop-convert,
// google-readability-casting)

/// List of routing IDs of common, built in message handlers.
/// These map to the @c MessageHandler::routingId() member.
///
/// Limited to 2^16 - 1.
///
/// @todo Rename to RoutingIDs that is used first then a message ID is assigned.
enum MessageTypeIDs : unsigned
{
  MtNull,
  MtServerInfo,
  MtControl,
  MtCollatedPacket,

  // TODO(KS): Move MtMesh and MtMaterial into a resources set.
  MtMesh,
  MtCamera,
  MtCategory,
  /// Extension. NYI.
  MtMaterial,

  /// First ID for renderers.
  ShapeHandlersIDStart = 64,
  /// First user ID
  UserIDStart = 2048
};

/// Default/built in renderers (routing IDs).
enum ShapeHandlerIDs : unsigned
{
  SIdSphere = ShapeHandlersIDStart,
  SIdBox,
  SIdCone,
  SIdCylinder,
  SIdCapsule,
  SIdPlane,
  SIdStar,
  SIdArrow,
  SIdMeshShape,
  SIdMeshSet,
  SIdPointCloudDeprecated,
  SIdText3D,
  SIdText2D,
  SIdPose,  ///< A set of axes representing a pose. Coloured XYZ => RGB.

  SIdBuiltInLast = SIdText2D
};

/// Message IDs for a @c ControlMessage.
enum ControlId : unsigned
{
  CIdNull,
  /// Marks a change of frame. Pending objects changes are applied Defines a new
  /// frame. @c value32 specifies the frame time delta in the server time units,
  /// or to use the default time delta when 0. @c Value64 should always be zero,
  /// but is used internally during playback to identify the frame number.
  CIdFrame,
  /// Specifies the a change in coordinate frame view. @c value32 contains the
  /// new @c CoordinateFrame to use.
  CIdCoordinateFrame,
  /// Set the total number of frames to expect ( @c value32 ). Generally for
  /// serialised streams as live streams rarely know the total frame count.
  CIdFrameCount,
  /// Forces a frame update (render) without advancing the time. This message is
  /// primarily used at the start of a recording in order to display the shapes
  /// which have been loaded so far.
  CIdForceFrameFlush,
  /// Clear the scene. This drops all existing data. This is primarily intended
  /// for internal use in playback mode. The @c value32 is used to identify the
  /// frame number to which we are resetting.
  CIdReset,
  /// Request a keyframe. @c value32 is the frame number. This is not for remote
  /// This is not for remote transmission, but supports snapping the scene in
  /// order to improve step-back updates.transmission, but supports snapping the
  /// scene in order to improve step-back updates.
  CIdKeyframe,
  /// Marks the end of the server stream. Clients may disconnect.
  CIdEnd,
};

/// Message IDs for @c MtCategory routing.
enum CategoryMessageId : unsigned
{
  CMIdName,  ///< Category name definition.
};

/// Object/shape management message ID. Used with @c ShapeHandlerIDs routing IDs.
enum ObjectMessageId : unsigned
{
  OIdNull,
  OIdCreate,
  OIdUpdate,
  OIdDestroy,
  OIdData
};

/// Flags controlling the creation and appearance of an object.
enum ObjectFlag : unsigned
{
  OFNone = 0,  ///< No flags. Default appearance.
  /// Indicates @c ObjectAttributes is in double precision.
  OFDoublePrecision = (1u << 0u),
  OFWire = (1u << 1u),         ///< Show the object as a wireframe mesh.
  OFTransparent = (1u << 2u),  ///< The object supports transparency. Use the colour alpha channel.
  OFTwoSided = (1u << 3u),     ///< Use a two sided shader.
  /// Shape creation should replace any pre-exiting shape with the same object ID.
  /// Normally duplicate shape creation messages are not allowed. This flag allows a duplicate shape
  /// ID (non-transient) by replacing the previous shape.
  OFReplace = (1u << 4u),
  /// Creating multiple shapes in one message.
  OFMultiShape = (1u << 5u),
  /// Do not reference count resources or queue resources for sending.
  ///
  /// By default each connection reference counts and queues resources for each shape, sending them
  /// from
  /// @c Connection::updateTransfers() . This flag prevents resources from being sent automatically
  /// for a shape. References are then dereferenced (potentially destroyed) when destroying a
  /// resource using shape. This flag prevents this reference counting for a shape, essentially
  /// assuming the client has the resources via explicit references using @c
  /// Connection::referenceResource() .
  ///
  /// This should always be used when using the @c OFReplace flag as reference counting can only be
  /// maintained with proper create/destroy command pairs.
  OFSkipResources = (1u << 6u),

  OFExtended = (1u << 8u)  ///< User flags start here.
};

/// Additional attributes for point data sources.
enum PointsAttributeFlag : unsigned
{
  PAFNone = 0,              ///< No additional data (points only)
  PAFNormals = (1u << 0u),  ///< Per point normals.
  PAFColours = (1u << 1u)   ///< Per point colours.
};

/// @c ObjectFlag extensions for Text2D rendering.
enum Text2DFlag : unsigned
{
  Text2DFWorldSpace = OFExtended  ///< Position is given in world space and mapped to screen space.
                                  ///< Otherwise in screen space with (0, 0, z) at the top left
                                  ///< corner and (1, 1, z) at the bottom right.
};

/// @c ObjectFlag extensions for Text2D rendering.
enum Text3DFlag : unsigned
{
  Text3DFScreenFacing = OFExtended  ///< Text is oriented to face the screen.
};

/// @c ObjectFlag extensions for @c MeshShape.
enum MeshShapeFlag : unsigned
{
  MeshShapeCalculateNormals = OFExtended  ///< Calculate normals and rendering with lighting.
};

/// Flags controlling the creation and appearance of an object.
enum UpdateFlag : unsigned
{
  // NOLINTBEGIN(hicpp-signed-bitwise)
  /// This flag indicates that the @c UpdateMessage only contains data for specific items.
  /// Combined with `{UFPosition, UFRotation, UFScale, UFColour}` to indicate what data are present.
  ///
  /// When this flag is not present, then all @c ObjectAttributes are being updated and none of the
  /// aforementioned flags should be set.
  UFUpdateMode = (OFExtended << 1u),
  UFPosition = (OFExtended << 2u),  ///< Update position data.
  UFRotation = (OFExtended << 3u),  ///< Update rotation data.
  UFScale = (OFExtended << 4u),     ///< Update scale data.
  UFColour = (OFExtended << 5u),    ///< Update colour data.
  UFPosRotScaleColour = UFPosition | UFRotation | UFScale | UFColour,
  // NOLINTEND(hicpp-signed-bitwise)
};

/// Flags for @c CollatedPacketMessage.
enum CollatedPacketFlag : unsigned
{
  CPFCompress = (1u << 0u),
};

/// Flags for various @c ControlId messages.
enum ControlFlag : unsigned
{
  /// Flag for @c CIdFrame indicating transient objects should be maintained and not flushed for
  /// this frame.
  CFFramePersist = (1u << 0u),
};

/// Data type identifies for any data stream type. Also used in @c DataBuffer to identify the
/// contained data type. Note the packed types are not valid to be held in a @c DataBuffer and are
/// only used in transmission.
enum DataStreamType : unsigned
{
  DctNone,     ///< No type: invalid.
  DctInt8,     ///< Elements using 8-bit signed integers.
  DctUInt8,    ///< Elements using 8-bit unsigned integers.
  DctInt16,    ///< Elements using 16-bit signed integers.
  DctUInt16,   ///< Elements using 16-bit unsigned integers.
  DctInt32,    ///< Elements using 32-bit signed integers.
  DctUInt32,   ///< Elements using 32-bit unsigned integers.
  DctInt64,    ///< Elements using 64-bit signed integers.
  DctUInt64,   ///< Elements using 64-bit unsigned integers.
  DctFloat32,  ///< Elements using single precision floating point values.
  DctFloat64,  ///< Elements using double precision floating point values.
  /// Elements packed using 16-bit signed integers used to quantise single precision floating point
  /// values. The quantisation scale factor immeidately preceeds the data array as a 32-bit floating
  /// point value.
  DctPackedFloat16,
  /// Elements packed using 32-bit signed integers used to quantise double precision floating point
  /// values. The quantisation scale factor immeidately preceeds the data array as a 64-bit floating
  /// point value.
  DctPackedFloat32,
};

/// Information about the server. This is sent to clients on connection.
/// Defines global settings for this server.
struct TES_CORE_API ServerInfoMessage
{
  /// Specifies the time unit in a @c CIdFrame @c ControlMessage.
  /// Each unit in the value is scaled by this time unit.
  ///
  /// This value is specified in micro-seconds.
  ///
  /// The default is 1000us (1 millisecond).
  uint64_t time_unit;
  /// The default time delta between frames to used when none is specified.
  /// Only used in replay.
  ///
  /// This value is specified in the @c time_unit.
  ///
  /// The default is 33ms (1/30s).
  uint32_t default_frame_time;
  /// Specifies the @c CoordinateFrame used by this server.
  ///
  /// The default is @c XYZ.
  uint8_t coordinate_frame;
  /// Reserved for future use. Must be zero.
  /// Aiming to pad out to a total of 64-bytes in the packet.
  uint8_t reserved[35];  // NOLINT(readability-magic-numbers)

  /// Read this message from @p reader.
  /// @param reader The data source.
  /// @return True on success.
  inline bool read(PacketReader &reader)
  {
    bool ok = true;
    ok = reader.readElement(time_unit) == sizeof(time_unit) && ok;
    ok = reader.readElement(default_frame_time) == sizeof(default_frame_time) && ok;
    ok = reader.readElement(coordinate_frame) == sizeof(coordinate_frame) && ok;
    ok = reader.readArray(reserved, sizeof(reserved) / sizeof(reserved[0])) ==
           sizeof(reserved) / sizeof(reserved[0]) &&
         ok;
    return ok;
  }

  /// Write this message to @p writer.
  /// @param writer The target buffer.
  /// @return True on success.
  inline bool write(PacketWriter &writer) const
  {
    bool ok = true;
    ok = writer.writeElement(time_unit) == sizeof(time_unit) && ok;
    ok = writer.writeElement(default_frame_time) == sizeof(default_frame_time) && ok;
    ok = writer.writeElement(coordinate_frame) == sizeof(coordinate_frame) && ok;
    ok = writer.writeArray(reserved, sizeof(reserved) / sizeof(reserved[0])) ==
           sizeof(reserved) / sizeof(reserved[0]) &&
         ok;
    return ok;
  }
};

/// Initialise @p info to the defaults.
/// @param info The structure to initialise. Must not be null.
void TES_CORE_API initDefaultServerInfo(ServerInfoMessage *info);

/// A system control message.
struct TES_CORE_API ControlMessage
{
  /// Flags particular to this type of control message.
  uint32_t control_flags;
  /// 32-bit value particular to this type of control message.
  uint32_t value32;
  /// 32-bit value particular to this type of control message.
  uint64_t value64;

  /// Read this message from @p reader.
  /// @param reader The data source.
  /// @return True on success.
  inline bool read(PacketReader &reader)
  {
    bool ok = true;
    ok = reader.readElement(control_flags) == sizeof(control_flags) && ok;
    ok = reader.readElement(value32) == sizeof(value32) && ok;
    ok = reader.readElement(value64) == sizeof(value64) && ok;
    return ok;
  }

  /// Write this message to @p writer.
  /// @param writer The target buffer.
  /// @return True on success.
  inline bool write(PacketWriter &writer) const
  {
    bool ok = true;
    ok = writer.writeElement(control_flags) == sizeof(control_flags) && ok;
    ok = writer.writeElement(value32) == sizeof(value32) && ok;
    ok = writer.writeElement(value64) == sizeof(value64) && ok;
    return ok;
  }
};

/// Category name message.
struct TES_CORE_API CategoryNameMessage
{
  /// ID for this message.
  enum : unsigned
  {
    MessageId = CMIdName
  };
  /// Identifies the category for the message.
  uint16_t category_id;
  /// The (new) parent category for @c category_id. Zero for none.
  uint16_t parent_id;
  /// Default @c category_id to active? Non zero for yes (1).
  uint16_t default_active;
  /// Number of bytes in @c name, excluding a null terminator.
  uint16_t name_length;
  /// The name string @em without a null terminator. Must be exactly
  /// @c name_length bytes.
  const char *name;

  /// Read message content.
  /// @param reader The stream to read from.
  /// @return True on success, false if there is an issue with amount
  ///   of data available.
  inline bool read(PacketReader &reader, char *name_buffer, size_t name_buffer_size)
  {
    bool ok = true;
    ok = reader.readElement(category_id) == sizeof(category_id) && ok;
    ok = reader.readElement(parent_id) == sizeof(parent_id) && ok;
    ok = reader.readElement(default_active) == sizeof(default_active) && ok;
    ok = reader.readElement(name_length) == sizeof(name_length) && ok;
    name = name_buffer;

    if (!name_buffer && name_length || name_length + 1u > name_buffer_size)
    {
      return false;
    }

    reader.readRaw((uint8_t *)name_buffer, name_length);
    name_buffer[name_length] = '\0';

    return ok;
  }

  /// Write this message to @p writer.
  /// @param writer The target buffer.
  /// @return True on success.
  inline bool write(PacketWriter &writer) const
  {
    bool ok = true;
    const uint16_t name_length = (uint16_t)((name) ? strlen(name) : 0);
    ok = writer.writeElement(category_id) == sizeof(category_id) && ok;
    ok = writer.writeElement(parent_id) == sizeof(parent_id) && ok;
    ok = writer.writeElement(default_active) == sizeof(default_active) && ok;
    ok = writer.writeElement(name_length) == sizeof(name_length) && ok;
    if (name_length > 0)
    {
      if (name)
      {
        ok = writer.writeRaw((const uint8_t *)name, name_length) == name_length && ok;
      }
      else
      {
        ok = false;
      }
    }
    return ok;
  }
};

/// A packet collation message header.
struct TES_CORE_API CollatedPacketMessage
{
  /// Message flags. See @c CollatedPacketFlag.
  uint16_t flags;
  /// Reserved: must be zero.
  uint16_t reserved;
  /// Number of uncompressed bytes in the payload.
  uint32_t uncompressed_bytes;

  /// Read this message from @p reader.
  /// @param reader The data source.
  /// @return True on success.
  inline bool read(PacketReader &reader)
  {
    bool ok = true;
    ok = reader.readElement(flags) == sizeof(flags) && ok;
    ok = reader.readElement(reserved) == sizeof(reserved) && ok;
    ok = reader.readElement(uncompressed_bytes) == sizeof(uncompressed_bytes) && ok;
    return ok;
  }

  /// Write this message to @p writer.
  /// @param writer The target buffer.
  /// @return True on success.
  inline bool write(PacketWriter &writer) const
  {
    bool ok = true;
    ok = writer.writeElement(flags) == sizeof(flags) && ok;
    ok = writer.writeElement(reserved) == sizeof(reserved) && ok;
    ok = writer.writeElement(uncompressed_bytes) == sizeof(uncompressed_bytes) && ok;
    return ok;
  }
};

/// Contains core object attributes. This includes details
/// of the model transform and colour.
template <typename Real>
struct TES_CORE_API ObjectAttributes
{
  uint32_t colour;   ///< Initial object colour.
  Real position[3];  ///< Object position.
  Real rotation[4];  ///< Object rotation (quaternion) xyzw order.
  Real scale[3];     ///< Object scale.

  /// Set to an identity transform coloured white.
  inline void identity()
  {
    colour = 0xffffffffu;
    position[0] = position[1] = position[2] = rotation[0] = rotation[1] = rotation[2] = 0.0f;
    rotation[3] = 1.0f;
    scale[0] = scale[1] = scale[2] = 1.0f;
  }

  /// Read this message from @p reader as is.
  /// @param reader The data source.
  /// @return True on success.
  inline bool read(PacketReader &reader) { return readT<Real>(reader); }

  /// Read this message from @p reader reading either double or single precision depending on @p
  /// read_double_precision .
  /// @param reader The data source.
  /// @param read_double_precision True to try read double precision, false to try single precision.
  /// @return True on success.
  inline bool read(PacketReader &reader, bool read_double_precision)
  {
    if (read_double_precision)
    {
      return readT<double>(reader);
    }
    return readT<float>(reader);
  }

  /// Read this message from @p reader using the type @p T to decode position, rotation and scale
  /// elements.
  /// @param reader The data source.
  /// @return True on success.
  /// @tparam T Either @c float or @c double
  template <typename T>
  inline bool readT(PacketReader &reader)
  {
    bool ok = true;
    T value;
    ok = reader.readElement(colour) == sizeof(colour) && ok;
    for (int i = 0; i < 3; ++i)
    {
      ok = reader.readElement(value) == sizeof(value) && ok;
      position[i] = Real(value);
    }
    for (int i = 0; i < 4; ++i)
    {
      ok = reader.readElement(value) == sizeof(value) && ok;
      rotation[i] = Real(value);
    }
    for (int i = 0; i < 3; ++i)
    {
      ok = reader.readElement(value) == sizeof(value) && ok;
      scale[i] = Real(value);
    }
    return ok;
  }

  /// Write this message to @p writer as is.
  /// @param writer The target buffer.
  /// @return True on success.
  inline bool write(PacketWriter &writer) const { return writeT<Real>(writer); }

  /// Write this message to @p writer selecting the packing precision based on @c
  /// write_double_precision .
  /// @param writer The target buffer.
  /// @param write_double_precision True to write double precision values, false for single
  /// precision.
  /// @return True on success.
  inline bool write(PacketWriter &writer, bool write_double_precision) const
  {
    if (write_double_precision)
    {
      return writeT<double>(writer);
    }
    return writeT<float>(writer);
  }

  /// Write this message to @p writer using the type @p T to encoder position, rotation and scale
  /// elements.
  /// @param writer The target buffer.
  /// @return True on success.
  /// @tparam T Either @c float or @c double
  template <typename T>
  inline bool writeT(PacketWriter &writer) const
  {
    bool ok = true;
    T value;
    ok = writer.writeElement(colour) == sizeof(colour) && ok;
    for (int i = 0; i < 3; ++i)
    {
      value = T(position[i]);
      ok = writer.writeElement(value) == sizeof(value) && ok;
    }
    for (int i = 0; i < 4; ++i)
    {
      value = T(rotation[i]);
      ok = writer.writeElement(value) == sizeof(value) && ok;
    }
    for (int i = 0; i < 3; ++i)
    {
      value = T(scale[i]);
      ok = writer.writeElement(value) == sizeof(value) && ok;
    }
    return ok;
  }

  template <typename real_dst>
  inline operator ObjectAttributes<real_dst>() const
  {
    ObjectAttributes<real_dst> dst;
    dst.colour = colour;
    dst.position[0] = real_dst(position[0]);
    dst.position[1] = real_dst(position[1]);
    dst.position[2] = real_dst(position[2]);
    dst.rotation[0] = real_dst(rotation[0]);
    dst.rotation[1] = real_dst(rotation[1]);
    dst.rotation[2] = real_dst(rotation[2]);
    dst.rotation[3] = real_dst(rotation[3]);
    dst.scale[0] = real_dst(scale[0]);
    dst.scale[1] = real_dst(scale[1]);
    dst.scale[2] = real_dst(scale[2]);
    return dst;
  }
};

template struct TES_CORE_API ObjectAttributes<float>;
template struct TES_CORE_API ObjectAttributes<double>;
using ObjectAttributesf = ObjectAttributes<float>;
using ObjectAttributesd = ObjectAttributes<double>;

/// Defines an object creation message. This is the message header and is immediately followed by @c
/// ObjectAttributes in either single or double precision depending on the @c OFDoublePrecision
/// flag. Any type type specific payload follows.
struct TES_CORE_API CreateMessage
{
  /// ID for this message.
  enum : unsigned
  {
    MessageId = OIdCreate
  };

  uint32_t id;        ///< Id of the object to create. Zero for transient objects.
  uint16_t category;  ///< Object categorisation. Used to control visibility.
  uint16_t flags;  ///< Flags controlling the appearance and creation of the object (@c ObjectFlag).
  uint16_t reserved;  ///< Reserved for future use.

  /// Read message content.
  /// Crc should have been validated already
  /// @param reader The stream to read from.
  /// @return True on success, false if there is an issue with amount
  ///   of data available.
  template <typename Real>
  inline bool read(PacketReader &reader, ObjectAttributes<Real> &attributes)
  {
    bool ok = true;
    ok = reader.readElement(id) == sizeof(id) && ok;
    ok = reader.readElement(category) == sizeof(category) && ok;
    ok = reader.readElement(flags) == sizeof(flags) && ok;
    ok = reader.readElement(reserved) == sizeof(reserved) && ok;
    ok = attributes.read(reader, flags & OFDoublePrecision) && ok;
    return ok;
  }

  /// Write this message to @p writer.
  /// @return True on success.
  template <typename Real>
  inline bool write(PacketWriter &writer, const ObjectAttributes<Real> &attributes) const
  {
    bool ok = true;
    ok = writer.writeElement(id) == sizeof(id) && ok;
    ok = writer.writeElement(category) == sizeof(category) && ok;
    ok = writer.writeElement(flags) == sizeof(flags) && ok;
    ok = writer.writeElement(reserved) == sizeof(reserved) && ok;
    ok = attributes.write(writer, flags & OFDoublePrecision) && ok;
    return ok;
  }
};

/// Defines an object data message. This is for complex shapes to send
/// additional creation data piecewise. Not supported for transient shapes.
///
/// This is the message header and the payload follows.
struct TES_CORE_API DataMessage
{
  /// ID for this message.
  enum : unsigned
  {
    MessageId = OIdData
  };

  uint32_t id;  ///< Id of the object to update data.

  /// Read message content.
  /// Crc should have been validated already
  /// @param reader The stream to read from.
  /// @return True on success, false if there is an issue with amount
  ///   of data available.
  inline bool read(PacketReader &reader)
  {
    bool ok = true;
    ok = reader.readElement(id) == sizeof(id) && ok;
    return ok;
  }

  /// Write this message to @p writer.
  /// @param writer The target buffer.
  /// @return True on success.
  inline bool write(PacketWriter &writer) const
  {
    bool ok = true;
    ok = writer.writeElement(id) == sizeof(id) && ok;
    return ok;
  }
};

/// A update message is identical in header to a @c CreateMessage. It's payload
/// may vary and in some cases it will have no further payload. See @c UpdateFlag .
struct TES_CORE_API UpdateMessage
{
  /// ID for this message.
  enum : unsigned
  {
    MessageId = OIdUpdate
  };

  uint32_t id;  ///< Object creation id. Zero if defining a transient/single frame message.
  /// Update flags from @c UpdateFlag. Note: @c OFDoublePrecision controls the precision of @c
  /// ObjectAttributes.
  uint16_t flags;

  /// Read message content.
  /// Crc should have been validated already
  /// @param reader The stream to read from.
  /// @return True on success, false if there is an issue with amount
  ///   of data available.
  template <typename Real>
  inline bool read(PacketReader &reader, ObjectAttributes<Real> &attributes)
  {
    bool ok = true;
    ok = reader.readElement(id) == sizeof(id) && ok;
    ok = reader.readElement(flags) == sizeof(flags) && ok;
    ok = attributes.read(reader, flags & OFDoublePrecision) && ok;
    return ok;
  }

  /// Write this message to @p writer.
  /// @param writer The target buffer.
  /// @return True on success.
  template <typename Real>
  inline bool write(PacketWriter &writer, const ObjectAttributes<Real> &attributes) const
  {
    bool ok = true;
    ok = writer.writeElement(id) == sizeof(id) && ok;
    ok = writer.writeElement(flags) == sizeof(flags) && ok;
    ok = attributes.write(writer, flags & OFDoublePrecision) && ok;
    return ok;
  }
};

/// Message to destroy an exiting object by id and type.
struct TES_CORE_API DestroyMessage
{
  /// ID for this message.
  enum : unsigned
  {
    MessageId = OIdDestroy
  };

  uint32_t id;  ///< Id of the object to destroy, matching the @c CreateMessage id.

  /// Read message content.
  /// Crc should have been validated already
  /// @param reader The stream to read from.
  /// @return True on success, false if there is an issue with amount
  ///   of data available.
  inline bool read(PacketReader &reader)
  {
    bool ok = true;
    ok = reader.readElement(id) == sizeof(id) && ok;
    return ok;
  }

  /// Write this message to @p writer.
  /// @param writer The target buffer.
  /// @return True on success.
  inline bool write(PacketWriter &writer) const
  {
    bool ok = true;
    ok = writer.writeElement(id) == sizeof(id) && ok;
    return ok;
  }
};

/// The message structure for a @c MtCamera routing message.
///
/// Note, we only expect message id zero.
struct TES_CORE_API CameraMessage
{
  /// Reserved @c camera_id for recording the camera properties during playback.
  static constexpr uint8_t kRecordedCameraID = 255u;

  /// ID of the camera. 255 is reserved to record the view used while recording.
  uint8_t camera_id;
  /// Flags. Currently must be zero as the only valid flag is the double precision indicator (value
  /// 1), which is not supported this structure. All values are floats.
  uint8_t flags;
  /// Padding/reserved. Must be zero.
  uint32_t reserved;

  /// Position X coordinate.
  float x;
  /// Position Y coordinate.
  float y;
  /// Position Z coordinate.
  float z;

  /// Forward vector X value.
  float dirX;
  /// Forward vector Y value.
  float dirY;
  /// Forward vector Z value.
  float dirZ;

  /// Up vector X value.
  float upX;
  /// Up vector Y value.
  float upY;
  /// Up vector Z value.
  float upZ;

  /// Near clip plane (optional). Zero or less implies an unspecified or unchanged value.
  float near;
  /// Far clip plane (optional). Zero or less implies an unspecified or unchanged value.
  float far;
  /// Horizontal field of view in degrees (optional). Zero or less implies an unspecified or
  /// unchanged value. Degrees.
  float fov;

  /// Read the message from the given <paramref name="reader"/>.
  inline bool read(PacketReader &reader)
  {
    bool ok = true;
    ok = reader.readElement(camera_id) == sizeof(camera_id) && ok;
    ok = reader.readElement(flags) == sizeof(flags) && ok;
    ok = reader.readElement(reserved) == sizeof(reserved) && ok;
    ok = reader.readElement(x) == sizeof(x) && ok;
    ok = reader.readElement(y) == sizeof(y) && ok;
    ok = reader.readElement(z) == sizeof(z) && ok;
    ok = reader.readElement(dirX) == sizeof(dirX) && ok;
    ok = reader.readElement(dirY) == sizeof(dirY) && ok;
    ok = reader.readElement(dirZ) == sizeof(dirZ) && ok;
    ok = reader.readElement(upX) == sizeof(upX) && ok;
    ok = reader.readElement(upY) == sizeof(upY) && ok;
    ok = reader.readElement(upZ) == sizeof(upZ) && ok;
    ok = reader.readElement(near) == sizeof(near) && ok;
    ok = reader.readElement(far) == sizeof(far) && ok;
    ok = reader.readElement(fov) == sizeof(fov) && ok;
    return ok;
  }

  /// <summary>
  /// Write this message to <paramref name="packet"/>.
  /// </summary>
  /// <param name="packet">The packet to write to.</param>
  /// <returns>True</returns>

  inline bool write(PacketWriter &packet)
  {
    bool ok = true;
    ok = packet.writeElement(camera_id) == sizeof(camera_id) && ok;
    ok = packet.writeElement(flags) == sizeof(flags) && ok;
    ok = packet.writeElement(reserved) == sizeof(reserved) && ok;
    ok = packet.writeElement(x) == sizeof(x) && ok;
    ok = packet.writeElement(y) == sizeof(y) && ok;
    ok = packet.writeElement(z) == sizeof(z) && ok;
    ok = packet.writeElement(dirX) == sizeof(dirX) && ok;
    ok = packet.writeElement(dirY) == sizeof(dirY) && ok;
    ok = packet.writeElement(dirZ) == sizeof(dirZ) && ok;
    ok = packet.writeElement(upX) == sizeof(upX) && ok;
    ok = packet.writeElement(upY) == sizeof(upY) && ok;
    ok = packet.writeElement(upZ) == sizeof(upZ) && ok;
    ok = packet.writeElement(near) == sizeof(near) && ok;
    ok = packet.writeElement(far) == sizeof(far) && ok;
    ok = packet.writeElement(fov) == sizeof(fov) && ok;
    return ok;
  }
};

// NOLINTEND(modernize-use-auto, modernize-avoid-c-arrays, modernize-loop-convert,
// google-readability-casting)
}  // namespace tes

#endif  // TES_CORE_MESSAGES_H
