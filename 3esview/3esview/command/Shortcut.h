//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_COMMAND_SHORTCUT_H
#define TES_VIEW_COMMAND_SHORTCUT_H

#include <3esview/ViewConfig.h>

#include <Magnum/Platform/GlfwApplication.h>

namespace tes::view::command
{
/// Defines a keyboard shortcut sequence.
///
/// A @c Shortcut may be constructed or parsed from a sequence string of the form:
///
/// @code{.unparsed}
///   [<modifier1> + ] [<modifier2> +] ... <key>
/// @endcode
///
/// The modifier and key names are case insensitive. For example; "ctrl+c"
///
/// Valid modifiers are:
///
/// - shift
/// - ctrl
/// - alt
/// - meta
///
/// When multiple modifiers are used, then order is irrelevant on parsing, but the resulting @c toString() operation
/// uses the order listed above. Duplicate modifier specifications are redundant and will be stripped.
///
/// Valid keys are:
///
/// - leftshift
/// - rightshift
/// - leftctrl
/// - rightctrl
/// - leftalt
/// - rightalt
/// - leftmeta
/// - rightmeta
/// - enter
/// - up
/// - down
/// - left
/// - right
/// - home
/// - end
/// - pageup
/// - pagedown
/// - backspace
/// - insert
/// - delete
/// - f1
/// - f2
/// - f3
/// - f4
/// - f5
/// - f6
/// - f7
/// - f8
/// - f9
/// - f10
/// - f11
/// - f12
/// - space : a ' ' character cannot be used since whitespace is removed on parsing.
/// - tab : a '\t' character cannot be used since whitespace is removed on parsing.
/// - quote or "'"
/// - comma or ","
/// - period or "."
/// - minus or "-"
/// - plus : a "+" character cannot be used since this is used to join modifier sequences.
/// - slash or "/"
/// - percent or "%"
/// - semicolon or ";"
/// - equal or "="
/// - leftbracket or "["
/// - rightbracket or "]"
/// - backslash "\\"
/// - backquote or "`"
/// - zero or "0"
/// - one or "1"
/// - two or "2"
/// - three or "3"
/// - four or "4"
/// - five or "5"
/// - six or "6"
/// - seven or "7"
/// - eight or "8"
/// - nine or "9"
/// - a
/// - b
/// - c
/// - d
/// - e
/// - f
/// - g
/// - h
/// - i
/// - j
/// - k
/// - l
/// - m
/// - n
/// - o
/// - p
/// - q
/// - r
/// - s
/// - t
/// - u
/// - v
/// - w
/// - x
/// - y
/// - z
/// - capslock
/// - scrolllock
/// - numlock
/// - printscreen
/// - pause
/// - menu
/// - numzero
/// - numone
/// - numtwo
/// - numthree
/// - numfour
/// - numfive
/// - numsix
/// - numseven
/// - numeight
/// - numnine
/// - numdecimal
/// - numdivide
/// - nummultiply
/// - numsubtract
/// - numadd
/// - numenter
/// - numequal
class TES_VIEWER_API Shortcut
{
public:
  /// Key type.
  using Key = Magnum::Platform::Application::KeyEvent::Key;
  /// Modifier key type.
  using Modifier = Magnum::Platform::Application::InputEvent::Modifier;

  /// Construct an invalid shortcut.
  Shortcut() = default;

  /// Construct with the given key and modifier flags.
  /// @param key Primary key stroke.
  /// @param modifiers Modifier key flags. See @c modifierFlag().
  Shortcut(Key key, unsigned modifiers = 0u)
    : _key(key)
    , _modifiers(modifiers)
  {}

  /// Construct with the given key and modifier
  /// @param key Primary key stroke.
  /// @param modifiers List of modifiers.
  Shortcut(Key key, std::initializer_list<Modifier> modifiers)
    : _key(key)
  {
    for (const auto mod : modifiers)
    {
      _modifiers |= modifierFlag(mod);
    }
  }

  /// Construct from string.
  ///
  /// May result in an invalid @c Shortcut. See @c isValid().
  ///
  /// See class comments on parsing rules.
  ///
  /// @param sequence The key sequence.
  Shortcut(const std::string &sequence) { *this = Shortcut::parse(sequence); }

  /// Copy constructor.
  /// @param other Object to copy.
  Shortcut(const Shortcut &other) = default;

  /// Copy assignment.
  /// @param other Object to copy.
  /// @return `*this`
  Shortcut &operator=(const Shortcut &other) = default;

  /// Equality operator.
  /// @param other Object to compare to.
  /// @return True if the objects are semantically equivalent.
  bool operator==(const Shortcut &other) const { return _key == other._key && _modifiers == other._modifiers; }

  /// Equality operator.
  /// @param other Object to compare to.
  /// @return True if the objects are not semantically equivalent.
  bool operator!=(const Shortcut &other) const { return !operator==(other); }

  /// Checks if this is a valid key sequence.
  /// @return True when valid.
  inline bool isValid() const { return _key != Key::Unknown; }

  /// Get the primary key.
  /// @return The shortcut key.
  Key key() const { return _key; }
  /// Get @c Modifier flags.
  /// @return Modifier flags.
  unsigned modifierFlags() const { return _modifiers; }

  /// Check if this shortcut includes modifiers.
  /// @return True if there are modifiers for the sequence.
  bool hasModifiers() const { return _modifiers != 0; }
  /// Check if this shortcut uses the specified @p modifier.
  /// @param modifier The modifier to check for.
  /// @return True if @p modifier is required for this sequence.
  bool hasModifier(Modifier modifier) const { return (_modifiers & modifierFlag(modifier)) != 0; }

  /// Get the bit flag for a @c Modifier.
  /// @param modifier The modifier to get a flag for.
  /// @return The bit flag corresponding to @p modifier.
  static unsigned modifierFlag(Modifier modifier) { return unsigned(modifier); }

  /// Try parse a shortcut @p sequence string.
  ///
  /// May result in an invalid @c Shortcut. See @c isValid().
  ///
  /// See class comments on parsing rules.
  ///
  /// @param sequence The key sequence.
  /// @return The parsed sequence, which is not valid on failure.
  static Shortcut parse(const std::string &sequence);

  /// Convert this @c Shortcut to a string which can be used with @c parse().
  /// @return The shortcut string.
  std::string toString() const;

private:
  Key _key = Key::Unknown;
  unsigned _modifiers = 0;
};
}  // namespace tes::view::command

#endif  // TES_VIEW_COMMAND_SHORTCUT_H
