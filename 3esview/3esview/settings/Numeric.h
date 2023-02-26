//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SETTINGS_NUMERIC_H
#define TES_VIEW_SETTINGS_NUMERIC_H

#include <3esview/ViewConfig.h>

#include <limits>
#include <optional>
#include <string>

namespace tes::view::settings
{
/// A numeric value for use in settings.
template <typename T>
class Numeric
{
public:
  Numeric(const std::string &label, T value, const std::string &tip)
    : _label(label)
    , _value(value)
    , _tip(tip)
  {}
  Numeric(const std::string &label, T value, T minimum, const std::string &tip)
    : _label(label)
    , _value(value)
    , _minimum(minimum)
    , _tip(tip)
  {}
  Numeric(const std::string &label, T value, const std::string &tip, T maximum)
    : _label(label)
    , _value(value)
    , _maximum(maximum)
    , _tip(tip)
  {}
  Numeric(const std::string &label, T value, T minimum, T maximum, const std::string &tip)
    : _label(label)
    , _value(value)
    , _minimum(minimum)
    , _maximum(maximum)
    , _tip(tip)
  {}

  Numeric(const Numeric<T> &other) = default;
  Numeric(Numeric<T> &&other) = default;

  Numeric<T> &operator=(const Numeric<T> &other) = default;
  Numeric<T> &operator=(Numeric<T> &&other) = default;

  [[nodiscard]] const std::string &label() const { return _label; }
  [[nodiscard]] const std::string &tip() const { return _tip; }

  [[nodiscard]] const T &value() const { return _value; }
  void setValue(T value) { _value = std::max(minimum(), std::min(value, maximum())); }

  bool hasMinimum() const { return _minimum.has_value(); }
  T minimum() const { return (hasMinimum()) ? *_minimum : std::numeric_limits<T>::min(); }
  void setMinimum(T minimum) { _minimum = minimum; }

  bool hasMaximum() const { return _maximum.has_value(); }
  T maximum() const { return (hasMaximum()) ? *_maximum : std::numeric_limits<T>::max(); }
  void setMaximum(T maximum) { _maximum = maximum; }

private:
  T _value = {};
  std::optional<T> _minimum;
  std::optional<T> _maximum;
  std::string _label;
  std::string _tip;
};

using Int = Numeric<int>;
using UInt = Numeric<unsigned>;
using Float = Numeric<float>;
using Double = Numeric<double>;

class Bool
{
public:
  Bool(const std::string &label, bool value, const std::string &tip)
    : _label(label)
    , _value(value)
    , _tip(tip)
  {}

  Bool(const Bool &other) = default;
  Bool(Bool &&other) = default;

  Bool &operator=(const Bool &other) = default;
  Bool &operator=(Bool &&other) = default;

  [[nodiscard]] const std::string &label() const { return _label; }
  [[nodiscard]] const std::string &tip() const { return _tip; }

  [[nodiscard]] bool value() const { return _value; }
  void setValue(bool value) { _value = value; }

private:
  bool _value = false;
  std::string _label;
  std::string _tip;
};
}  // namespace tes::view::settings

#endif  // TES_VIEW_SETTINGS_NUMERIC_H
