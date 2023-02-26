//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SETTINGS_LOADER_H
#define TES_VIEW_SETTINGS_LOADER_h

#include <3esview/ViewConfig.h>

#include "Settings.h"

#include <filesystem>

namespace tes::view::settings
{
bool load(Settings::Config &config);
bool load(Settings::Config &config, const std::filesystem::path &path);
bool save(const Settings::Config &config);
bool save(const Settings::Config &config, const std::filesystem::path &path);
};  // namespace tes::view::settings

#endif  // TES_VIEW_SETTINGS_LOADER_h
