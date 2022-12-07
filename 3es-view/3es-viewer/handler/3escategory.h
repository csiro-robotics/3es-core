//
// Author: Kazys Stepanas
//
#ifndef TES_VIEWER_HANDLER_CATEGORY_H
#define TES_VIEWER_HANDLER_CATEGORY_H

#include "3es-viewer.h"

#include "3esmessage.h"

#include <mutex>
#include <unordered_map>

namespace tes::viewer::handler
{
class Category : public Message
{
public:
  /// Represents a display category.
  struct CategoryInfo
  {
    /// Display name for the category.
    std::string name;
    /// Category ID. Zero is always the root category to which all other categories belong. It can be given an explicit
    /// name.
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
  void beginFrame(const FrameStamp &stamp) override;
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
}  // namespace tes::viewer::handler

#endif  // TES_VIEWER_HANDLER_CATEGORY_H
