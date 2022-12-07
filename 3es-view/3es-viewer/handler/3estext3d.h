//
// Author: Kazys Stepanas
//
#ifndef TES_VIEWER_HANDLER_TEXT3D_H
#define TES_VIEWER_HANDLER_TEXT3D_H

#include "3es-viewer.h"

#include "3esmessage.h"

#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Shaders/DistanceFieldVector.h>
#include <Magnum/Text/Renderer.h>

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace Magnum::Text
{
class AbstractFont;
class DistanceFieldGlyphCache;
}  // namespace Magnum::Text

namespace tes::viewer::handler
{
class Text3D : public Message
{
public:
  Text3D(Magnum::Text::AbstractFont *font, std::shared_ptr<Magnum::Text::DistanceFieldGlyphCache> cache);

  void initialise() override;
  void reset() override;
  void updateServerInfo(const ServerInfoMessage &info) override;
  void beginFrame(const FrameStamp &stamp) override;
  void endFrame(const FrameStamp &stamp) override;
  void draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params) override;
  void readMessage(PacketReader &reader) override;
  void serialise(Connection &out, ServerInfoMessage &info) override;

private:
  struct TextEntry
  {
    std::string text;
    uint32_t id;
    Magnum::Matrix4 transform;
    Magnum::Color4 colour;
    double font_size = 1;
    bool screen_facing = false;
  };

  void draw(const TextEntry &text, const DrawParams &params);

  std::mutex _mutex;
  std::vector<TextEntry> _pending;
  std::vector<TextEntry> _transient;
  std::vector<uint32_t> _remove;
  std::unordered_map<uint32_t, TextEntry> _text;
  std::unique_ptr<Magnum::Text::Renderer3D> _renderer;
  /// Default transformation matrix required to get the text from facing along Z to align with the world up vector,
  /// facing back along the forward vector. That is, for @c CoordinateFrame::XYZ, we want text to face along -Y by
  /// default, with Z up.
  Magnum::Matrix4 _default_transform;

  Magnum::Text::AbstractFont *_font = nullptr;
  Magnum::Shaders::DistanceFieldVector3D _shader;
  std::shared_ptr<Magnum::Text::DistanceFieldGlyphCache> _cache;
};
}  // namespace tes::viewer::handler

#endif  // TES_VIEWER_HANDLER_TEXT3D_H
