//
// Author: Kazys Stepanas
//
#include "3escategory.h"

#include <3eslog.h>
#include <3esconnection.h>
#include <3esmessages.h>
#include <3espacketwriter.h>

#include <array>

namespace tes::viewer::handler
{
Category::Category()
  : Message(MtCategory, "category")
{}


bool Category::isActive(unsigned category) const
{
  std::lock_guard guard(_mutex);
  auto search = _category_map.find(category);
  bool active = true;
  while (active && search != _category_map.end())
  {
    active = search->second.active;
    // Recurse on parent unless we are the root, or we know it's not active.
    search = (search->first != 0) ? _category_map.find(search->second.parent_id) : _category_map.end();
  }
  return active;
}


bool Category::setActive(unsigned category, bool active)
{
  std::lock_guard guard(_mutex);
  const auto search = _category_map.find(category);
  if (search != _category_map.end())
  {
    search->second.active = active;
    return true;
  }
  return false;
}


bool Category::lookup(unsigned category, CategoryInfo &info)
{
  std::lock_guard guard(_mutex);
  const auto search = _category_map.find(category);
  if (search != _category_map.end())
  {
    info = search->second;
    return true;
  }
  return false;
}


void Category::initialise()
{}

void Category::reset()
{
  std::lock_guard guard(_mutex);
  _category_map.clear();
}


void Category::updateServerInfo(const ServerInfoMessage &info)
{}


void Category::beginFrame(const FrameStamp &stamp)
{}


void Category::endFrame(const FrameStamp &stamp)
{}


void Category::draw(DrawPass pass, const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix)
{}


void Category::readMessage(PacketReader &reader)
{
  bool ok = false;
  switch (reader.messageId())
  {
  case CategoryNameMessage::MessageId: {
    CategoryNameMessage msg;
    std::array<char, 8 * 1024u> name;
    ok = msg.read(reader, name.data(), name.size());
    if (ok)
    {
      CategoryInfo info = {};
      info.name = msg.name;
      info.id = msg.categoryId;
      info.parent_id = msg.parentId;
      info.default_active = msg.defaultActive != 0;
      info.active = info.default_active;
      ok = updateCategory(info);
    }

    if (!ok)
    {
      log::error("Failed to decode category message.");
    }
    break;
  }
  default:
    log::error("Unsupported category message ID: ", reader.messageId());
  }
}


void Category::serialise(Connection &out, ServerInfoMessage &info)
{
  std::lock_guard guard(_mutex);
  CategoryNameMessage msg = {};
  const std::string error_str = "<error>";
  bool ok = true;

  std::vector<uint8_t> packet_buffer(1024u, 0u);
  PacketWriter writer(packet_buffer.data(), packet_buffer.size());
  for (auto &[id, info] : _category_map)
  {
    msg.categoryId = info.id;
    msg.parentId = info.parent_id;
    if (info.name.length() < std::numeric_limits<decltype(msg.nameLength)>::max())
    {
      msg.name = info.name.c_str();
      msg.nameLength = info.name.size();
    }
    else
    {
      msg.name = error_str.c_str();
      msg.nameLength = error_str.size();
    }
    msg.defaultActive = (info.default_active) ? 1 : 0;

    writer.reset(routingId(), CategoryNameMessage::MessageId);
    ok = msg.write(writer) && ok;
    ok = writer.finalise() && ok;
    ok = out.send(writer) >= 0 && ok;
  }

  if (!ok)
  {
    log::error("Category serialisation failed.");
  }
}


bool Category::updateCategory(const CategoryInfo &info)
{
  std::lock_guard guard(_mutex);
  _category_map[info.id] = info;
  return true;
}
}  // namespace tes::viewer::handler
