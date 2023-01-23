//
// author: Kazys Stepanas
//
#ifndef _3ESSHAPE_H_
#define _3ESSHAPE_H_

#include <3escore/CoreConfig.h>

#include "3escolour.h"
#include "3esid.h"
#include "3esmessages.h"
#include "3estransform.h"

#include <cstdint>

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable : 4251)
#endif  // WIN32

namespace tes
{
class PacketWriter;
class Resource;

/// A base class for shapes which are to be represented remotely.
///
/// A shape instance is unique represented by its @c routingId() and @c id()
/// combined. The @c routingId() can be considered a unique shape type
/// identifier (see @c ShapeHandlerIDs), while the @c id() represents the
/// shape instance.
///
/// Shape instances may be considered transient or persistent. Transient
/// shapes have an @c id() of zero and are automatically destroyed (by the
/// client) on the next frame update. Persistent shapes have a non-zero
/// @c id() and persist until an explicit @c DestroyMessage arrives.
///
/// Internally, the @c Shape class is represented by a @c CreateMessage.
/// This includes an ID, category, flags, position, rotation, scale and
/// colour. These data represent the minimal data required to represent the
/// shape and suffice for most primitive shapes. Derivations may store
/// additional data members. Derivations may also adjust the semantics of
/// some of the fields in the @c CreateMessage; e.g., the scale XYZ values
/// have a particular interpretation for the @c Capsule shape.
///
/// Shapes may be considered simple or complex (@c isComplex() reports
/// @c true). Simple shapes only need a @c writeCreate() call to be fully
/// represented, after which @c writeUpdate() may move the object. Complex
/// shapes required additional data to be fully represented and the
/// @c writeCreate() packet stream may not be large enough to hold all the
/// data. Such complex shapes will have @c writeData() called multiple times
/// with a changing progress marker.
///
/// Note that a shape which is not complex may override the @c writeCreate()
/// method and add additional data, but must always begin with the
/// @c CreateMessage. Complex shapes are only required when this is not
/// sufficient and the additional data may overflow the packet buffer.
///
/// In general, use the @c CreateMessage only where possible. If additional
/// information is required and the additional data is sufficient to fit
/// easily in a single data packet (~64KiB), then write this information
/// in @c writeCreate() immediately following the @c CreateMessage. For
/// larger data requirements, then the shape should report as complex
/// (@c isComplex() returning @c true) and this information should be written
/// in @c writeData().
///
/// The API also includes message reading functions, which creates a
/// read/write symmetry. The read methods are intended primarily for testing
/// purposes and to serve as an example of how to process messages. Whilst
/// reading methods may be used to implement a visualisation client, they
/// may lead to sub-optimal message handling and memory duplication. There may
/// also be issues with synchronising the shape ID with the intended instance.
///
/// No method for reading @c DestroyMessage is provided as it does not apply
/// modifications to the shape members.
class _3es_coreAPI Shape
{
public:
  /// Create a new shape with the given @c routingId and instance @c id .
  /// @param routingId Identifies the shape type.
  /// @param id The shape instance id.
  Shape(uint16_t routingId, const Id &id = Id(), const Transform &transform = Transform());

  /// Copy constructor.
  /// @param other Object to copy.
  Shape(const Shape &other);

  /// Virtual destructor.
  virtual inline ~Shape() = default;

  /// Return a reference name for the shape type; e.g., "box". Essentially the class name.
  ///
  /// Note this cannot be relied upon except during initial creation.
  ///
  /// @return The shape type name.
  virtual inline const char *type() const { return "unknown"; }

  /// Identifies the shape routing id. This is used to route to the correct message handler in the viewer application
  /// and essentially uniquely identifies the shape type.
  /// @return The shape routing id. See @c ShapeHandlerIDs .
  uint16_t routingId() const;

  /// Direct access to the internal data.
  /// @return The @c CreateMessage used to represent this shape.
  inline const CreateMessage &data() const { return _data; }

  /// Direct access to the shape attributes. These are stored in double precision, but transmission depends on the
  /// @c CreateMessage::flag @c OFDoublePrecision . See @c setDoublePrecision() .
  inline const ObjectAttributesd &attributes() const { return _attributes; }

  /// Access the instance id of this shape.
  ///
  /// Shapes must have either a zero id or a unique id. A zero id represents a transient shape which does not need to
  /// be explicitly deleted, while any non-zero id must be unique.
  ///
  /// @return The shape instance id.
  uint32_t id() const;

  /// Set the instance id.
  /// @param id The new shape id.
  Shape &setId(uint32_t id);

  /// Access the shape category.
  ///
  /// Categories can be used by the viewer to perform collective operations on shapes, such as disable rendering for
  /// particular categories. The category structure is hierarchical, but user defined.
  ///
  /// @return The shape's category.
  uint16_t category() const;

  /// Set the shape's category.
  /// @return category The new category value.
  Shape &setCategory(uint16_t category);

  /// Sets the wireframe flag value for this shape. Only before sending create.
  /// Not all shapes will respect the flag.
  /// @return @c *this.
  Shape &setWireframe(bool wire);
  /// Returns true if the wireframe flag is set.
  /// @return True if wireframe flag is set.
  bool wireframe() const;

  /// Sets the transparent flag value for this shape. Only before sending create.
  /// Not all shapes will respect the flag.
  /// @return @c *this.
  Shape &setTransparent(bool transparent);
  /// Returns true if the transparent flag is set.
  /// @return True if transparent flag is set.
  bool transparent() const;

  /// Sets the two sided shader flag value for this shape. Only before sending create.
  /// Not all shapes will respect the flag.
  /// @return @c *this.
  Shape &setTwoSided(bool twoSided);
  /// Returns true if the two sided shader flag is set.
  /// @return True if two sided flag is set.
  bool twoSided() const;

  /// Configures the shape to replace any previous shape with the same ID on creation.
  /// Only valid on creation.
  /// @return @c *this.
  Shape &setReplace(bool replace);
  /// Returns true set to replace pre-existing shape with the same ID.
  /// @return True if the replace flag is set.
  bool replace() const;

  /// Configures the shape to skip referencing resources for this instance. See @c ObjectFlag::OFSkipResources .
  /// Must be set on both creation and destruction.
  /// @return @c *this.
  Shape &setSkipResources(bool skip);
  /// Returns true set to skip resource referencing for this shape instance.
  /// @return True if the skip resources flag is set.
  bool skipResources() const;

  /// Configures the shape to use double or (on) single (off) precision attributes. See
  /// @c ObjectFlag::OFDoublePrecision . This is normally implicytly set by how the @c Transform() constructor argument
  /// is given.
  /// @return @c *this.
  Shape &setDoublePrecision(bool doublePrecision);
  /// Returns true set to skip resource referencing for this shape instance.
  /// @return True if the skip resources flag is set.
  bool doublePrecision() const;

  /// Set the full set of @c ObjectFlag values.
  /// This affects attributes such as @c isTwoSided() and @c isWireframe().
  /// @param flags New flag values to write.
  /// @return @c *this.
  Shape &setFlags(uint16_t flags);
  /// Retrieve the full set of @c ObjectFlag values.
  /// @return Active flag set.
  uint16_t flags() const;

  /// Update the @c position(), @c rotation(), @c scale() and @c doublePrecision() flag.
  /// @return @c *this
  Shape &setTransform(const Transform &transform);

  /// Query the @c position(), @c rotation(), @c scale() and @c doublePrecision() flag.
  Transform transform() const;

  Shape &setPosition(const Vector3d &pos);
  Vector3d position() const;

  Shape &setPosX(double p);
  Shape &setPosY(double p);
  Shape &setPosZ(double p);

  Shape &setRotation(const Quaterniond &rot);
  Quaterniond rotation() const;

  Shape &setScale(const Vector3d &scale);
  Vector3d scale() const;

  Shape &setColour(const Colour &colour);
  Colour colour() const;

  /// Is this a complex shape? Complex shapes require @c writeData() to be
  /// called.
  /// @return True if complex, false if simple.
  virtual inline bool isComplex() const { return false; }

  /// Update the attributes of this shape to match @p other.
  /// Used in maintaining cached copies of shapes. The shapes should
  /// already represent the same object.
  ///
  /// Not all attributes need to be updated. Only attributes which may be updated
  /// via an @c UpdateMessage for this shape need be copied.
  ///
  /// The default implementation copies only the @c ObjectAttributes.
  ///
  /// @param other The shape to update data from.
  virtual void updateFrom(const Shape &other);

  /// Writes the @c CreateMessage to @p stream.
  ///
  /// For simple shapes the @c CreateMessage and potential extensions will be
  /// sufficient. More complex shapes may have extensive additional used data
  /// to represent object (e.g., a point cloud). In this case the shape
  /// must report @c isComplex() as @c true and implement @c writeData().
  /// See class notes.
  ///
  /// @param stream The stream to write the @c CreateMessage to.
  /// @return @c true if the message is successfully written to @c stream.
  virtual bool writeCreate(PacketWriter &stream) const;

  /// Called only for complex shapes to write additional creation data.
  ///
  /// Complex shapes implementing this method must first write the @c DataMessage as a header identifying
  /// the shape for which the message is intended, via it's ID. The @c DataMessage acts as a header and
  /// the data format following the message is entirely dependent on the implementing shape.
  ///
  /// @param stream The data stream to write to.
  /// @param[in,out] progressMarker Indicates data transfer progress.
  ///   Initially zero, the @c Shape manages its own semantics.
  /// @return Indicates completion progress. 0 indicates completion,
  ///   1 indicates more data are available and more calls should be made.
  ///   -1 indicates an error. No more calls should be made.
  virtual inline int writeData(PacketWriter &stream, unsigned &progressMarker) const
  {
    TES_UNUSED(stream);
    TES_UNUSED(progressMarker);
    return 0;
  }

  /// Writes the @c UpdateMessage to @c stream supporting a change in
  /// @c ObjectAttributes.
  ///
  /// This method only be used when the attributes change.
  ///
  /// @param stream The stream to write the @c UpdateMessage to.
  /// @return @c true if the message is successfully written to @c stream.
  bool writeUpdate(PacketWriter &stream) const;

  /// Write a @c DestroyMessage to @c stream - only for persistent shapes.
  ///
  /// The @c id() (combined with @c routingId()) identifies which shape
  /// instance to destroy.
  ///
  /// @param stream The stream to write the @c DestroyMessage to.
  /// @return @c true if the message is successfully written to @c stream.
  bool writeDestroy(PacketWriter &stream) const;

  /// Read a @c CreateMessage for this shape. This will override the
  /// @c id() of this instance.
  ///
  /// The @c routingId() must have already been resolved.
  ///
  /// @param stream The stream to read message data from.
  /// @return @c true if the message is successfully read.
  virtual bool readCreate(PacketReader &stream);

  /// Read an @c UpdateMessage for this shape.
  ///
  /// Respects the @c UpdateFlag values, only modifying requested data.
  ///
  /// @param stream The stream to read message data from.
  /// @return @c true if the message is successfully read.
  virtual bool readUpdate(PacketReader &stream);

  /// Read back data written by @c writeData().
  ///
  /// Must be implemented by complex shapes, first reading the @c DataMessage
  /// then data payload. The base implementation returns @c false assuming a
  /// simple shape.
  ///
  /// @param stream The stream to read message data from.
  /// @return @c true if the message is successfully read.
  virtual bool readData(PacketReader &stream);

  /// Enumerate the resources used by this shape. Resources are most commonly used by
  /// mesh shapes to expose the mesh data, where the shape simply positions the mesh.
  ///
  /// The function is called to fetch the shape's resources into @p resources,
  /// up to the given @p capacity. Repeated calls may be used to fetch all resources
  /// into a smaller array by using the @p fetchOffset parameter as a marker indicating
  /// how many items have already been fetched. Regardless, data are always written to
  /// @p resources starting at index zero.
  ///
  /// This function may also be called with a @c nullptr for @p resources and/or
  /// a zero @p capacity. In this case the return value indicates the number of
  /// resources used by the shape.
  ///
  /// @param resources The array to populate with this shape's resources.
  /// @param capacity The element count capacity of @p resources.
  /// @param fetchOffset An offset used to fetch resources into an array too small to
  ///   hold all available resources. It is essentially the running sum of resources
  ///   fetched so far.
  /// @return The number of items added to @p resources when @p resources and @p capacity
  ///   are non zero. When @p resources is null or @p capacity zero, the return value
  ///   indicates the total number of resources used by the shape.
  virtual unsigned enumerateResources(const Resource **resources, unsigned capacity, unsigned fetchOffset = 0) const;

  /// Deep copy clone.
  /// @return A deep copy.
  virtual Shape *clone() const;

protected:
  /// Called when @p copy is created from this object to copy appropriate attributes to @p copy.
  ///
  /// The general use case is for a subclass to override @c clone(), creating the correct
  /// concrete type, then call @c onClone() to copy data. The advantage is that @p onClone()
  /// can recursively call up the class hierarchy.
  /// @param copy The newly cloned object to copy data to. Must not be null.
  void onClone(Shape *copy) const;

  void init(const Id &id, const Transform &transform, uint16_t flags = 0);

  uint16_t _routingId;
  CreateMessage _data;
  ObjectAttributesd _attributes;
};


inline Shape::Shape(uint16_t routingId, const Id &id, const Transform &transform)
  : _routingId(routingId)
{
  init(id, transform);
  setDoublePrecision(transform.preferDoublePrecision());
}


inline Shape::Shape(const Shape &other)
  : _routingId(other._routingId)
  , _data(other._data)
  , _attributes(other._attributes)
{}


inline void Shape::init(const Id &id, const Transform &transform, uint16_t flags)
{
  _data.id = id.id();
  _data.category = id.category();
  _data.flags = flags;
  _data.reserved = 0u;
  _attributes.colour = 0xffffffffu;
  _attributes.position[0] = transform.position()[0];
  _attributes.position[1] = transform.position()[1];
  _attributes.position[2] = transform.position()[2];
  _attributes.rotation[0] = transform.rotation()[0];
  _attributes.rotation[1] = transform.rotation()[1];
  _attributes.rotation[2] = transform.rotation()[2];
  _attributes.rotation[3] = transform.rotation()[3];
  _attributes.scale[0] = transform.scale()[0];
  _attributes.scale[1] = transform.scale()[1];
  _attributes.scale[2] = transform.scale()[2];
}


inline uint16_t Shape::routingId() const
{
  return _routingId;
}


inline uint32_t Shape::id() const
{
  return _data.id;
}


inline Shape &Shape::setId(uint32_t id)
{
  _data.id = id;
  return *this;
}


inline uint16_t Shape::category() const
{
  return _data.category;
}


inline Shape &Shape::setCategory(uint16_t category)
{
  _data.category = category;
  return *this;
}


inline Shape &Shape::setWireframe(bool wire)
{
  _data.flags = uint16_t(_data.flags & ~OFWire);
  _data.flags |= uint16_t(OFWire * !!wire);
  return *this;
}


inline bool Shape::wireframe() const
{
  return (_data.flags & OFWire) != 0;
}


inline Shape &Shape::setTransparent(bool transparent)
{
  _data.flags = uint16_t(_data.flags & ~OFTransparent);
  _data.flags |= uint16_t(OFTransparent * !!transparent);
  return *this;
}


inline bool Shape::transparent() const
{
  return (_data.flags & OFTransparent) != 0;
}


inline Shape &Shape::setTwoSided(bool twoSided)
{
  _data.flags = uint16_t(_data.flags & ~OFTwoSided);
  _data.flags |= uint16_t(OFTwoSided * !!twoSided);
  return *this;
}


inline bool Shape::twoSided() const
{
  return (_data.flags & OFTwoSided) != 0;
}


inline Shape &Shape::setReplace(bool replace)
{
  _data.flags = uint16_t(_data.flags & ~OFReplace);
  _data.flags |= uint16_t(OFReplace * !!replace);
  return *this;
}


inline bool Shape::replace() const
{
  return (_data.flags & OFReplace) != 0;
}


inline Shape &Shape::setSkipResources(bool skip)
{
  _data.flags = uint16_t(_data.flags & ~OFSkipResources);
  _data.flags |= uint16_t(OFSkipResources * !!skip);
  return *this;
}


inline bool Shape::skipResources() const
{
  return (_data.flags & OFSkipResources) != 0;
}


inline Shape &Shape::setDoublePrecision(bool doublePrecision)
{
  _data.flags = uint16_t(_data.flags & ~OFDoublePrecision);
  _data.flags |= uint16_t(OFDoublePrecision * !!doublePrecision);
  return *this;
}


inline bool Shape::doublePrecision() const
{
  return (_data.flags & OFDoublePrecision) != 0;
}


inline Shape &Shape::setFlags(uint16_t flags)
{
  _data.flags = flags;
  return *this;
}


inline uint16_t Shape::flags() const
{
  return _data.flags;
}


inline Shape &Shape::setTransform(const Transform &transform)
{
  setPosition(transform.position());
  setRotation(transform.rotation());
  setScale(transform.scale());
  setDoublePrecision(transform.preferDoublePrecision());
  return *this;
}


inline Transform Shape::transform() const
{
  Transform t(position(), rotation(), scale());
  t.setPreferDoublePrecision(doublePrecision());
  return t;
}


inline Shape &Shape::setPosition(const Vector3d &pos)
{
  _attributes.position[0] = pos[0];
  _attributes.position[1] = pos[1];
  _attributes.position[2] = pos[2];
  return *this;
}


inline Vector3d Shape::position() const
{
  return Vector3d(_attributes.position[0], _attributes.position[1], _attributes.position[2]);
}


inline Shape &Shape::setPosX(double p)
{
  _attributes.position[0] = p;
  return *this;
}


inline Shape &Shape::setPosY(double p)
{
  _attributes.position[1] = p;
  return *this;
}


inline Shape &Shape::setPosZ(double p)
{
  _attributes.position[2] = p;
  return *this;
}


inline Shape &Shape::setRotation(const Quaterniond &rot)
{
  _attributes.rotation[0] = rot[0];
  _attributes.rotation[1] = rot[1];
  _attributes.rotation[2] = rot[2];
  _attributes.rotation[3] = rot[3];
  return *this;
}


inline Quaterniond Shape::rotation() const
{
  return Quaterniond(_attributes.rotation[0], _attributes.rotation[1], _attributes.rotation[2],
                     _attributes.rotation[3]);
}


inline Shape &Shape::setScale(const Vector3d &scale)
{
  _attributes.scale[0] = scale[0];
  _attributes.scale[1] = scale[1];
  _attributes.scale[2] = scale[2];
  return *this;
}


inline Vector3d Shape::scale() const
{
  return Vector3d(_attributes.scale[0], _attributes.scale[1], _attributes.scale[2]);
}


inline Shape &Shape::setColour(const Colour &colour)
{
  _attributes.colour = colour.c;
  return *this;
}


inline Colour Shape::colour() const
{
  return Colour(_attributes.colour);
}
}  // namespace tes

#ifdef WIN32
#pragma warning(pop)
#endif  // WIN32

#endif  // _3ESSHAPE_H_
