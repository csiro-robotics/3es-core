
#ifndef TES_VIEW_HANDLER_SHAPE_H
#define TES_VIEW_HANDLER_SHAPE_H

#include <3esview/ViewConfig.h>

#include "Message.h"

#include <iosfwd>
#include <memory>
#include <unordered_map>

namespace tes::view::painter
{
class ShapePainter;
}

namespace tes::view::handler
{
/// A common message handler for all primitive shapes, rendered using a @c painter::ShapePainter.
class TES_VIEWER_API Shape : public Message
{
public:
  Shape(uint16_t routing_id, const std::string &name,
        std::shared_ptr<painter::ShapePainter> painter);

  void initialise() override;
  void reset() override;
  void prepareFrame(const FrameStamp &stamp) override;
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
  virtual bool handleCreate(const CreateMessage &msg, const ObjectAttributes &attrs,
                            PacketReader &reader);
  virtual bool handleUpdate(const UpdateMessage &msg, const ObjectAttributes &attrs,
                            PacketReader &reader);
  virtual bool handleDestroy(const DestroyMessage &msg, PacketReader &reader);
  virtual bool handleData(const DataMessage &msg, PacketReader &reader);

private:
  /// Data stored about any multi-shape entries.
  struct MultiShapeInfo
  {
    /// Number of child shapes.
    unsigned shape_count = 0;
    /// Expect double precision attributes?
    bool double_precision = false;
  };

  std::shared_ptr<painter::ShapePainter> _painter;
  /// Map of multi-shape attributes. We need to use some of this information when unpacking the data
  /// messages.
  std::unordered_map<uint32_t, MultiShapeInfo> _multi_shapes;
  /// The last transient multi-shape info. We use this when unpacking a transient multi-shape data
  /// message.
  MultiShapeInfo _last_transient_multi_shape;
};
}  // namespace tes::view::handler

#endif  // TES_VIEW_HANDLER_SHAPE_H
