// Documentation of the messagging protocol.

// clang-format off

/*!
@page docprotocol Messaging Protocol

3<sup>rd</sup> Eye Scene debugging is built on top of a core messaging system used to expose 3D data for external
viewing. This section details the packet header used for all messages, documents the core messages and details
compression.

The primary transport layer is intended to be TCP/IP, however other protocols may be used in future. For consistency,
all data elements are in <a href="https://en.wikipedia.org/wiki/Endianness">Network Byte Order (Big Endian)</a>. Many
common platforms are Little Endian and will required byte order swaps before using data values; e.g., Intel based
processors are generally Little Endian.


@section secversion Protocol Version

*Protocol Version: 0.3*

This page documents version 0.3 of the 3<sup>rd</sup> Eye Scene protocol. Changes to the major version number indicate a
breaking change or an introduction of major features in the core protocol. Point version changes indicate minor changes
such as the introduction of new flag values.

The protocol version number will be at least 1.0 on the initial release of the 3<sup>rd</sup> Eye Scene viewer
regardless of whether or not the protocol contains actual changes from the previous version.


@section secheader Packet Header

All messages begin with a standard @ref secheader "packet header". This header begins with a common, 4-byte marker
identifying the packet start, followed by the remaining header, a message payload and in most cases a 2-byte CRC. The
packet header also contains protocol version details, routing and message IDs, payload size details and a small number
of packet flags.

The routing and message IDs are used to identify how the packet is to be handled. Conceptually, the routing ID
identifies the recipient while the message ID identifies the packet content. Routing IDs must uniquely identify the
receiver, while message IDs have different meanings depending on the routing ID.

The packet header is layed out as follows (16-bytes).

Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Marker            | 4         | Identifies the start of a packet. Always the hex value 0x03e55e30.
Major version     | 2         | The major version of the message protocol. With the minor version, This identifies the packet header and message contents and layout.
Minor version     | 2         | The minor version of the message protocol.
Routing ID        | 2         | Identifies the message recipient. See @ref secroutingids.
Message ID        | 2         | The message ID identifies the payload contents to the handler.
Payload size      | 2         | Size of the payload after this header. Excludes header size and CRC bytes.
Payload offset    | 1         | For future use. Identifies a byte offset after this header where the payload begins. Initially always zero, in future this may be used to put additional data into the packet header.
Flags             | 1         | Packet flags. See below.


The @c Flags member supports the following flags:

Flag              | Value     | Description
----------------- | --------: | -------------------------------------------------------------------
No CRC            | 1         | The packet does not include a CRC after the header and payload.

The header is followed by the message payload separated by the number of bytes specified in `Payload offset`. The
offset should always be zero in the original protocol with the message payload immediately following the packer header.

The payload is followed by a 2-byte CRC, unless the `No CRC` flag is set. This CRC is calculated over the entire
packet header and message content.



@section secroutingids Core Routing IDs

The 3<sup>rd</sup> Eye Scene core supports the following routing IDs and message handlers.

Name              | RoutingID | Description
----------------- | --------: | -------------------------------------------------------------------
Null              | 0         | An invalid routing ID. Not used.
Server Info       | 1         | Handles messages about the server connection.
Control           | 2         | Used for control messages such as frame refresh.
Collated Packet   | 3         | Handles collated and and optionally compressed packet data.
Mesh              | 4         | Handles mesh resource messages. See TODO.
Camera            | 5         | Handles camera related messages.
Category          | 6         | Handles category and related messages.
Material          | 7         | Not implemented. Intended for future handling of material messages.
Shapes            | 64+       | Shape handlers share core message structures, but identify different 3D shapes and primitives.
User extensions   | 2048+     | User defined message IDs start here.




@subsection secshapeids Shape Routing IDs

Shape handlers all use a common message structure (see TODO) using the same create, update and destroy messages. Shapes
support a data message for sending additional data specific to that shape. Shapes have the following routing IDs.

Shape             | RoutingID | Description
----------------- | --------: | -------------------------------------------------------------------
Sphere            | 64        | A sphere primitive.
Box               | 65        | A box primitive.
Cone              | 66        | A cone primitive.
Cylinder          | 67        | A cylinder primitive.
Capsule           | 68        | A capsule primitive. A capsule is formed from two hemispheres connected by a cylinder. Often used as a physics primitive.
Plane             | 69        | A 2D quadrilateral positioned in 3D space with a normal component.Used to represent 3D planes at a point.
Star              | 70        | A star shape.
Arrow             | 71        | An arrow shape made from a conical head and cylindrical body.
Mesh Shape        | 72        | A single mesh object made up of vertices and indices with a specified draw type or topology.
Mesh Set          | 73        | A collection of mesh objects created via Mesh Routing ID. A mesh set supports more complex mesh structures than Mesh Set allowing multiple shared mesh resources. Meshes themselves support vertex colour and vertex normals.  See TODO "mesh resources".
Point Cloud       | 74        | Similar to a mesh set, a point cloud shape supports a single, shared point cloud mesh resource.
Text 3D           | 75        | Supports text with full 3D positioning and scale, with optional billboarding.
Text 2D           | 76        | Supports 2D text either located in screen space, or located in 3D and projected into screen space.
Pose              | 77        | Some representation of a postion and orientation making a pose. May be visualised as a set of oriented axes.


@section secserverinfomsg Server Info Messages

Server info messages provide information about the server. There is currently only one server message, message ID zero.
It is detailed below.

Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Time Unit         | 8         | Specifies the time unit used in frame messages. That is, each time unit in other messages is scaled by this value. This value is measured in microseconds and defaults to 1000us (1 milliscond).
Default Frame Time| 4         | Specifies the default time to display a frame for when an end frame message does not specify a time value (zero). Generally not relevant for real time visualisation as there should always be a real frame time in this case. This value is scaled by the `Time Unit` and defaults to 33(ms).
Coordinate Frame  | 1         | Specifies the server's coordinate frame. See below.
Reserved          | 35        | Additionaly bytes reserved for future expansion. These pad the structure to 48-bytes, which pads to 64-bytes when combined with the packet header. All bytes must be zero for correct CRC.

The coordinate frame identifies the server's basis axes. It is set from one of the following values. The coordinate
frame name identifies the right, forward and up axes in turn. For example XYZ, specifies the X axis as right, Y as
forward and Z as up and is right handed. Similarly XZY specifies X right, Z forward, Y up and is left handed. Some axes
are prefixed with a '-' sign. This indicates the axis is flipped.

Frame Name        | Value     | Left/Right Handed
----------------- | --------: | -----------------
XYZ               | 0         | Right
XZ-Y              | 1         | Right
YX-Z              | 2         | Right
YZX               | 3         | Right
ZXY               | 4         | Right
ZY-X              | 5         | Right
XY-Z              | 6         | Left
XZY               | 7         | Left
YXZ               | 8         | Left
YZ-X              | 9         | Left
ZX-Y              | 10        | Left
ZYX               | 11        | Left



@section seccontrolmsg Control Messages

Control messages are special commands to the client. The most commonly used control message is the @c Frame message
which identifies an "end of frame" event and causes the client to display expire transient shapes and dispaly new shapes
shapes. All control messages have the same message structure, but the semantics of the message content vary.

Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Flags             | 4         | Flags for the control message. Semantics vary.
Value 32          | 4         | A 32-bit value. Semantics vary.
Value 64          | 8         | A 64-bit value. Semantics vary.

Valid control messages and their value and flag semantics are:

Name              | MessageID | Description
----------------- | --------: | -------------------------------------------------------------------
Null              | 0         | An invalid message ID. Not used.
Frame             | 1         | An end of frame message causing a frame flush. Uses flags (see TODO). Value 32 stores the frame time in the server time unit (see @ref secserverinfomsg) or zero to use the default frame time.
Coordiante Frame  | 2         | Marks a change in the coordinate from since the Server Info message was sent. Value 32 stores the new coordinate frame ID.
Frame Count       | 3         | Value 32 specifies the total number of frames. Intended only for recorded file streams to identify the target frame count.
Force Frame Flush | 4         | Force a frame flush without advancing the frame number or time. Intended as an internal control message only on the client. Values not used.
Reset             | 5         | Clear the scene, dropping all existing data. Values not used.
Keyframe          | 6         | Request a key frame to be serialied. Value 32 is the frame number. Only to be used by the client and not to be sent from the server.
End               | 7         | Marks the end fo the server stream. Clients may disconnect.



@subsection seccontrolframemsg Frame Message Flags

Flats associated with a Frame control message flags are:

Name              | Value     | Description
----------------- | --------: | -------------------------------------------------------------------
Persist           | 1         | Request that transient objects persist and are not flushed this frame.



@section seccollatedmsg Collated Packet Messages

Collated packet rounting has one valid message ID (zero), and identifies the packet payload as containing additional
packet headers and packet data, optionally GZIP compressed. Collated packets serve two purposes:

-# Collating messages into a single chunk, generally for thread safety.
-# To support packet compression.

The collated packet message has the following payload:

Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Flags             | 2         | See @ref seccollatedflags
Reserved          | 2         | Reserved for future use (padding)
Uncompressed Bytes| 4         | Specifies the number of @em uncompressed bytes in the payload. This is the sum of all the payload's individual packets (excluding the message header).

It is important to note that collated packet sizes are generally limited to less than 65535 bytes due to the packet
payload size restriction with one important exception (see below). When a server creates collated packets, it is free to
add as much data as possible to reach the payload limit, but must not contain partial packets. That is, a packet
contained in a collated packet must be wholly contained in that packet regardless of compression settings.

There is an exception to the size limitation which caters to how a collated packet is used in file saving. File streams
may contain a collated packet which exceeds the packet size limitation. To support this, it cannot have a CRC as the CRC
location cannot be calculated based on a 16-bit payload size in the collated packet header. The collated packet can
break the size restrictions by expressing that it contains more than 65535 uncompressed bytes.

Collated packet data begins immediately following the collated packet header and message (above) and is expressed as a
series of valid packet headers with their corresponding message payloads. These bytes may be GZIP compressed as
specified by the collated packet message flags. A collated packet reader knows it has reached the end of the collated
data when it has processed a number of uncompressed bytes equal to `Uncompressed Bytes` as specified in the
collated packet message (this includes CRCs if present for the contained packets). The next two bytes will either be the
CRC for the collated packet message (if present) or the beginning of the next packet, as identified by the packet header
marker bytes.



@subsection seccollatedflags Collated Packet Flags

Collated packet message flags are:

Name              | Value     | Description
----------------- | --------: | -------------------------------------------------------------------
Compress          | 1         | Collated packet payload is GZIP compressed. Compression begins after the message structure. That is, neither packet header nor the message are compressed.


@section vertexbufferpayload Vertex Buffer Payloads

Some messages will write a `VertexBuffer` based payload. This is not a message of itself, but is a consistent payload
data block. Vertex buffers are used to transfer large amounts of data associated with meshes. Multiple messages with
a `VertexBuffer` payload will often be required to transmit the entire data set. The `Offset` field marks where in the
buffer the payload should be assembled.

Wherever a `VertexBuffer` is refereneced, the following payload is expected:

Datum               | Byte Size | Description
------------------- | --------: | ------------------------------------------------
Offset              | 4         | Marks the offset into the destination buffer where this payload should be decoded.
Count               | 2         | The number of items present in this payload.
Component Count     | 1         | The number of components or channels in each item (see below)
Content Type        | 1         | The type of the data in this payload.
[Quansitation Unit] | 4 or 8    | Present only for packed data buffers (see below), marks the quantisation unit which values must be mutliplied by to retrieve the unpacked value.
[Packing Origin]    | 4[] or 8[]| Present only for packed data buffers (see below), stores an origin offset value to be added when restoring data. This is an array with a length matching the `Component Count`.
Data                | array     | The data content array. The byte size depends on the `Content Type`. There will be `Count * Component Count` items present.

Buffers may contain more than one component or channel such as in a Vector3 vertex array (float content). For example,
a `VertexBuffer` built to transfer 12 Vector3 values will contain the following payload values:

- Offset = 0
- Count = 12  : all vertices being set
- Component Count = 3 : three chanels, XYZ
- Content Type = 9 (see below)
- Quantisation unit not present : not a packed data type
- Packing origin not present : not a packed data type
- Data = Count * Component Count * sizeof(float) = 12 * 3 * 4 = 144 bytes

The Content Type values are:

Content Type  | Value | Byte Size
------------- | ----: | --------:
None          | 0     | 0
Int8          | 1     | 1
UInt8         | 2     | 1
Int16         | 3     | 2
UInt16        | 4     | 2
Int32         | 5     | 4
UInt32        | 6     | 4
Int64         | 7     | 8
UInt64        | 8     | 8
Float32       | 9     | 4
Float64       | 10    | 8
PackedFloat16 | 11    | 2
PackedFloat32 | 12    | 4

The two PackedFloat values are used to pack quantised floating point values. A PackedFloat16 value packs Float32 values
into a 16-bit, signed integer packed format, while PackedFloat32 does the same for Float64 into 32-bit, signed integers.
These two content types add the Quantisation Unit, 4 bytes for PackedFloat16 and 8 for PackedFloat32, and the Packing
Origin, an array of `Component Count` items, 4 bytes per component for PackedFloat16 and 8 for PackedFloat32. Data are
packed using the formula:

```cpp
int16_t packed_value = int16_t((float_value - packing_origin[channel]) / quantisation_unit);
```

Elements are restored using:

```cpp
float packed_value = (float(packed_value) + packing_origin[channel]) * quantisation_unit;
```

@section secmeshmsg Mesh Resource Messages

Mesh resource messages are used to create and populate mesh resources for viewing by the client. These resources may be
referenced in other messages such as those pertaining to the Mesh Set shape type. Mesh resources are identified by an
unique mesh resource ID. This ID is only unique among other mesh resources. It is the server's responsibility to ensure
unique resource ID assignment.

Valid mesh resource messages are:

Name              | Value     | Description
----------------- | --------: | -------------------------------------------------------------------
Invalid           | 0         | Not used.
Destroy           | 1         | Message to destroy a mesh resource.
Create            | 2         | Message to create a mesh resource.
Vertex            | 3         | Transfer of 3D vertex data.
Index             | 4         | Transfer of index data.
Vertex Colour     | 5         | Transfer of per vertex colour data.
Normal            | 6         | Transfer of per vertex normal data data.
UV                | 7         | Transfer of per vertex UV data. Not supported yet.
Set Material      | 8         | Identifies the material resource(s) for the mesh. Not supported yet.
Redefine          | 9         | Prepares the mesh for modification. New vertex, per vertex or index data may be incoming.
Finalise          | 10        | Finalises the mesh resource, making it ready for viewing.

The vertex, index, vertex colour, normal and UV messages all share a common message header followed by specific data
arrays (see below). All other messages are unique.



@subsection secmeshmsgdestroy Destroy Mesh Message
Instructs the client it may safely release a previously created mesh resource as identified by its unique mesh resource
ID. This message may be sent with an invalid resource ID.

Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Resource ID       | 4         | The resource ID of the mesh to destroy.



@subsection secmeshmsgcreate Create Mesh Message
Instructs the client to create a new mesh resource with the specified ID. The resource is not valid for use until a
finalise message is sent.

Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Resource ID       | 4         | The resource ID of the mesh to create. Zero is a valid ID.
Vertex Count      | 4         | Specifies the number of vertices in the mesh.
Index Count       | 4         | Specifies the number of indices in the mesh.
Flags             | 2         | Creation flags
Scale Factor      | 4         | Identifies the mesh topology. See TODO
Tint              | 4         | Mesh tint colour. See @ref secencodingcolour .
Translation       | 12 or 24  | 3 32 or 64-bit floats identifying the translation component of the overall mesh tranformation.
Rotation          | 16 or 32  | 4 32 or 64-bit floats identifying the quaternion rotation of the overall mesh tranformation.
Scale             | 12 or 24  | 3 32 or 64-bit floats identifying the scale component of the overall mesh transformation.

The Flag value 1 is used to indicate that the Translation, Rotation and Scale values are all 64-bit quantities.
Otherwise a 32-bit representation is used.

The full set of creation flags is:

Flag                    | Value   | Description
----------------------- | ------: | --------------------------------------------
Double Precision        | 1       | Indicates Translation, Rotation and Scale values are all 64-bit quantities.



@subsection secmeshtopology Mesh Topology
The mesh topology details how the vertices and indices are interpreted. Valid values are:

Name              | Value     | Description
----------------- | --------: | -------------------------------------------------------------------
Points            | 0         | Vertices represent individual points. Indices are optional and each index referencing a single point. May support draw scale to size the points.
Lines             | 1         | Vertices represent line end points. Indices must come in pairs. May support draw scale to control line width.
Triangles         | 2         | Vertices represent triangles. Indices must come in triples.
Voxels            | 3         | Similar to points, except that the normal value is used to scale a cube to represent voxels.
Quads             | 4         | Reserved for quadrilateral topology. Not implemented.



@subsection secmeshdatamsg Mesh Element Messages
This section details the message structure of transmitting mesh elements; vertices, indices, vertex colour, normals and
UVs. All messages have the same structure:

Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Resource ID       | 4         | The resource ID of the mesh to which the data belong.
Vertex Buffer     | varies    | The remainder of the payload is a vertex buffer.

The vertex buffer payloads have certain expectations placed on them.

Message           | Description
----------------- | -----------------------------------------------------------
Vertex            | Triples of Float32, Float64, PackedFloat16 or PackedFloat32
Vertex Colour     | UInt32 colours. See @ref secencodingcolour .
Index             | Any integer type
Normal            | Triples of Float32, Float64, PackedFloat16 or PackedFloat32
UV                | Pairs of Float32, Float64, PackedFloat16 or PackedFloat32

From this we see that each message identifies the target mesh resource, an index offset to start writing the new data,
an element count for this message followed by the elements themselves. The message only supports a 2-byte count, which
is likely exceeded by any complex mesh, so the offset identifies where to start writing. Consider a mesh with 80000
vertices. Each message can only transmit at most 65535 vertices because of the 2-byte count limit. The limit is actually
much lower than this because of the overall packet payload byte limit of 65535. For the sake of this example, let us say
that each packet can contain 30000 vertices. This will be split across three vertex messages. Each message identifies
the vertex count and the offset for the `VertexBuffer` payload, which is the index of the first vertex in the packet.
In this case, we have the following three messages:

- offset 0, count 30000
- offset 30000, count 30000
- offset 60000, count 20000

> The maths above is suspect. 30000 may be the incorrect payload count.


@section seccameramsg Camera Messages

Camera routing supports a single message (ID zero) which specifies settings for a camera view. This is an optional
camera and the client can elect to ignore or override any camera message, or parts of a camera. However, this message
can be used to set an initial focus, or to show what the server is viewing. Multiple cameras are supported and the
client can elect to lock their view to any valid camera.

Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Camera ID         | 1         | Identifies the camera. ID 255 is reserved for serialising details
                  |           | of the client's own camera when recording.
Flags             | 1         | One byte flag values. Flag value 1 indicates all float values are 64 bit.
Reserved          | 2         | Two bytes of padding. Must be zero.
Position          | 12 or 24  | XYZ coordinate for the camera position. Each element is a 32 or 64-bit float (see Flags).
Facing            | 12 or 24  | XYZ forward vector (normalised) identifying the camera facing relative to Position.
Up                | 12 or 24  | XYZ up vector (normalised) for the camera.
Near              | 4 or 8    | Near clip plane (32 or 64-bit float). Zero implies no change.
Far               | 4 or 8    | Far clip plane (32 or 64-bit float). Zero implies no change.
FOV               | 4 or 8    | Horizontal field of view in degrees. Zero implies no change.

Supported flag values are:

Flag                    | Value   | Description
----------------------- | ------: | --------------------------------------------
Double Precision        | 1       | Indicates Position, Facing, Up, Near, Far and FOV values are all 64-bit quantities.


@section seccategorymsg Category Messages

Category messages are used to identify and control categories associated with @ref secshapemsg "shapes". These messages
can be used to identyfing valid categories, give a category a name, build a category hierarchy or set the default state
for a category.

There is currently one category message with ID of zero.

Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Category ID       | 2         | Specifies the category ID.
Parent ID         | 2         | Sets the parent of this category, building category hierarchy. Zero always implies no parent.
Default Active    | 2         | Default the category to the active state? Must be either 1 (active) or 0 (inactive).
Name Length       | 2         | Number of bytes in the name string, excluding the null terminator.
Name              | Varies    | The name string in UTF-8 encoding. Contains `Name Length` bytes.



@section secmaterialmsg Material Messages

Material messages are not yet supported. They are intended for future expansion to detail material resources which may
be used with mesh resources.



@section secshapemsg Shape Messages

Shapes for the core of the 3<sup>rd</sup> Eye Scene message protocol. Shapes are used to visualise 3D geometry on the
client. Shapes cover a range of routing IDs all with a common message structure. Shapes may also be extended to user
defined routing IDs supporting the same message protocol. Shapes may be simple or complex. A simple shape is fully
defined by its create message, while a complex shape requires additional data messages to be completed.

Shapes support four primary messages:
- Create to instantiate a shape.
- Data to transmit additional data required to create a shape (complex shapes only).
- Update updates the shape transformation or colour.
- Destroy destroys a previously created shape.

In all cases a shape may append additional data after the main message (essential for @c secshapemsgdata "data
messages"). Details of how individual shapes vary are detailed in later sections.

Shapes may be persistent or transient. Persistent shapes require unique IDs - unique within the context of the shape's
routing ID, not across all existing shapes - and persist until explicitly destroy. Transient shapes have no ID and are
generally destroyed on the next frame (unless the end frame message includes the persist flag).



@subsection secshapemsgcreate Create Shape Message

Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Object ID         | 4         | Unique identifies the shape within the context of the current shape routing ID. Zero is reserved for transient shapes.
Category ID       | 2         | Identifies the category to which the shape belongs. Zero is the catch-all default.
Flags             | 2         | Shape creation flags. See @ref secshapemsgflags "below".
Reserved          | 2         | Reserved. Must be zero.
Attributes        | *         | See @ref secshapemsgattributes



@subsection secshapemsgflags Shape Flags

Shapes may support the following set of flags, though not all shapes will support all flags.

Flag              | Value     | Description
----------------- | --------: | -------------------------------------------------------------------
Double Precision  | 1         | Indicates that the Attributes section contains double precision floating point values.
Wireframe         | 2         | The shape should be visualised using wirefame rendering if supported.
Transparent       | 4         | The shape should be visualised as transparent. The colour alpha channel specifies the opacity.
Two Sided         | 8         | Use a two sided shader (for triangles)
Replace           | 16        | Replace an existing shape definition if there is one with the same object id and shape type. The existsing shape will be deleted.
Multi Shape       | 32        | The payload describes mutiple shapes in one message. The Attributes affect all child shapes. Addtional payload and attributes will follow for each shape.
Skip Resources    | 64        | Do not reference count or queue associated resources for sending (see below).
User              | 256       | Shape specific flags start here.

The Double Precision flag indicates that all floating point quantities in the Attributes section are 64-bit, double
precision values. Otherwise they are 32-bit, single precision values.

The Skip Resources flag prevents sending resources associated with this shape instances. Normally associated resources
are reference counted for the client and transfered when the first reference is attained - such as the first mesh
reference for a MeshSet shape. A destroy message for the resource is sent when the last reference is removed. This flag
prevents resources from being sent automatically for a shape. References are then dereferenced (potentially destroyed)
when destroying a resource using shape. This flag prevents this reference counting for a shape, essentially assuming
the client has the resources via explicit resource references.


@subsection secshapemsgattributes Shape Attributes

Shape attributes are used in both shape create and update messages and contain the following data:

Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Colour            | 4         | A four byte encoded colour for the same. See @ref secencodingcolour .
Translation       | 12 or 24  | 3 32 or 64-bit floating point values identifying the translation component of the overall shape transformation.
Rotation          | 16 or 32  | Usage varies, but generally defines 4 32 or 64-bit floating point values identifying a rotation for the shape.
Scale             | 12 or 24  | Usage varies, but generally defines 3 32 or 64-bit floating point values identifying the scale of the shape.

The selection of 32-bit or 64-bit floating point values is controlled by the presence (64-bit) or absence (32-bit) of
the Double Precision flag - see @ref secshapemsgflags

Note: Individual shapes may treat rotation and scale differently. Spheres, for example use rotation and scale to create
ellipsoids. Boxes use rotation and scale in a similar faction. Meanwhile "directional" shapes use the XY scale to
encode radius, and Z to encode length. Text uses the Z value to encode text size.



@subsection secencodingcolour Colour Encoding

@ref secshapemsgattributes "Shape attributes" include a @c Colour value to apply to the shape. Meshes also support a
tint colour. All 3<sup>rd</sup> Eye Scene colour values are 32-bit, RGBA colour values with 8-bits per colour channel.
The channels are ordered in low to high byte as red, green, blue, alpha.



@subsection secshapemsgspecificcreate Shape Specific Create Messages

This section lists any how specific shapes interpret the translation, rotation and scale components of their @ref
secshapemsgattributes "attributes" and also any additional data appended to a create message. Any unlisted shapes
interpret the translation, rotation and scale simply as is (rotation as a quaternion rotation) and append no data.
Unless otherwise specifies, each shape's local pivot or origin is at the centre of the shape.

Shape             | Complex?  | Directional?  |Variation
----------------  | --------- | ------------- | -------------------------------
Sphere            | No        | No            | Pivot is at the centre and scale is the radius. Rotation and and non-uniform scale may be used to create ellipsoids.
Box               | No        | No            | Scale sets the box edge length. E.g., (1, 1, 1) makes a unit cube.
Cone              | No        | Yes           | Pivot is at the apex. Default direction is (0, 0, 1) away from the apex.
Cylinder          | No        | Yes           | Default direction is (0, 0, 1), position is at the centre.
Capsule           | No        | Yes           | Default direction is (0, 0, 1), position is at the centre.
Plane             | No        | Yes           | Position defines a local pivot point to centre the quad on. Rotation defines the quaternion rotation away from the default 'normal' (0, 0, 1) Scale components 0, 2, define the quad size (both are equal) Scale component 1 specifies the length to render the normal with.
Star              | No        | No            | Pivot is at the centre, rotation is ignored, scale (uniform) specifies the radius.
Arrow             | No        | Yes           | Pivot is at the arrow base. Default direction is (0, 0, 1) away from the base.
Mesh Shape        | Yes       | No            | Writes additional data.
Mesh Set          | No        | No            | Writes additional data.
Point Cloud       | Yes       | No            | Writes additional data.
Text 3D           | No        | Yes*          | Supports user flags. Writes additional data. Position may defaults to screen space with (0, 0) the upper left corer and (1, 1) the lower right (Z ignored).
Text 2D           | No        | Yes*          | Supports user flags. Writes additional data.

\* Text only uses Z scale and direction usage is optional.

The directional shapes all use rotation as a quaternion rotation away from the default direction (0, 0, 1). It could
have been overridden to specify a true direction vector. However, this was not done to better support a common set of
get/set rotation methods with consistent semantics. A rotation is either ignored, or it is a quaternion rotation.

Directional shapes intepret scale as XY => radius and Z => length. Text ignores radius and used "length" to adjust text
size.

@subsection secshapemsgmeshcreate Mesh Shape Create Addendum

Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Vertex Count      | 4         | Number of vertices in the mesh. Must match topology if there are not indices.
Index Count       | 4         | Number of indices. Must match topology, or zero. Zero implies sequential vertex indexing.
Draw scale        | 4         | Topology scaling factor such as point size. See @ref secmeshtopology "topology"
Topology          | 1         | See @ref secmeshtopology "topology"

@subsection secshapemsgmeshsetcreate Mesh Set Create Addendum

Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Number of Parts   | 2         | Number of mesh resource parts to the mesh set.
Mesh Resource ID  | 4 *       | Mesh Resource ID for the next part.
Attributes        | N *       | @ref secshapemsgattributes for the part.

\* A `Part ID` and `Attributes` Are written for each part and appear a number of times equal to the `Number of Parts`.

The `Attributes` have floating point data size matching the `Double Precision` flag of the create message. That is,
each part will use double precision `Attributes` if the parent shape creation was marked with double precision.
Otherwise single precision is used.

@subsection secshapemsgpointcloudcreate Point Cloud Create Addendum

Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Mesh Resource ID  | 4 *       | Mesh Resource ID for this point cloud. Shared with mesh resources (mind the topology).
Index Count       | 4         | Index count used to index a subset of the vertices in the mesh resource. May be zero.
Point Size        | 4         | Override for the point rendering scale. Zero to use the default.


@subsection secshapemsgtextcreate Text 2D/3D Create Addendum

Text 2D/3D additional data:
Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Text Length       | 2         | Number of bytes in the following text (excludes null terminator).
Text              | N         | UTF-8 encoded string with `Text Length` bytes. No null terminator.

Text 2D shapes also support the following flags:
Flag              | Value     | Description
----------------- | --------: | -------------------------------------------------------------------
World Space       | 256       | Position is in world space to be projected onto the screen.

Text 3D Flags:
Flag              | Value     | Description
----------------- | --------: | -------------------------------------------------------------------
Screen Facing     | 256       | Text should always face the screen (billboarding).

@subsection secshapemsgmultishapecreate Multi Shape Creation

Non-complex shapes also support creation with the `Multi Shape` flag. This represents a collection of the same shape
type each with its own attributes. The `Attributes` section of the creation message serves as a parent transformation
matrix affecting all shapes in the set. A multi-shape create message has additional payload and there may be a data
message to follow.

A `Multi Shape` create message payload also contains:

Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Shape Count       | 4         | The number of items in the shape set. This may be larger than the number of shapes in the create message.
Payload Count     | 2         | The number of shapes described in the payload of this create message.
`Attributes`      | N         | There are `Payload Count` `Atttributes` structures in the payload.

The `Double Precision` flag affects both the create message `Attributes` for the parent and for the `Attributes` of
each shape in the create and the data message.

When the `Payload Count` matches the `Shape Count` then there will be no data message. When the `Payload Count` is
smaller than the `Shape Count`, then there will be at least one data message. The data message payload is the same
as the creation payload except that `Shape Count` is not present.

A multi-shape is treated as a single single with respect to it's ID. That is, a single destory message with the
appropriate ID will remove all shapes in the `Multi Shape` set.


@section secshapemsgdata Shape Data Message

Shape data messages are only sent for complex shapes. These are shapes which cannot be succinctly defined by their
create message, even with additional data appended to the create message. All data messages share the following header,
after which the data layout is entirely dependent on the shape type.

Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Object ID         | 4         | ID of the target shape. Transient shapes are ID zero.

Note: Transient shapes may be complex and require additional data messages. However, the message must have an object ID
of zero, which cannot uniquely identify a shape. Clients can only assume that a data message targeting a transient shape
pertains to the last transient shape created for that shape routing ID.


@subsection secshapemsgmeshdata Mesh Shape Data Payload

Mesh shapes write the following data messages:

Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Data Type ID      | 2         | Identifies the data payload: vertices, indices, normals, etc.
Vertex Buffer     | varies    | The data payload - see @ref vertexbufferpayload

The Data Type ID is one of the following:

Data Type ID  | Value | Expected payload
------------- | ----: | -------------------------------------------------------------------
Vertices      | 0     | Triples of Float32, Float64, PackedFloat16 or PackedFloat32
Indices       | 1     | Any integer type
Normals       | 2     | Triples of Float32, Float64, PackedFloat16 or PackedFloat32
Colours       | 3     | 32-bit colours values. See @ref secencodingcolour

Note: packet payload size restrictions may be ignored when serialising to disk.


@subsection secshapemsgpointclouddata Point Cloud Data Payload

Point cloud shapes can optionally write a set of indices which restrict what is viewed in the referenced point cloud
mesh resource. The payload is a `VertexBuffer` of any integer type.

Note: packet payload size restrictions may be ignored when serialising to disk.



@subsection shapemsgupdate Update Shape Message

Shape udpate messages are used to adjust the core attributes of a shape. Most notably, update messages are used to
reposition shapes in 3D space. Updates messages are only for persistent shapes. Update messages are formatted as
follows:

Datum             | Byte Size | Description
----------------- | --------: | -------------------------------------------------------------------
Object ID         | 4         | ID of the target shape. Transient (ID zero) do not support update messages.
Flags             | 2         | Was intended for object flags, but has been removed. Must be zero.
Attributes        | *         | May be a partial or full `Attributes` section ( @ref secshapemsgattributes ).

The available flags are:

Flag                | Value     | Description
------------------- | --------: | -------------------------------------------------------------------
Double Precision    | 1         | Indicates that the Attributes section contains double precision floating point values.
Limited Attributes  | 256       | Indicates not all components from `Attributes` are present
Position            | 512       | Indicates `Attributes Position` components are present. Requires `Limited Attributes` flag.
Rotation            | 1024      | Indicates `Attributes Rotation` components are present. Requires `Limited Attributes` flag.
Scale               | 2048      | Indicates `Attributes Scale` components are present. Requires `Limited Attributes` flag.
Colour              | 4096      | Indicates `Attributes Colour` components are present. Requires `Limited Attributes` flag.

All `Attributes` are present when `Limited Attributes` is not set. Otherwise only those the components indicated by the
following flags are present.

Additional object flags are not currently supported for update, however, it may be possible that in future some flags
such as `Wireframe` or `Transparent` may be toggled.

Update messages generally do not support additional data, shape specific data, though this is not a hard restriction.




*/

// clang-format on
