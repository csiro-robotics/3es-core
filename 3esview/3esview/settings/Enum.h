//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SETTINGS_ENUM_H
#define TES_VIEW_SETTINGS_ENUM_H

#include <3esview/ViewConfig.h>

#include <initializer_list>
#include <unordered_map>
#include <string>
#include <vector>

namespace tes::view::settings
{
/// An enum value for use in settings.
template <typename E>
class Enum
{
public:
  Enum(const std::string &label, E value, const std::string &tip,
       std::initializer_list<std::pair<E, std::string>> named_values)
    : _label(label)
    , _value(value)
    , _named_values(named_values)
    , _tip(tip)
  {}

  Enum(const Enum<E> &other) = default;
  Enum(Enum<E> &&other) = default;

  Enum<E> &operator=(const Enum<E> &other) = default;
  Enum<E> &operator=(Enum<E> &&other) = default;

  [[nodiscard]] const std::string &label() const { return _label; }
  [[nodiscard]] const std::string &tip() const { return _tip; }

  [[nodiscard]] const E &value() const { return _value; }
  void setValue(E value) { _value = value; }

  bool setValueByName(const std::string &name)
  {
    for (const auto &[e, str] : _named_values)
    {
      if (name == str)
      {
        _value = e;
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] std::string valueName() const { return enumName(_value); }

  [[nodiscard]] std::string enumName(E value) const
  {
    for (const auto &[e, str] : _named_values)
    {
      if (value == e)
      {
        return str;
      }
    }
    return {};
  }

  [[nodiscard]] const std::vector<std::pair<E, std::string>> &namedValues() const
  {
    return _named_values;
  }

private:
  E _value = {};
  std::vector<std::pair<E, std::string>> _named_values;
  std::string _label;
  std::string _tip;
};
}  // namespace tes::view::settings

#endif  // TES_VIEW_SETTINGS_ENUM_H
