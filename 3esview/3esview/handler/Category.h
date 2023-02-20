//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_HANDLER_CATEGORY_H
#define TES_VIEW_HANDLER_CATEGORY_H

#include <3esview/ViewConfig.h>

#include "Message.h"

#include <mutex>
#include <unordered_map>

namespace tes::view::handler
{
class TES_VIEWER_API Category : public Message
{
public:
  /// Represents a display category.
  struct TES_VIEWER_API CategoryInfo
  {
    /// Display name for the category.
    std::string name;
    /// Category ID. Zero is always the root category to which all other categories belong. It can
    /// be given an explicit name.
    uint16_t id = 0;
    /// Parent category, defaulting to the root ID.
    uint16_t parent_id = 0;
    /// Does this category default to the active state?
    bool default_active = false;
    /// Currently active?
    bool active = false;
  };

  Category();

  bool isActive(unsigned category) const;
  bool setActive(unsigned category, bool active = true);

  bool lookup(unsigned category, CategoryInfo &info);

  void initialise() override;
  void reset() override;
  void prepareFrame(const FrameStamp &stamp) override;
  void endFrame(const FrameStamp &stamp) override;
  void draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params) override;
  void readMessage(PacketReader &reader) override;
  void serialise(Connection &out, ServerInfoMessage &info) override;

private:
  bool updateCategory(const CategoryInfo &info);

  using CategoryMap = std::unordered_map<unsigned, CategoryInfo>;
  mutable std::mutex _mutex;
  CategoryMap _category_map;
};
}  // namespace tes::view::handler

#endif  // TES_VIEW_HANDLER_CATEGORY_H
