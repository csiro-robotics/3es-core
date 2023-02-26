//
// Author: Kazys Stepanas
//
#include "Settings.h"

namespace tes::view::settings
{
Settings::Config Settings::config() const
{
  const std::lock_guard guard(_mutex);
  return _config;
}

void Settings::update(const Config &config)
{
  const std::lock_guard guard(_mutex);
  _config = config;
  notify(config);
}


void Settings::update(const Camera &config)
{
  const std::lock_guard guard(_mutex);
  _config.camera = config;
  const auto full_config = _config;
  notify(Category::Camera, full_config);
}


void Settings::update(const Log &config)
{
  const std::lock_guard guard(_mutex);
  _config.log = config;
  const auto full_config = _config;
  notify(Category::Log, full_config);
}


void Settings::update(const Playback &config)
{
  const std::lock_guard guard(_mutex);
  _config.playback = config;
  const auto full_config = _config;
  notify(Category::Playback, full_config);
}


void Settings::update(const Render &config)
{
  const std::lock_guard guard(_mutex);
  _config.render = config;
  const auto full_config = _config;
  notify(Category::Render, full_config);
}


void Settings::addObserver(NotifyCallback callback)
{
  const std::lock_guard guard(_observer_mutex);
  _observers.emplace_back(std::move(callback));
}


void Settings::addObserver(Category category, NotifyCallback callback)
{
  if (category == Category::Invalid)
  {
    return;
  }

  const std::lock_guard guard(_observer_mutex);
  _sub_observers[static_cast<unsigned>(category)].emplace_back(std::move(callback));
}


void Settings::notify(const Config &config)
{
  const std::lock_guard guard(_observer_mutex);
  // Global notify.
  for (const auto &observer : _observers)
  {
    observer(config);
  }

  // Sub observer notify - all as we can't be sure.
  for (const auto &sub_observers : _sub_observers)
  {
    for (const auto &observer : sub_observers)
    {
      observer(config);
    }
  }
}


void Settings::notify(Category category, const Config &config)
{
  if (category == Category::Invalid)
  {
    return;
  }

  const std::lock_guard guard(_observer_mutex);

  // Sub observer notify.
  const auto &sub_observers = _sub_observers[static_cast<unsigned>(category)];
  for (const auto &observer : sub_observers)
  {
    observer(config);
  }

  // Global notify.
  for (const auto &observer : _observers)
  {
    observer(config);
  }
}
}  // namespace tes::view::settings
