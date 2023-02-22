//
// Author: Kazys Stepanas
//
#include "Shortcut.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>
#include <unordered_map>

namespace tes::view::command
{
namespace
{
using KeyMap = std::unordered_map<std::string, Shortcut::Key>;
using ModifierMap = std::unordered_map<std::string, Shortcut::Modifier>;

const KeyMap &keyMap()
{
  static const KeyMap map = { { "leftshift", Shortcut::Key::LeftShift },
                              { "rightshift", Shortcut::Key::RightShift },
                              { "leftctrl", Shortcut::Key::LeftCtrl },
                              { "rightctrl", Shortcut::Key::RightCtrl },
                              { "leftalt", Shortcut::Key::LeftAlt },
                              { "rightalt", Shortcut::Key::RightAlt },
                              { "leftmeta", Shortcut::Key::LeftSuper },
                              { "rightmeta", Shortcut::Key::RightSuper },
                              { "enter", Shortcut::Key::Enter },
                              { "up", Shortcut::Key::Up },
                              { "down", Shortcut::Key::Down },
                              { "left", Shortcut::Key::Left },
                              { "right", Shortcut::Key::Right },
                              { "home", Shortcut::Key::Home },
                              { "end", Shortcut::Key::End },
                              { "pageup", Shortcut::Key::PageUp },
                              { "pagedown", Shortcut::Key::PageDown },
                              { "backspace", Shortcut::Key::Backspace },
                              { "insert", Shortcut::Key::Insert },
                              { "delete", Shortcut::Key::Delete },
                              { "f1", Shortcut::Key::F1 },
                              { "f2", Shortcut::Key::F2 },
                              { "f3", Shortcut::Key::F3 },
                              { "f4", Shortcut::Key::F4 },
                              { "f5", Shortcut::Key::F5 },
                              { "f6", Shortcut::Key::F6 },
                              { "f7", Shortcut::Key::F7 },
                              { "f8", Shortcut::Key::F8 },
                              { "f9", Shortcut::Key::F9 },
                              { "f10", Shortcut::Key::F10 },
                              { "f11", Shortcut::Key::F11 },
                              { "f12", Shortcut::Key::F12 },
                              { "space", Shortcut::Key::Space },
                              { "tab", Shortcut::Key::Tab },
                              { "quote", Shortcut::Key::Quote },
                              { "'", Shortcut::Key::Quote },
                              { "comma", Shortcut::Key::Comma },
                              { ",", Shortcut::Key::Comma },
                              { "period", Shortcut::Key::Period },
                              { ".", Shortcut::Key::Period },
                              { "minus", Shortcut::Key::Minus },
                              { "-", Shortcut::Key::Minus },
                              { "plus", Shortcut::Key::Plus },
                              { "slash", Shortcut::Key::Slash },
                              { "/", Shortcut::Key::Slash },
                              { "percent", Shortcut::Key::Percent },
                              { "%", Shortcut::Key::Percent },
                              { "semicolon", Shortcut::Key::Semicolon },
                              { ";", Shortcut::Key::Semicolon },
                              { "equal", Shortcut::Key::Equal },
                              { "=", Shortcut::Key::Equal },
                              { "leftbracket", Shortcut::Key::LeftBracket },
                              { "[", Shortcut::Key::LeftBracket },
                              { "rightbracket", Shortcut::Key::RightBracket },
                              { "]", Shortcut::Key::RightBracket },
                              { "backslash", Shortcut::Key::Backslash },
                              { "\\", Shortcut::Key::Backslash },
                              { "backquote", Shortcut::Key::Backquote },
                              { "`", Shortcut::Key::Backquote },
                              // { "world1", Shortcut::Key::World1 },
                              // { "world2", Shortcut::Key::World2 },
                              { "zero", Shortcut::Key::Zero },
                              { "0", Shortcut::Key::Zero },
                              { "one", Shortcut::Key::One },
                              { "1", Shortcut::Key::One },
                              { "two", Shortcut::Key::Two },
                              { "2", Shortcut::Key::One },
                              { "three", Shortcut::Key::Three },
                              { "3", Shortcut::Key::One },
                              { "four", Shortcut::Key::Four },
                              { "4", Shortcut::Key::One },
                              { "five", Shortcut::Key::Five },
                              { "5", Shortcut::Key::One },
                              { "six", Shortcut::Key::Six },
                              { "6", Shortcut::Key::One },
                              { "seven", Shortcut::Key::Seven },
                              { "7", Shortcut::Key::One },
                              { "eight", Shortcut::Key::Eight },
                              { "8", Shortcut::Key::One },
                              { "nine", Shortcut::Key::Nine },
                              { "9", Shortcut::Key::One },
                              { "a", Shortcut::Key::A },
                              { "b", Shortcut::Key::B },
                              { "c", Shortcut::Key::C },
                              { "d", Shortcut::Key::D },
                              { "e", Shortcut::Key::E },
                              { "f", Shortcut::Key::F },
                              { "g", Shortcut::Key::G },
                              { "h", Shortcut::Key::H },
                              { "i", Shortcut::Key::I },
                              { "j", Shortcut::Key::J },
                              { "k", Shortcut::Key::K },
                              { "l", Shortcut::Key::L },
                              { "m", Shortcut::Key::M },
                              { "n", Shortcut::Key::N },
                              { "o", Shortcut::Key::O },
                              { "p", Shortcut::Key::P },
                              { "q", Shortcut::Key::Q },
                              { "r", Shortcut::Key::R },
                              { "s", Shortcut::Key::S },
                              { "t", Shortcut::Key::T },
                              { "u", Shortcut::Key::U },
                              { "v", Shortcut::Key::V },
                              { "w", Shortcut::Key::W },
                              { "x", Shortcut::Key::X },
                              { "y", Shortcut::Key::Y },
                              { "z", Shortcut::Key::Z },
                              { "capslock", Shortcut::Key::CapsLock },
                              { "scrolllock", Shortcut::Key::ScrollLock },
                              { "numlock", Shortcut::Key::NumLock },
                              { "printscreen", Shortcut::Key::PrintScreen },
                              { "pause", Shortcut::Key::Pause },
                              { "menu", Shortcut::Key::Menu },
                              { "numzero", Shortcut::Key::NumZero },
                              { "numone", Shortcut::Key::NumOne },
                              { "numtwo", Shortcut::Key::NumTwo },
                              { "numthree", Shortcut::Key::NumThree },
                              { "numfour", Shortcut::Key::NumFour },
                              { "numfive", Shortcut::Key::NumFive },
                              { "numsix", Shortcut::Key::NumSix },
                              { "numseven", Shortcut::Key::NumSeven },
                              { "numeight", Shortcut::Key::NumEight },
                              { "numnine", Shortcut::Key::NumNine },
                              { "numdecimal", Shortcut::Key::NumDecimal },
                              { "numdivide", Shortcut::Key::NumDivide },
                              { "nummultiply", Shortcut::Key::NumMultiply },
                              { "numsubtract", Shortcut::Key::NumSubtract },
                              { "numadd", Shortcut::Key::NumAdd },
                              { "numenter", Shortcut::Key::NumEnter },
                              { "numequal", Shortcut::Key::NumEqual } };
  return map;
}

const ModifierMap &modifierMap()
{
  static const ModifierMap map = { { "shift", Shortcut::Modifier::Shift },
                                   { "ctrl", Shortcut::Modifier::Ctrl },
                                   { "alt", Shortcut::Modifier::Alt },
                                   { "meta", Shortcut::Modifier::Super } };

  return map;
}
}  // namespace

Shortcut Shortcut::parse(const std::string &sequence)
{
  // First split the sequence by '+' characters.
  std::istringstream stream(sequence);

  std::vector<std::string> tokens;
  std::string token;
  while (std::getline(stream, token, '+'))
  {
    // Strip whitespace.
    token.erase(std::remove_if(token.begin(), token.end(),
                               [](const std::string::value_type &ch) { return std::isspace(ch); }),
                token.end());
    // Convert to lower case.
    std::transform(token.begin(), token.end(), token.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    // Add the token
    tokens.emplace_back(token);
  }

  if (tokens.empty())
  {
    return Shortcut();
  }

  const auto &key_map = keyMap();
  const auto &mod_map = modifierMap();

  // Set key to the last item.
  const auto key_lookup = key_map.find(tokens.back());
  if (key_lookup == key_map.end())
  {
    return Shortcut();
  }

  // Erase the key from the tokens list.
  tokens.erase(tokens.begin() + tokens.size() - 1);

  // Build the modifiers.
  unsigned modifiers = 0u;
  for (const auto &mod_str : tokens)
  {
    const auto mod_lookup = mod_map.find(mod_str);
    if (mod_lookup == mod_map.end())
    {
      return Shortcut();
    }

    modifiers |= Shortcut::modifierFlag(mod_lookup->second);
  }

  return Shortcut(key_lookup->second, modifiers);
}


std::string Shortcut::toString() const
{
  if (!isValid())
  {
    return {};
  }

  const auto &key_map = keyMap();
  const auto &mod_map = modifierMap();

  // Convert modifiers.
  std::ostringstream str;
  std::string append = "";
  const char joiner = '+';

  for (const auto &[name, mod] : mod_map)
  {
    if (_modifiers & modifierFlag(mod))
    {
      str << append << name;
      append = joiner;
    }
  }

  for (const auto &[name, key] : key_map)
  {
    if (key == _key)
    {
      str << append << name;
      append = joiner;
      break;
    }
  }

  str.flush();
  return str.str();
}
}  // namespace tes::view::command
