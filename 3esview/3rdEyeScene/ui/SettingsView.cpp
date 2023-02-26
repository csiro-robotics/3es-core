//
// Author: Kazys Stepanas
//
#include "SettingsView.h"

#include <3escore/CoreUtil.h>
#include <3escore/Log.h>

#include <3esview/Viewer.h>

#include <Magnum/ImGuiIntegration/Context.hpp>

#include <array>

namespace tes::view::ui
{
SettingsView::SettingsView(Viewer &viewer)
  : Panel(viewer)
{}


void SettingsView::draw(Magnum::ImGuiIntegration::Context &ui)
{
  TES_UNUSED(ui);
  if (!ImGui::Begin("Settings"))
  {
    ImGui::End();
    return;
  }

  auto config = _viewer.tes()->settings().config();
  if (ImGui::BeginTable("SettingsSplit", 2,
                        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable))
  {
    unsigned idx = 0;

    const bool camera_dirty = show(idx++, config.camera);
    const bool render_dirty = show(idx++, config.render);
    const bool playback_dirty = show(idx++, config.playback);
    // const bool log_dirty = show(idx++,config.log);

    // If more than two things are dirty, then update everything. Really should never happen as we
    // can only modify one property at a time.
    unsigned dirty_count = 0;
    dirty_count += !!camera_dirty;
    dirty_count += !!render_dirty;
    dirty_count += !!playback_dirty;
    // dirty_count += !!log_dirty;
    if (dirty_count == 1)
    {
      if (camera_dirty)
      {
        _viewer.tes()->settings().update(config.camera);
      }
      else if (render_dirty)
      {
        _viewer.tes()->settings().update(config.render);
      }
      else if (playback_dirty)
      {
        _viewer.tes()->settings().update(config.playback);
      }
      // else if (log_dirty)
      // {
      //   _viewer.tes()->settings().update(config.log);
      // }
    }
    else if (dirty_count > 1)
    {
      _viewer.tes()->settings().update(config);
    }

    ImGui::EndTable();
  }

  ImGui::End();
}


bool SettingsView::show(unsigned idx, settings::Camera &config)
{
  const bool open = beginSection(idx, "Camera");
  bool dirty = false;

  if (open)
  {
    unsigned idx = 0;
    dirty = showProperty(idx++, config.invert_y) || dirty;
    dirty = showProperty(idx++, config.allow_remote_settings) || dirty;
    dirty = showProperty(idx++, config.near_clip) || dirty;
    dirty = showProperty(idx++, config.far_clip) || dirty;
    dirty = showProperty(idx++, config.fov) || dirty;
  }

  endSection(open);

  return dirty;
}


bool SettingsView::show(unsigned idx, settings::Log &config)
{
  const bool open = beginSection(idx, "Log");
  bool dirty = false;

  if (open)
  {
    unsigned idx = 0;
    dirty = showProperty(idx++, config.log_window_size) || dirty;
  }

  endSection(open);

  return dirty;
}


bool SettingsView::show(unsigned idx, settings::Playback &config)
{
  const bool open = beginSection(idx, "Playback");
  bool dirty = false;

  if (open)
  {
    unsigned idx = 0;
    dirty = showProperty(idx++, config.allow_key_frames) || dirty;
    dirty = showProperty(idx++, config.keyframe_every_mib) || dirty;
    dirty = showProperty(idx++, config.keyframe_every_frames) || dirty;
    dirty = showProperty(idx++, config.keyframe_skip_forward_frames) || dirty;
    dirty = showProperty(idx++, config.keyframe_compression) || dirty;
    dirty = showProperty(idx++, config.looping) || dirty;
    dirty = showProperty(idx++, config.pause_on_error) || dirty;
  }

  endSection(open);

  return dirty;
}


bool SettingsView::show(unsigned idx, settings::Render &config)
{
  const bool open = beginSection(idx, "Render");
  bool dirty = false;

  if (open)
  {
    unsigned idx = 0;
    dirty = showProperty(idx++, config.use_edl_shader) || dirty;
    dirty = showProperty(idx++, config.edl_radius) || dirty;
    dirty = showProperty(idx++, config.edl_exponential_scale) || dirty;
    dirty = showProperty(idx++, config.edl_linear_scale) || dirty;
    dirty = showProperty(idx++, config.point_size) || dirty;
    dirty = showProperty(idx++, config.background_colour) || dirty;
  }

  endSection(open);

  return dirty;
}


bool SettingsView::showProperty(unsigned idx, settings::Bool &prop)
{
  beginProperty(idx, prop.label(), prop.tip());
  bool value = prop.value();
  const bool dirty = ImGui::Checkbox(prop.label().c_str(), &value);
  if (dirty)
  {
    prop.setValue(value);
  }
  endProperty();
  return dirty;
}


bool SettingsView::showProperty(unsigned idx, settings::Int &prop)
{
  beginProperty(idx, prop.label(), prop.tip());
  int value = prop.value();
  const bool dirty = ImGui::InputInt(prop.label().c_str(), &value);
  if (dirty)
  {
    prop.setValue(value);
  }
  endProperty();
  return dirty;
}


bool SettingsView::showProperty(unsigned idx, settings::UInt &prop)
{
  beginProperty(idx, prop.label(), prop.tip());
  unsigned value = prop.value();
  const bool dirty = ImGui::InputInt(prop.label().c_str(), reinterpret_cast<int *>(&value));
  if (dirty)
  {
    prop.setValue(value);
  }
  endProperty();
  return dirty;
}


bool SettingsView::showProperty(unsigned idx, settings::Float &prop)
{
  beginProperty(idx, prop.label(), prop.tip());
  float value = prop.value();
  const bool dirty = ImGui::InputFloat(prop.label().c_str(), &value);
  if (dirty)
  {
    prop.setValue(value);
  }
  endProperty();
  return dirty;
}


bool SettingsView::showProperty(unsigned idx, settings::Double &prop)
{
  beginProperty(idx, prop.label(), prop.tip());
  double value = prop.value();
  const bool dirty = ImGui::InputDouble(prop.label().c_str(), &value);
  if (dirty)
  {
    prop.setValue(value);
  }
  endProperty();
  return dirty;
}


bool SettingsView::showProperty(unsigned idx, settings::Colour &prop)
{
  beginProperty(idx, prop.label(), prop.tip());
  auto value = prop.value();
  std::array<float, 3> rgb_f = { prop.value().rf(), prop.value().gf(), prop.value().bf() };
  const bool dirty = ImGui::ColorEdit3(prop.label().c_str(), rgb_f.data());
  if (dirty)
  {
    value.setRf(rgb_f[0]);
    value.setGf(rgb_f[1]);
    value.setBf(rgb_f[2]);
    prop.setValue(value);
  }
  endProperty();
  return dirty;
}


template <typename E>
bool SettingsView::showProperty(unsigned idx, settings::Enum<E> &prop)
{
  beginProperty(idx, prop.label(), prop.tip());
  bool dirty = false;
  auto value = prop.value();
  if (ImGui::BeginCombo(prop.label().c_str(), prop.valueName().c_str()))
  {
    const auto named_values = prop.namedValues();
    int value_index = 0;
    std::vector<const char *> names(named_values.size());
    for (int i = 0; i < static_cast<int>(named_values.size()); ++i)
    {
      names[i] = named_values[i].second.c_str();
      if (value == named_values[i])
      {
        value_index = i;
      }
    }
    if (Combo(name.c_str(), &value_index, names.c_str(), static_cast<int>(names.size())))
    {
      prop.setValueByName(names[value_index]);
      dirty = true;
    }
  }
  ImGui::EndCombo();
  endProperty();
  return dirty;
}


bool SettingsView::beginSection(unsigned idx, const std::string &label)
{
  // Use object uid as identifier. Most commonly you could also use the object pointer as a base ID.
  ImGui::PushID(idx);
  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::AlignTextToFramePadding();
  const bool node_open = ImGui::TreeNode(label.c_str());
  ImGui::TableSetColumnIndex(1);
  ImGui::Text(label.c_str());

  return node_open;
}


void SettingsView::endSection(bool open)
{
  if (open)
  {
    ImGui::TreePop();
  }
  ImGui::PopID();
}


void SettingsView::beginProperty(unsigned idx, const std::string &label, const std::string &info)
{
  TES_UNUSED(info);
  ImGui::PushID(idx);  // Use field index as identifier.
  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::AlignTextToFramePadding();
  const ImGuiTreeNodeFlags flags =
    ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet;
  ImGui::TreeNodeEx(label.c_str(), flags);
  if (ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::SetTooltip(info.c_str());
    ImGui::EndTooltip();
  }

  ImGui::TableSetColumnIndex(1);
  ImGui::SetNextItemWidth(-FLT_MIN);
}


void SettingsView::endProperty()
{
  ImGui::NextColumn();
  ImGui::PopID();
}
}  // namespace tes::view::ui
