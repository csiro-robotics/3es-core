
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
/// A common message handler for all primitive shapes, rendered using a @c painter::ShapePainter.
class Shape : public Message
{
public:
  Shape(uint16_t routing_id, const std::string &name, std::shared_ptr<painter::ShapePainter> painter);

  void initialise() override;
  void reset() override;
  void beginFrame(const FrameStamp &stamp) override;
  void endFrame(const FrameStamp &stamp) override;

  void draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params) override;

  void readMessage(PacketReader &reader) override;
  void serialise(Connection &out, ServerInfoMessage &info) override;

  /// Compose the object transform from the given object attributes.
  /// @param attrs Object attributes as read from the message payload.
  /// @return The matrix transformation for the shape.
  virtual Magnum::Matrix4 composeTransform(const ObjectAttributes &attrs) const;
  /// Decompose the object transform to the given object attributes.
  /// @param transform The transformation matrix to decompose.
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
