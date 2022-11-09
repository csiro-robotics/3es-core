
#ifndef TES_VIEWER_HANDLER_SHAPE_H
#define TES_VIEWER_HANDLER_SHAPE_H

#include "3es-viewer.h"

#include "3esmessage.h"

#include <iosfwd>
#include <memory>

namespace tes::viewer::painter
{
class ShapePainter;
}

namespace tes::viewer::handler
{
/// Defines the base functionality for @c handler::Message objects which add 3D objects to the scene.
///
/// The base class handles incoming create, update and destroy messages
/// with consideration given to transient and non-transient and solid or wire frame
/// shapes.
///
/// The @c handler::Shape class uses a @c painter::Shape to track shapes to draw.
///
/// @note out of date
/// Derivations must define the <see cref="SolidMesh"/> and <see cref="WireframeMesh"/>
/// properties to yield the solid and wire frame mesh objects respectively. Derivations
/// must also complete the <see cref="Shape3D.MeshSetHandler"/> definition by implementing the
/// <see cref="Shape3D.MeshSetHandler.Name"/> and <see cref="Shape3D.MeshSetHandler.RoutingID"/>
/// properties.
///
/// @note out of date
/// Derivations may optionally override the default message handling, or parts thereof.
/// Most commonly, the methods <see cref="DecodeTransform"/> and <see cref="EncodeAttributes"/>
/// may be overridden.
class Shape : public Message
{
public:
  using ObjectAttributes = tes::ObjectAttributes<float>;

  Shape(uint16_t routing_id, const std::string &name, std::shared_ptr<painter::ShapePainter> painter);

  void initialise() override;
  void reset() override;
  void beginFrame(unsigned frame_number, unsigned render_mark, bool maintain_transient) override;
  void endFrame(unsigned frame_number, unsigned render_mark) override;

  void draw(DrawPass pass, unsigned frame_number, unsigned render_mark,
            const Magnum::Matrix4 &projection_matrix) override;

  void readMessage(PacketReader &reader) override;
  void serialise(Connection &out, ServerInfoMessage &info) override;

  /// Compose the object transform from the given object attributs.
  /// @param attrs Object attributes as read from the message payload.
  /// @return The matrix transformation for the shape.
  virtual Magnum::Matrix4 composeTransform(const ObjectAttributes &attrs) const;
  /// Decompose the object transform to the given object attributs.
  /// @param transform The transormation matrix to decompose.
  /// @param attrs Object attributes to write to.
  /// @return The matrix transformation for the shape.
  virtual void decomposeTransform(const Magnum::Matrix4 &transform, ObjectAttributes &attrs) const;

protected:
  virtual bool handleCreate(const CreateMessage &msg, const ObjectAttributes &attrs, PacketReader &reader);
  virtual bool handleUpdate(const UpdateMessage &msg, const ObjectAttributes &attrs, PacketReader &reader);
  virtual bool handleDestroy(const DestroyMessage &msg, PacketReader &reader);
  virtual bool handleData(const DataMessage &msg, PacketReader &reader);

private:
  std::shared_ptr<painter::ShapePainter> _painter;
};
}  // namespace tes::viewer::handler

#endif  // TES_VIEWER_HANDLER_SHAPE_H
