//
// Author: Kazys Stepanas
//
#ifndef TES_VIEWER_HANDLER_TEXT2D_H
#define TES_VIEWER_HANDLER_TEXT2D_H

#include "3es-viewer.h"

#include "3esmessage.h"

#include <Corrade/PluginManager/Manager.h>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Shaders/DistanceFieldVector.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/DistanceFieldGlyphCache.h>
#include <Magnum/Text/Renderer.h>

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace tes::viewer::handler
{
class Text2D : public Message
{
public:
  Text2D(const std::string &font_name, const std::string &fonts_resources);

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
    Magnum::Vector2 position;
    Magnum::Color4 colour;
  };

  void draw(const TextEntry &text, const Magnum::Vector2 &view_size);

  std::mutex _mutex;
  std::vector<TextEntry> _pending;
  std::vector<TextEntry> _transient;
  std::vector<uint32_t> _remove;
  std::unordered_map<uint32_t, TextEntry> _text;
  std::unique_ptr<Magnum::Text::Renderer2D> _renderer;

  Corrade::PluginManager::Manager<Magnum::Text::AbstractFont> _manager;
  std::unique_ptr<Magnum::Text::AbstractFont> _font;
  Magnum::Text::DistanceFieldGlyphCache _cache;
  Magnum::Shaders::DistanceFieldVector2D _shader;
};
}  // namespace tes::viewer::handler

#endif  // TES_VIEWER_HANDLER_TEXT2D_H
