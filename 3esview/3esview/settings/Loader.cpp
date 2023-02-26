//
// Author: Kazys Stepanas
//
#include "Loader.h"

#include <3esview/3p/cfgpath.h>

#include <3escore/Colour.h>

#include <ryml/ryml.hpp>
#include <ryml/ryml_std.hpp>

#include <c4/format.hpp>

#include <array>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

namespace tes::view::settings
{
namespace
{
bool parse(const ryml::NodeRef &node, Bool &value)
{
  if (node.empty())
  {
    return true;
  }

  std::string str = node.val().data();
  std::transform(str.begin(), str.end(), str.begin(), [](char ch) { return ::tolower(ch); });
  if (str == "1" || str == "on" || str == "yes" || str == "true")
  {
    value.setValue(true);
    return true;
  }
  if (str == "0" || str == "off" || str == "no" || str == "false")
  {
    value.setValue(false);
    return true;
  }
  return false;
}


bool parse(const ryml::NodeRef &node, Colour &value)
{
  if (node.empty())
  {
    return true;
  }

  std::array<ryml::NodeRef, 3> nodes = {
    node["red"],
    node["green"],
    node["blue"],
  };
  std::array<tes::Colour::Channel, 3> channels = { tes::Colour::Channel::R, tes::Colour::Channel::G,
                                                   tes::Colour::Channel::B };

  auto colour = value.value();
  for (size_t i = 0; i < nodes.size(); ++i)
  {
    std::string str = nodes[i].val().data();
    std::istringstream in(str);
    int channel = {};
    in >> channel;
    if (in.bad())
    {
      return false;
    }

    colour.channel(channels[i]) = static_cast<uint8_t>(channel);
  }

  value.setValue(colour);

  return true;
}  // namespace


template <typename T>
bool parse(const ryml::NodeRef &node, Numeric<T> &value)
{
  if (node.empty())
  {
    return true;
  }

  const std::string str = node.val().data();
  std::istringstream in(str);
  auto temp = value.value();

  in >> temp;
  if (!in.bad())
  {
    value.setValue(temp);
    return true;
  }

  return false;
}


template <typename E>
bool parse(const ryml::NodeRef &node, Enum<E> &value)
{
  if (node.empty())
  {
    return true;
  }

  // use case insensitive compare
  std::string str = node.val().at();
  std::transform(str.begin(), str.end(), str.begin(), [](char ch) { return ::tolower(ch); });

  const auto named_values = value.namedValues();
  for (size_t i = 0; i < named_values.size(); ++i)
  {
    auto name = named_values[i].second;
    std::transform(name.begin(), name.end(), name.begin(), [](char ch) { return ::tolower(ch); });
    if (str == name)
    {
      value.setValue(named_values[i].first);
      return true;
    }
  }
  return false;
}


template <typename T>
bool write(ryml::NodeRef &parent, T &prop)
{
  std::ostringstream out;
  out << prop.value() << std::flush;
  const auto str = out.str();
  const ryml::csubstr rstr_val(str.c_str(), str.size());
  parent[ryml::csubstr(prop.label().c_str(), prop.label().size())] = rstr_val;
  return true;
}


bool write(ryml::NodeRef &parent, Bool &prop)
{
  const auto key = ryml::csubstr(prop.label().c_str(), prop.label().size());
  if (prop.value())
  {
    parent[key] = "true";
  }
  else
  {
    parent[key] = "false";
  }
  return true;
}


bool write(ryml::NodeRef &parent, Colour &prop)
{
  auto node = parent[ryml::csubstr(prop.label().c_str(), prop.label().size())];
  const std::array<std::pair<std::string, int>, 3> values = {
    std::pair{ "red", static_cast<int>(prop.value().red()) },
    std::pair{ "green", static_cast<int>(prop.value().green()) },
    std::pair{ "blue", static_cast<int>(prop.value().blue()) },
  };

  for (const auto &[name, value] : values)
  {
    const auto str_val = std::to_string(value);
    const ryml::csubstr rstr_val(str_val.c_str(), str_val.size());
    node[ryml::csubstr(name.c_str(), name.size())] = rstr_val;
  }

  return true;
}

template <typename T>
bool parse(const ryml::NodeRef &parent, T &prop)
{
  return parse(parent[prop.label()], prop);
}


bool load(const ryml::NodeRef &node, Camera &camera)
{
  bool ok = true;
  ok = parse(node, camera.invert_y) && ok;
  ok = parse(node, camera.allow_remote_settings) && ok;
  ok = parse(node, camera.near_clip) && ok;
  ok = parse(node, camera.far_clip) && ok;
  ok = parse(node, camera.fov) && ok;

  return ok;
}


bool save(ryml::NodeRef node, const Camera &camera)
{
  bool ok = true;
  ok = write(node, camera.invert_y) && ok;
  ok = write(node, camera.allow_remote_settings) && ok;
  ok = write(node, camera.near_clip) && ok;
  ok = write(node, camera.far_clip) && ok;
  ok = write(node, camera.fov) && ok;

  return ok;
}


bool load(const ryml::NodeRef &node, Log &log)
{
  bool ok = true;
  ok = parse(node, log.log_window_size) && ok;
  return ok;
}


bool save(ryml::NodeRef &node, const Log &log)
{
  bool ok = true;
  ok = write(node, log.log_window_size) && ok;
  return ok;
}


bool load(const ryml::NodeRef &node, Playback &playback)
{
  bool ok = true;
  ok = parse(node, playback.allow_key_frames) && ok;
  ok = parse(node, playback.keyframe_every_mib) && ok;
  ok = parse(node, playback.keyframe_every_frames) && ok;
  ok = parse(node, playback.keyframe_skip_forward_frames) && ok;
  ok = parse(node, playback.keyframe_compression) && ok;
  ok = parse(node, playback.looping) && ok;
  ok = parse(node, playback.pause_on_error) && ok;
  return ok;
}


bool save(ryml::NodeRef &node, const Playback &playback)
{
  bool ok = true;
  ok = write(node, playback.allow_key_frames) && ok;
  ok = write(node, playback.keyframe_every_mib) && ok;
  ok = write(node, playback.keyframe_every_frames) && ok;
  ok = write(node, playback.keyframe_skip_forward_frames) && ok;
  ok = write(node, playback.keyframe_compression) && ok;
  ok = write(node, playback.looping) && ok;
  ok = write(node, playback.pause_on_error) && ok;
  return ok;
}


bool load(const ryml::NodeRef &node, Render &render)
{
  bool ok = true;
  ok = parse(node, render.use_edl_shader) && ok;
  ok = parse(node, render.edl_radius) && ok;
  ok = parse(node, render.edl_exponential_scale) && ok;
  ok = parse(node, render.edl_linear_scale) && ok;
  ok = parse(node, render.point_size) && ok;
  ok = parse(node, render.background_colour) && ok;
  return ok;
}


bool save(ryml::NodeRef &node, const Render &render)
{
  bool ok = true;
  ok = write(node, render.use_edl_shader) && ok;
  ok = write(node, render.edl_radius) && ok;
  ok = write(node, render.edl_exponential_scale) && ok;
  ok = write(node, render.edl_linear_scale) && ok;
  ok = write(node, render.point_size) && ok;
  ok = write(node, render.background_colour) && ok;
  return ok;
}

std::filesystem::path userConfigPath()
{
  std::array<char, 1024> user_config_path;
  get_user_config_file(user_config_path.data(), static_cast<unsigned>(user_config_path.size()),
                       "3rdEyeScene");
  std::filesystem::path path{ std::string(user_config_path.data()) };
  path = path.replace_extension("yaml");
  return path;
}
}  // namespace


bool load(Settings::Config &config)
{
  return load(config, userConfigPath());
}


bool load(Settings::Config &config, const std::filesystem::path &path)
{
  std::ifstream in_file(path.c_str(), std::ios::binary);
  if (!in_file.is_open())
  {
    return false;
  }

  in_file.seekg(0, std::ios_base::end);
  const auto byte_count = in_file.tellg();
  in_file.seekg(0, std::ios_base::beg);

  std::vector<char> content(byte_count);
  in_file.read(content.data(), content.size());

  ryml::Tree doc = ryml::parse_in_place(ryml::to_substr(content));

  if (doc.empty())
  {
    return true;
  }

  const auto root = doc.rootref();
  if (root.empty())
  {
    return true;
  }

  bool ok = true;
  ok = load(root["camera"], config.camera) && ok;
  ok = load(root["log"], config.log) && ok;
  ok = load(root["playback"], config.playback) && ok;
  ok = load(root["render"], config.render) && ok;
  return ok;
}


bool save(const Settings::Config &config)
{
  return save(config, userConfigPath());
}


bool save(const Settings::Config &config, const std::filesystem::path &path)
{
  std::ofstream out_file(path.c_str(), std::ios::binary);
  if (!out_file.is_open())
  {
    return false;
  }

  ryml::Tree doc;
  bool ok = true;

  auto root = doc.rootref();
  root.append_child() << ryml::key("camera") << "";
  ok = save(root["camera"], config.camera) && ok;
  ok = save(root["log"], config.log) && ok;
  ok = save(root["playback"], config.playback) && ok;
  ok = save(root["render"], config.render) && ok;

  out_file << doc;
  out_file.flush();
  out_file.close();

  return ok;
}
}  // namespace tes::view::settings
