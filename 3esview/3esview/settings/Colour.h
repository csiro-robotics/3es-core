//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SETTINGS_COLOUR_H
#define TES_VIEW_SETTINGS_COLOUR_H

#include <3esview/ViewConfig.h>

#include <3escore/Colour.h>
#include <string>

namespace tes::view::settings
{
/// An enum value for use in settings.
class Colour
{
public:
  using ValueType = tes::Colour;

  Colour(const std::string &label, ValueType value, const std::string &tip)
    : _label(label)
    , _value(value)
    , _tip(tip)
  {}

  Colour(const Colour &other) = default;
  Colour(Colour &&other) = default;

  Colour &operator=(const Colour &other) = default;
  Colour &operator=(Colour &&other) = default;

  [[nodiscard]] const std::string &label() const { return _label; }
  [[nodiscard]] const std::string &tip() const { return _tip; }

  [[nodiscard]] const ValueType &value() const { return _value; }
  void setValue(ValueType value) { _value = value; }

private:
  ValueType _value = {};
  std::string _label;
  std::string _tip;
};
}  // namespace tes::view::settings

#endif  // TES_VIEW_SETTINGS_COLOUR_H
