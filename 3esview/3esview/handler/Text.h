//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_HANDLER_TEXT_H
#define TES_VIEW_HANDLER_TEXT_H

#include <3esview/ViewConfig.h>

#include "Message.h"

#include <3esview/MagnumColour.h>
#include <3esview/MagnumV3.h>
#include <3esview/painter/Text.h>
#include <3esview/util/PendingActionQueue.h>

#include <3escore/Connection.h>
#include <3escore/Log.h>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/DistanceFieldGlyphCache.h>

#include <cctype>
#include <limits>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace tes::view::handler
{
/// A template abstraction which handles messages for either @c tes::Text2D or @c tes::Text3D via
/// @c Affordances.
///
/// @c Affordances expects the following API:
///
/// @code
/// /// Affordances API for use with the @c Text message handler.
/// class Affordances
/// {
/// public:
///   /// Identifies the text drawing mode as using @c painter::Text::draw2D() - when @c true - or
///   /// using @c painter::Text::draw3D() - when @c false .
///   /// @return True to draw 2D text, false to draw 3D text.
///   constexpr static bool is2D();
///
///   /// Configure a @c painter::Text::TextEntry from a @c TextShape .
///   /// @param shape The source shape data.
///   /// @param entry The entry to configure.
///   static void configure(const TextShape &shape, painter::Text::TextEntry &entry);
///
///   /// Configure a @c TextShape from a @c painter::Text::TextEntry .
///   /// @param entry The source entry data.
///   /// @param shape The shape to configure.
///   static void configure(const painter::Text::TextEntry &entry, TextShape &shape);
/// };
/// @endcode
///
/// @tparam TextShape Either @c tes::Text2D or @c tes::Text3D .
/// @tparam Affordances Helper class which effects specialisations for the @c TextShape .
template <typename TextShape, typename Affordances>
class Text : public Message
{
public:
  /// Text entry alias.
  using TextEntry = painter::Text::TextEntry;
  /// Pending action queue.
  using PendingQueue = util::PendingActionQueue<TextShape>;

  /// Constructor.
  /// @param routing_id The message routing ID for the supported text messages.
  /// @param name The message handler name.
  /// @param painter The text drawing API.
  Text(uint16_t routing_id, const std::string &name, std::shared_ptr<painter::Text> painter);

  void initialise() override;
  void reset() override;
  void prepareFrame(const FrameStamp &stamp) override;
  void endFrame(const FrameStamp &stamp) override;
  void draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params) override;
  void readMessage(PacketReader &reader) override;
  void serialise(Connection &out, ServerInfoMessage &info) override;

private:
  /// Handle creation for a new shape.
  /// @param shape The shape to instantiate.
  /// @return True on success.
  bool create(const TextShape &shape);
  /// Update the position and/or colour of a text shape.
  /// @param shape_id The ID of the shape to update. Transient shapes (zero ID) cannot be updated.
  /// @param update Details of the update message.
  /// @return True on success.
  bool update(uint32_t shape_id, const typename PendingQueue::Action::Update &update);
  /// Destroy a text shape.
  /// @param shape_id The ID of the shape to update. Transient shapes (zero ID) cannot be destroyed.
  /// @return True on success.
  bool destroy(uint32_t shape_id);

  std::mutex _mutex;
  PendingQueue _pending_queue;
  std::vector<TextEntry> _transient;
  std::unordered_map<uint32_t, TextEntry> _text;
  std::shared_ptr<painter::Text> _painter;
};


template <typename TextShape, typename Affordances>
Text<TextShape, Affordances>::Text(uint16_t routing_id, const std::string &name,
                                   std::shared_ptr<painter::Text> painter)
  : Message(routing_id, name)
  , _painter(std::move(painter))
{}


template <typename TextShape, typename Affordances>
void Text<TextShape, Affordances>::initialise()
{}


template <typename TextShape, typename Affordances>
void Text<TextShape, Affordances>::reset()
{
  std::lock_guard guard(_mutex);
  _pending_queue.clear();
  _transient.clear();
  _text.clear();
}


template <typename TextShape, typename Affordances>
void Text<TextShape, Affordances>::prepareFrame(const FrameStamp &stamp)
{
  TES_UNUSED(stamp);
}


template <typename TextShape, typename Affordances>
void Text<TextShape, Affordances>::endFrame(const FrameStamp &stamp)
{
  std::lock_guard guard(_mutex);
  _transient.clear();
  _pending_queue.mark(stamp.frame_number);
  for (const auto &action : _pending_queue.view(stamp.frame_number))
  {
    switch (action.action)
    {
    default:
    case PendingQueue::Action::Kind::None:
      break;
    case PendingQueue::Action::Kind::Create:
      create(action.create.shape);
      break;
    case PendingQueue::Action::Kind::Update:
      update(action.shape_id, action.update);
      break;
    case PendingQueue::Action::Kind::Destroy:
      destroy(action.shape_id);
      break;
    }
  }
}


template <typename TextShape, typename Affordances>
void Text<TextShape, Affordances>::draw(DrawPass pass, const FrameStamp &stamp,
                                        const DrawParams &params)
{
  TES_UNUSED(stamp);
  TES_UNUSED(params);
  if (pass != DrawPass::Overlay)
  {
    return;
  }

  if constexpr (Affordances::is2D())
  {
    _painter->draw2D(
      _transient.begin(), _transient.end(),
      [](const std::vector<TextEntry>::iterator &iter) { return *iter; }, params);
    _painter->draw2D(
      _text.begin(), _text.end(),
      [](const std::unordered_map<uint32_t, TextEntry>::iterator &iter) { return iter->second; },
      params);
  }
  else
  {
    _painter->draw3D(
      _transient.begin(), _transient.end(),
      [](const std::vector<TextEntry>::iterator &iter) { return *iter; }, params);
    _painter->draw3D(
      _text.begin(), _text.end(),
      [](const std::unordered_map<uint32_t, TextEntry>::iterator &iter) { return iter->second; },
      params);
  }
}


template <typename TextShape, typename Affordances>
void Text<TextShape, Affordances>::readMessage(PacketReader &reader)
{
  PendingQueue::Action action = {};
  switch (reader.messageId())
  {
  case OIdCreate: {
    action.action = PendingQueue::ActionKind::Create;
    if (!action.create.shape.readCreate(reader))
    {
      log::error("Failed to read create for ", name());
      return;
    }
    action.shape_id = action.create.shape.id();
    _pending_queue.emplace_back(action);
    break;
  }
  case OIdUpdate: {
    UpdateMessage update = {};
    ObjectAttributes attrs = {};
    if (!update.read(reader, attrs))
    {
      log::error("Failed to read update for ", name());
      return;
    }

    action.action = PendingQueue::ActionKind::Update;
    action.shape_id = update.id;
    action.update.flags = update.flags;
    action.update.position = Vector3d(attrs.position);
    action.update.rotation = Quaterniond(attrs.rotation);
    action.update.scale = Vector3d(attrs.scale);
    action.update.colour = Colour(attrs.colour);
    _pending_queue.emplace_back(action);
  }
  case OIdDestroy: {
    DestroyMessage destroy = {};
    if (!destroy.read(reader))
    {
      log::error("Failed to read destroy for ", name());
      return;
    }
    action.action = PendingQueue::ActionKind::Destroy;
    action.shape_id = destroy.id;
    _pending_queue.emplace_back(action);
    break;
  }
  default:
    log::error("Unsupported ", name(), " message ID: ", reader.messageId());
    break;
  }
}


template <typename TextShape, typename Affordances>
void Text<TextShape, Affordances>::serialise(Connection &out, ServerInfoMessage &info)
{
  TES_UNUSED(info);
  TextShape shape;

  const auto write_shape = [&out, &shape](uint32_t id, const TextEntry &text) {
    shape.setId(id);
    Affordances::configure(text, shape);
    if (out.create(shape) < 0)
    {
      log::error("Error writing text 2D shape.");
    }
  };

  for (const auto &text : _transient)
  {
    write_shape(0, text);
  }

  for (const auto &[id, text] : _text)
  {
    write_shape(id, text);
  }
}

template <typename TextShape, typename Affordances>
bool Text<TextShape, Affordances>::create(const TextShape &shape)
{
  TextEntry entry = {};
  Affordances::configure(shape, entry);

  if (shape.isTransient())
  {
    _transient.emplace_back(entry);
  }
  else
  {
    _text[shape.id()] = entry;
  }
  return true;
}


template <typename TextShape, typename Affordances>
bool Text<TextShape, Affordances>::update(uint32_t shape_id,
                                          const typename PendingQueue::Action::Update &update)
{
  if (shape_id == 0)
  {
    // Can't update transients.
    return false;
  }

  const auto search = _text.find(shape_id);
  if (search == _text.end())
  {
    return false;
  }

  auto &entry = search->second;

  // Use ObjectAttributes as an intermediary so we can use composeTransform().
  // No need to set colour. That is unused by composeTransform()
  ObjectAttributes attrs = {};
  attrs.position[0] = static_cast<Magnum::Float>(update.position[0]);
  attrs.position[1] = static_cast<Magnum::Float>(update.position[1]);
  attrs.position[2] = static_cast<Magnum::Float>(update.position[2]);
  attrs.rotation[0] = static_cast<Magnum::Float>(update.rotation[0]);
  attrs.rotation[1] = static_cast<Magnum::Float>(update.rotation[1]);
  attrs.rotation[2] = static_cast<Magnum::Float>(update.rotation[2]);
  attrs.rotation[3] = static_cast<Magnum::Float>(update.rotation[3]);
  attrs.scale[0] = static_cast<Magnum::Float>(update.scale[0]);
  attrs.scale[1] = static_cast<Magnum::Float>(update.scale[1]);
  attrs.scale[2] = static_cast<Magnum::Float>(update.scale[2]);

  entry.transform = composeTransform(attrs);
  entry.colour = tes::view::convert(update.colour);

  return true;
}


template <typename TextShape, typename Affordances>
bool Text<TextShape, Affordances>::destroy(uint32_t shape_id)
{
  const auto search = _text.find(shape_id);
  if (search == _text.end())
  {
    return false;
  }

  _text.erase(search);
  return true;
}
}  // namespace tes::view::handler

#endif  // TES_VIEW_HANDLER_TEXT_H
