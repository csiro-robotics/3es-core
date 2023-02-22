//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SHAPES_MESH_H
#define TES_CORE_SHAPES_MESH_H

#include <3escore/CoreConfig.h>

#include "Shape.h"

#include <3escore/CoreUtil.h>
#include <3escore/IntArg.h>
#include <3escore/Matrix4.h>
#include <3escore/Ptr.h>

#include <cstdint>

namespace tes
{
class MeshResource;

/// Represents a mesh shape. Requires a @c MeshResource parts to get represent mesh topology.
/// The shape never owns the @c MeshResource parts and they must outlive the shape.
class TES_CORE_API MeshSet : public Shape
{
public:
  /// Pointer type used for referencing a @c MeshResource. Uses the @c Ptr type to allow borrowed
  /// or shared semantics.
  using MeshResourcePtr = Ptr<const MeshResource>;

  /// Create a shape with a @c part_count parts. Use @c setPart() to populate.
  /// @param part_count The number of parts to the mesh.
  /// @param id The unique mesh shape ID, zero for transient (not recommended for mesh shapes).
  /// @param category The mesh shape category.
  MeshSet(const Id &id = Id(), const UIntArg &part_count = 0);
  /// Create a shape with a single @p part with transform matching the shape transform.
  /// @param part The mesh part.
  /// @param id The unique mesh shape ID, zero for transient (not recommended for mesh shapes).
  /// @param category The mesh shape category.
  MeshSet(MeshResourcePtr part, const Id &id = Id());

  /// Copy constructor
  /// @param other Object to copy.
  MeshSet(const MeshSet &other);

  /// Move constructor.
  /// @param other Object to move.
  MeshSet(MeshSet &&other) noexcept;

  /// Destructor.
  ~MeshSet() override;

  [[nodiscard]] const char *type() const override { return "meshSet"; }

  /// Get the number of parts to this shape.
  /// @return The number of parts this shape has.
  [[nodiscard]] unsigned partCount() const;
  /// Set the part at the given index.
  /// @param index The part index to set. Should be in the range <tt>[0, partCount())</tt>. Ignored
  /// when out of range.
  /// @param part The mesh data to set at @p index.
  /// @param transform The transform for this part, relative to this shape's transform.
  ///     This transform may not be updated after the shape is sent to a client.
  /// @param colour Tint to apply just to this part.
  /// @return @c *this
  MeshSet &setPart(const UIntArg &index, MeshResourcePtr part, const Transform &transform,
                   const Colour &colour = Colour(255, 255, 255));
  /// Fetch the part resource at the given @p index.
  /// @param index The part index to fetch. Must be in the range <tt>[0, partCount())</tt>.
  /// @return The mesh at the given index.
  [[nodiscard]] MeshResourcePtr partResource(const UIntArg &index) const;
  /// Fetch the transform for the part at the given @p index.
  /// @param index The part transform to fetch. Must be in the range <tt>[0, partCount())</tt>.
  /// @return The transform for the mesh at the given index.
  [[nodiscard]] const Transform &partTransform(const UIntArg &index) const;
  /// Fetch the colour tint for the part at the given @p index.
  /// @param index The part transform to fetch. Must be in the range <tt>[0, partCount())</tt>.
  /// @return The colour tint of mesh at the given index.
  [[nodiscard]] const Colour &partColour(const UIntArg &index) const;

  /// Overridden to include the number of mesh parts, their IDs and transforms.
  bool writeCreate(PacketWriter &stream) const override;

  /// Reads the @c CreateMessage and details about the mesh parts.
  ///
  /// Successfully reading the message modifies the data in this shape such
  /// that the parts (@c partResource()) are only dummy resources
  /// (@c MeshPlaceholder). This identifies the resource IDs, but the data
  /// must be resolved separately.
  ///
  /// @param stream The stream to read from.
  /// @return @c true on success.
  bool readCreate(PacketReader &stream) override;

  /// Enumerate the mesh resources for this shape.
  /// @todo Add material resources.
  unsigned enumerateResources(std::vector<ResourcePtr> &resources) const override;

  /// Clone the mesh shape. @c MeshResource objects are shared.
  /// @return The cloned shape.
  [[nodiscard]] std::shared_ptr<Shape> clone() const override;

protected:
  void onClone(MeshSet &copy) const;

private:
  struct Part
  {
    MeshResourcePtr resource;
    Transform transform = Transform::identity();
    Colour colour = Colour(255, 255, 255);

    Part() = default;
    Part(MeshResourcePtr resource)
      : resource(std::move(resource))
    {}
    Part(MeshResourcePtr resource, Transform transform)
      : resource(std::move(resource))
      , transform(std::move(transform))
    {}
    Part(MeshResourcePtr resource, Transform transform, const Colour &colour)
      : resource(std::move(resource))
      , transform(std::move(transform))
      , colour(colour)
    {}
  };

  std::vector<Part> _parts;
};

inline unsigned MeshSet::partCount() const
{
  return int_cast<unsigned>(_parts.size());
}

inline MeshSet &MeshSet::setPart(const UIntArg &index, MeshResourcePtr part,
                                 const Transform &transform, const Colour &colour)
{
  if (index.i < _parts.size())
  {
    _parts[index.i].resource = std::move(part);
    _parts[index.i].transform = transform;
    _parts[index.i].colour = colour;
  }
  return *this;
}

inline MeshSet::MeshResourcePtr MeshSet::partResource(const UIntArg &index) const
{
  return _parts[index.i].resource;
}

inline const Transform &MeshSet::partTransform(const UIntArg &index) const
{
  return _parts[index.i].transform;
}

inline const Colour &MeshSet::partColour(const UIntArg &index) const
{
  return _parts[index.i].colour;
}
}  // namespace tes

#endif  // TES_CORE_SHAPES_MESH_H
