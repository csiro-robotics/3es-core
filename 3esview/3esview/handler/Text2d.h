//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_HANDLER_TEXT2D_H
#define TES_VIEW_HANDLER_TEXT2D_H

#include <3esview/ViewConfig.h>

#include "Message.h"

#include <3esview/painter/Text.h>

#include <Magnum/Magnum.h>

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace tes::view::handler
{
class TES_VIEWER_API Text2D : public Message
{
public:
  using TextEntry = painter::Text::TextEntry;

  Text2D(std::shared_ptr<painter::Text> painter);

  void initialise() override;
  void reset() override;
  void beginFrame(const FrameStamp &stamp) override;
  void endFrame(const FrameStamp &stamp) override;
  void draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params) override;
  void readMessage(PacketReader &reader) override;
  void serialise(Connection &out, ServerInfoMessage &info) override;

private:
  std::mutex _mutex;
  std::vector<std::pair<uint32_t, TextEntry>> _pending;
  std::vector<TextEntry> _transient;
  std::vector<uint32_t> _remove;
  std::unordered_map<uint32_t, TextEntry> _text;
  std::shared_ptr<painter::Text> _painter;
};
}  // namespace tes::view::handler

#endif  // TES_VIEW_HANDLER_TEXT2D_H
