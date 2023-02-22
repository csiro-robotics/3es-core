//
// author Kazys Stepanas
//
// Copyright (c) Kazys Stepanas 2014
//
#ifndef TES_CORE_COLOUR_H
#define TES_CORE_COLOUR_H

#include "CoreConfig.h"

#include "Maths.h"

#include <array>
#include <vector>

namespace tes
{
/// A 32-bit integer colour class.
///
/// Storage is designed to allow colours to be written as unsigned
/// hexadecimal integers as 0xRRGGBBAA regardless of the target Endian.
class TES_CORE_API Colour
{
public:
  /// Channel index enumeration.
  ///
  /// Note the channel ordering depends on the machine endian. This supports implicit conversion to
  /// and from @c uint32_t colour such as when used in the @c DataBuffer .
  enum class Channel : int
  {
#if TES_IS_BIG_ENDIAN
    R = 0,  ///< Red channel index.
    G = 1,  ///< Green channel index.
    B = 2,  ///< Blue channel index.
    A = 3   ///< Alpha channel index.
#else       // TES_IS_BIG_ENDIAN
    A = 0,  ///< Alpha channel index.
    B = 1,  ///< Blue channel index.
    G = 2,  ///< Green channel index.
    R = 3   ///< Red channel index.
#endif      // TES_IS_BIG_ENDIAN
  };

  enum NamedColour : unsigned;

  /// Construct a colour with the given numeric value.
  /// @param colour_value The integer colour representation: 0xRRGGBBAA.
  Colour(uint32_t colour_value = 0xffffffffu) noexcept;  // NOLINT(readability-magic-numbers)

  /// Construct a colour the named colours.
  /// @param name The colour name enumeration value.
  Colour(NamedColour name);

  /// Copy constructor.
  /// @param other The colour to copy.
  Colour(const Colour &other) noexcept;

  /// Partial copy constructor with new alpha value.
  /// @param other The colour to copy RGB channels from.
  /// @param alpha The new alpha channel value.
  Colour(const Colour &other, uint8_t alpha) noexcept;
  /// Partial copy constructor with new alpha value.
  /// @param other The colour to copy RGB channels from.
  /// @param alpha The new alpha channel value.
  Colour(const Colour &other, int alpha) noexcept;
  /// Partial copy constructor with new alpha value.
  /// @param other The colour to copy RGB channels from.
  /// @param alpha The new alpha channel value.
  Colour(const Colour &other, float alpha) noexcept;

  /// Explicit byte based RGBA colour channel initialisation constructor.
  /// @param red Red channel value [0, 255].
  /// @param green Green channel value [0, 255].
  /// @param blue Blue channel value [0, 255].
  /// @param alpha Alpha channel value [0, 255].
  explicit Colour(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255u) noexcept;

  /// Intantiate from a @c uint8_t array.
  /// @param array Data array to intialise for with indexing set by @c Colour::Channel .
  Colour(const std::array<uint8_t, 4> &array) noexcept;

  /// Integer based RGBA colour channel initialisation constructor.
  /// @param red Red channel value [0, 255].
  /// @param green Green channel value [0, 255].
  /// @param blue Blue channel value [0, 255].
  /// @param alpha Alpha channel value [0, 255].
  Colour(int red, int green, int blue, int alpha = 255) noexcept;

  /// Floating point RGBA colour channel initialisation constructor.
  /// @param red Red channel value [0, 1].
  /// @param green Green channel value [0, 1].
  /// @param blue Blue channel value [0, 1].
  /// @param alpha Alpha channel value [0, 1].
  Colour(float red, float green, float blue, float alpha = 1.0f) noexcept;

  /// Access the specified colour channel for read/write.
  /// @param channel The channel to access.
  /// @return A reference to the colour channel value.
  uint8_t &channel(Channel channel) { return _storage[static_cast<int>(channel)]; }
  /// Access the specified colour channel for read-only.
  /// @param channel The channel to access.
  /// @return The colour channel value.
  [[nodiscard]] uint8_t channel(Channel channel) const
  {
    return _storage[static_cast<int>(channel)];
  }

  /// Return the internal data storage. Used for buffer packing and network transfer.
  /// @return The internal array.
  [[nodiscard]] const std::array<uint8_t, 4> &storage() const { return _storage; }

  /// Access the red colour channel for read/write.
  /// @return A reference to the red colour channel.
  uint8_t &r() { return red(); }
  /// Access the red colour channel for read-only.
  /// @return The red colour channel value.
  [[nodiscard]] uint8_t r() const { return red(); }
  /// Access the red colour channel for read/write.
  /// @return A reference to the red colour channel.
  uint8_t &red() { return channel(Channel::R); }
  /// Access the red colour channel for read-only.
  /// @return The red colour channel value.
  [[nodiscard]] uint8_t red() const { return channel(Channel::R); }

  /// Access the green colour channel for read/write.
  /// @return A reference to the green colour channel.
  uint8_t &g() { return green(); }
  /// Access the green colour channel for read-only.
  /// @return The green colour channel value.
  [[nodiscard]] uint8_t g() const { return green(); }
  /// Access the green colour channel for read/write.
  /// @return A reference to the green colour channel.
  uint8_t &green() { return channel(Channel::G); }
  /// Access the green colour channel for read-only.
  /// @return The green colour channel value.
  [[nodiscard]] uint8_t green() const { return channel(Channel::G); }

  /// Access the blue colour channel for read/write.
  /// @return A reference to the blue colour channel.
  uint8_t &b() { return blue(); }
  /// Access the blue colour channel for read-only.
  /// @return The blue colour channel value.
  [[nodiscard]] uint8_t b() const { return blue(); }
  /// Access the blue colour channel for read/write.
  /// @return A reference to the blue colour channel.
  uint8_t &blue() { return channel(Channel::B); }
  /// Access the blue colour channel for read-only.
  /// @return The blue colour channel value.
  [[nodiscard]] uint8_t blue() const { return channel(Channel::B); }

  /// Access the alpha colour channel for read/write.
  /// @return A reference to the alpha colour channel.
  uint8_t &a() { return alpha(); }
  /// Access the alpha colour channel for read-only.
  /// @return The alpha colour channel value.
  [[nodiscard]] uint8_t a() const { return alpha(); }
  /// Access the alpha colour channel for read/write.
  /// @return A reference to the alpha colour channel.
  uint8_t &alpha() { return channel(Channel::A); }
  /// Access the alpha colour channel for read-only.
  /// @return The alpha colour channel value.
  [[nodiscard]] uint8_t alpha() const { return channel(Channel::A); }

  /// Return a 32-bit integer representation of the colour.
  ///
  /// This packs the 4 1-byte colour channels into a single 32-bit integer. The byte is platform
  /// endian dependent and corresponds to how the values of the @c Channel enumeration change
  /// depending on platform endian.
  ///
  /// @return A 32-bit integer representation of the colour.
  [[nodiscard]] uint32_t colour32() const;
  /// Cast to a @c uint32_t - see @c colour32().
  /// @return A 32-bit integer representation of the colour.
  operator uint32_t() const { return colour32(); }

  /// Get red channel in floating point form.
  /// @return Red channel [0, 1].
  [[nodiscard]] float rf() const;
  /// Get green channel in floating point form.
  /// @return Green channel [0, 1].
  [[nodiscard]] float gf() const;
  /// Get blue channel in floating point form.
  /// @return Blue channel [0, 1].
  [[nodiscard]] float bf() const;
  /// Get alpha channel in floating point form.
  /// @return Alpha channel [0, 1].
  [[nodiscard]] float af() const;

  /// Set red channel from a floating point value.
  /// @param value Channel value [0, 1].
  void setRf(float value);
  /// Set green channel from a floating point value.
  /// @param value Channel value [0, 1].
  void setGf(float value);
  /// Set blue channel from a floating point value.
  /// @param value Channel value [0, 1].
  void setBf(float value);
  /// Set alpha channel from a floating point value.
  /// @param value Channel value [0, 1].
  void setAf(float value);

  /// Set a channel in floating point form.
  /// @param value Channel value [0, 1].
  /// @param channel The target channel.
  void setf(float value, Channel channel);
  /// Get a channel in floating point form.
  /// @param channel The target channel.
  /// @return The channel value [0, 1].
  [[nodiscard]] float getf(Channel channel) const;

  /// Lighten or darken a colour by @p factor.
  /// Works in HSV space, multiplying the V value by @p factor and clamping the result [0, 1].
  /// @return The adjusted colour.
  [[nodiscard]] Colour adjust(float factor) const;

  /// Lighten the colour by 1.5
  /// @return A lighter colour.
  [[nodiscard]] Colour lighten() const { return adjust(1.5f); }

  /// Darken the colour by 0.5
  /// @return A darker colour.
  [[nodiscard]] Colour darken() const { return adjust(0.5f); }

  /// Assignment operator.
  /// @param other The colour value to assign.
  /// @return @c this.
  Colour &operator=(const Colour &other);

  // inline operator uint32_t() const { return c; }

  /// Precise equality operator.
  /// @param other The colour to compare to.
  /// @return True if this colour is precisely equal to @p other.
  bool operator==(const Colour &other) const;
  /// Precise inequality operator.
  /// @param other The colour to compare to.
  /// @return True if this colour is not precisely equal to @p other.
  bool operator!=(const Colour &other) const;

  [[nodiscard]] static Colour lerp(const Colour &from, const Colour &to, float factor);

  /// Create a @c Colour object from HSV values.
  ///
  /// Out of range arguments yield undefined behaviour.
  ///
  /// @param hue The hue value [0, 360].
  /// @param saturation The saturation value [0, 1].
  /// @param value The colour value [0, 1].
  /// @param alpha Optional alpha channel value [0, 1].
  /// @return The corresponding colour object.
  [[nodiscard]] static Colour fromHsv(float hue, float saturation, float value, float alpha = 1.0f);

  /// Convert RGB to HSV form.
  /// @param[out] hue The hue value [0, 360].
  /// @param[out] saturation The saturation value [0, 1].
  /// @param[out] value The colour value [0, 1].
  /// @param red Red channel.
  /// @param green Green channel.
  /// @param blue Blue channel.
  static void rgbToHsv(float &hue, float &saturation, float &value, float red, float green,
                       float blue);

  /// Convert HSV to RGB form.
  /// @param[out] red Red channel [0, 1].
  /// @param[out] green Green channel [0, 1].
  /// @param[out] blue Blue channel [0, 1].
  /// @param hue The hue value [0, 360].
  /// @param saturation The saturation value [0, 1].
  /// @param value The colour value [0, 1].
  static void hsvToRgb(float &red, float &green, float &blue, float hue, float saturation,
                       float value);
  /// Convert HSV to RGB form.
  /// @param[out] red Red channel [0, 255].
  /// @param[out] green Green channel [0, 255].
  /// @param[out] blue Blue channel [0, 255].
  /// @param hue The hue value [0, 360].
  /// @param saturation The saturation value [0, 1].
  /// @param value The colour value [0, 1].
  static void hsvToRgb(uint8_t &red, uint8_t &green, uint8_t &blue, float hue, float saturation,
                       float value);

  /// Helper for converting between @c Colour and @c uint32_t .
  struct TES_CORE_API ConverterUInt32
  {
    static constexpr int kRedIndex = static_cast<int>(Channel::R);
    static constexpr int kGreenIndex = static_cast<int>(Channel::G);
    static constexpr int kBlueIndex = static_cast<int>(Channel::B);
    static constexpr int kAlphaIndex = static_cast<int>(Channel::A);
    static constexpr unsigned kRedShift =
      (static_cast<unsigned>(Channel::R) * static_cast<unsigned>(sizeof(uint8_t))) * 8u;
    static constexpr unsigned kGreenShift =
      (static_cast<unsigned>(Channel::G) * static_cast<unsigned>(sizeof(uint8_t))) * 8u;
    static constexpr unsigned kBlueShift =
      (static_cast<unsigned>(Channel::B) * static_cast<unsigned>(sizeof(uint8_t))) * 8u;
    static constexpr unsigned kAlphaShift =
      (static_cast<unsigned>(Channel::A) * static_cast<unsigned>(sizeof(uint8_t))) * 8u;

    uint32_t operator()(const std::array<uint8_t, 4> &storage) const;
    void operator()(uint32_t colour, std::array<uint8_t, 4> &storage) const;
  };

  /// Enumerates a set of predefined colours ("web safe" colours).
  enum NamedColour : unsigned
  {
    // Greys and blacks.
    Gainsboro,
    LightGrey,
    Silver,
    DarkGrey,
    Grey,
    DimGrey,
    LightSlateGrey,
    SlateGrey,
    DarkSlateGrey,
    Black,

    // Whites
    White,
    Snow,
    Honeydew,
    MintCream,
    Azure,
    AliceBlue,
    GhostWhite,
    WhiteSmoke,
    Seashell,
    Beige,
    OldLace,
    FloralWhite,
    Ivory,
    AntiqueWhite,
    Linen,
    LavenderBlush,
    MistyRose,

    // Pinks
    Pink,
    LightPink,
    HotPink,
    DeepPink,
    PaleVioletRed,
    MediumVioletRed,

    // Reds
    LightSalmon,
    Salmon,
    DarkSalmon,
    LightCoral,
    IndianRed,
    Crimson,
    FireBrick,
    DarkRed,
    Red,

    // Oranges
    OrangeRed,
    Tomato,
    Coral,
    DarkOrange,
    Orange,

    // Yellows
    Yellow,
    LightYellow,
    LemonChiffon,
    LightGoldenrodYellow,
    PapayaWhip,
    Moccasin,
    PeachPuff,
    PaleGoldenrod,
    Khaki,
    DarkKhaki,
    Gold,

    // Browns
    Cornsilk,
    BlanchedAlmond,
    Bisque,
    NavajoWhite,
    Wheat,
    BurlyWood,
    Tan,
    RosyBrown,
    SandyBrown,
    Goldenrod,
    DarkGoldenrod,
    Peru,
    Chocolate,
    SaddleBrown,
    Sienna,
    Brown,
    Maroon,

    // Greens
    DarkOliveGreen,
    Olive,
    OliveDrab,
    YellowGreen,
    LimeGreen,
    Lime,
    LawnGreen,
    Chartreuse,
    GreenYellow,
    SpringGreen,
    MediumSpringGreen,
    LightGreen,
    PaleGreen,
    DarkSeaGreen,
    MediumSeaGreen,
    SeaGreen,
    ForestGreen,
    Green,
    DarkGreen,

    // Cyans
    MediumAquamarine,
    Aqua,
    Cyan,
    LightCyan,
    PaleTurquoise,
    Aquamarine,
    Turquoise,
    MediumTurquoise,
    DarkTurquoise,
    LightSeaGreen,
    CadetBlue,
    DarkCyan,
    Teal,

    // Blues
    LightSteelBlue,
    PowderBlue,
    LightBlue,
    SkyBlue,
    LightSkyBlue,
    DeepSkyBlue,
    DodgerBlue,
    CornflowerBlue,
    SteelBlue,
    RoyalBlue,
    Blue,
    MediumBlue,
    DarkBlue,
    Navy,
    MidnightBlue,

    // Purples
    Lavender,
    Thistle,
    Plum,
    Violet,
    Orchid,
    Fuchsia,
    Magenta,
    MediumOrchid,
    MediumPurple,
    BlueViolet,
    DarkViolet,
    DarkOrchid,
    DarkMagenta,
    Purple,
    Indigo,
    DarkSlateBlue,
    SlateBlue,
    MediumSlateBlue,

    PredefinedLast = MediumSlateBlue
  };

private:
  std::array<uint8_t, 4> _storage = {};
};


/// Defines a predetermined set of colours which can be index in a cyclic manner.
///
/// Indexing a colour set is safe regardless of the given index. The index is put in range using
/// a modulus operator and an empty set always returns a black, zero alpha @c Colour .
class TES_CORE_API ColourSet
{
public:
  /// Enumerates the various available colour cycles.
  ///
  /// Note: the colours cycles include sets which attempt to cater for various
  /// forms of colour blindness. These are not rigorously constructed and may
  /// not be as well suited as they are intended. Feel free to offer suggested
  /// improvements to these colours sets.
  ///
  /// @see @c colourCycle()
  enum PredefinedSet : unsigned
  {
    /// Full web safe colour set.
    WebSafe,
    /// Standard colour set.
    Standard,
    /// A colour set which attempts to cater for Deuteranomaly colour blindness.
    Deuteranomaly,
    /// A colour set which attempts to cater for Protanomaly colour blindness.
    Protanomaly,
    /// A colour set which attempts to cater for Tritanomaly colour blindness.
    Tritanomaly,
    /// A small grey scale colour set.
    Grey,
    /// Defines the last colour index.
    LastSet = Grey
  };

  /// Default constructor generating an empty set.
  ColourSet();
  /// Construct from the given set of colours.
  /// @param colours The set of colours to initialise with.
  ColourSet(std::initializer_list<Colour> colours);
  /// Move constructor.
  /// @param other Object to move.
  ColourSet(ColourSet &&other) noexcept;
  /// Copy constructor.
  /// @param other Object to copy.
  ColourSet(const ColourSet &other);
  ~ColourSet();

  /// Move assignment.
  /// @param other Object to move.
  ColourSet &operator=(ColourSet &&other) noexcept;
  /// Copy assignment.
  /// @param other Object to copy.
  ColourSet &operator=(const ColourSet &other);

  /// Query the number of colours in the set.
  /// @return The number of colours.
  [[nodiscard]] size_t size() const { return _colours.size(); }
  /// Check if the set is empty.
  /// @return True when empty.
  [[nodiscard]] bool empty() const { return _colours.empty(); }

  /// Request a colour from the set.
  ///
  /// This returns at the index corresponding to @p number using a modulus operator to ensure
  /// @p number is in range.
  ///
  /// @param number The colour number.
  /// @return The colour at @p number or zero alpha black for an empty set.
  [[nodiscard]] Colour cycle(size_t number) const;
  // /// @overload
  // [[nodiscard]] Colour cycle(int number) const { return cycle(static_cast<size_t>(number)); }

  /// Index operator, aliasing @c cycle().
  ///
  /// Out of range indexing is safe.
  ///
  /// @param index The colour index.
  /// @return The colour at @p index or zero alpha black for an empty set.
  [[nodiscard]] Colour operator[](size_t index) const { return cycle(index); }
  // /// @overload
  // [[nodiscard]] Colour operator[](int index) const { return cycle(index); }

  /// Retrieve a predefined colour set by enum.
  /// @param name The predefined set enum/name.
  /// @return The predefined colour set.
  [[nodiscard]] static const ColourSet &predefined(PredefinedSet name);

private:
  std::vector<Colour> _colours;
};


namespace literals
{
// NOLINTNEXTLINE(google-runtime-int)
inline Colour operator"" _rgba(unsigned long long colour_value)
{
  return { static_cast<uint32_t>(colour_value) };
}

// NOLINTNEXTLINE(google-runtime-int)
inline Colour operator"" _rgb(unsigned long long colour_value)
{
  return Colour(Colour(static_cast<uint32_t>(colour_value), 255));
}
}  // namespace literals


inline Colour::Colour(uint32_t colour_value) noexcept
{
  ConverterUInt32()(colour_value, _storage);
}


inline Colour::Colour(const Colour &other) noexcept
{
  std::copy(other._storage.begin(), other._storage.end(), _storage.begin());
}


inline Colour::Colour(const Colour &other, uint8_t alpha) noexcept
  : Colour(other)
{
  _storage[static_cast<int>(Channel::A)] = alpha;
}


inline Colour::Colour(const Colour &other, int alpha) noexcept
  : Colour(other, static_cast<uint8_t>(alpha))
{}


inline Colour::Colour(const Colour &other, float alpha) noexcept
  : Colour(other)
{
  setAf(alpha);
}


inline Colour::Colour(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) noexcept
{
  r() = red;
  g() = green;
  b() = blue;
  a() = alpha;
}


inline Colour::Colour(const std::array<uint8_t, 4> &array) noexcept
  : _storage(array)
{}


inline Colour::Colour(int red, int green, int blue, int alpha) noexcept
  : Colour(static_cast<uint8_t>(red), static_cast<uint8_t>(green), static_cast<uint8_t>(blue),
           static_cast<uint8_t>(alpha))
{}


inline Colour::Colour(float red, float green, float blue, float alpha) noexcept
{
  setRf(red);
  setGf(green);
  setBf(blue);
  setAf(alpha);
}

inline uint32_t Colour::colour32() const
{
  return ConverterUInt32()(_storage);
}

inline float Colour::rf() const
{
  return getf(Channel::R);
}


inline float Colour::gf() const
{
  return getf(Channel::G);
}


inline float Colour::bf() const
{
  return getf(Channel::B);
}


inline float Colour::af() const
{
  return getf(Channel::A);
}


inline void Colour::setRf(float value)
{
  setf(value, Channel::R);
}


inline void Colour::setGf(float value)
{
  setf(value, Channel::G);
}


inline void Colour::setBf(float value)
{
  setf(value, Channel::B);
}


inline void Colour::setAf(float value)
{
  setf(value, Channel::A);
}


inline void Colour::setf(float value, Channel channel)
{
  _storage[static_cast<int>(channel)] = static_cast<uint8_t>(value * 255.0f);
}


inline float Colour::getf(Channel channel) const
{
  return static_cast<float>(_storage[static_cast<int>(channel)]) / 255.0f;
}


inline Colour Colour::adjust(float factor) const
{
  float hue = {};
  float saturation = {};
  float value = {};
  Colour colour;
  rgbToHsv(hue, saturation, value, rf(), gf(), bf());
  value = std::max(0.0f, std::min(value * factor, 1.0f));
  hsvToRgb(colour.red(), colour.green(), colour.blue(), hue, saturation, value);
  colour.alpha() = alpha();
  return colour;
}


inline Colour &Colour::operator=(const Colour &other)
{
  std::copy(other._storage.begin(), other._storage.end(), _storage.begin());
  return *this;
}


inline bool Colour::operator==(const Colour &other) const
{
  return _storage == other._storage;
}


inline bool Colour::operator!=(const Colour &other) const
{
  return _storage != other._storage;
}

inline Colour operator*(const Colour &opa, const Colour &opb)
{
  Colour colour;
  for (int i = 0; i < 4; ++i)
  {
    const auto chi = static_cast<Colour::Channel>(i);
    colour.setf(std::min(opa.getf(chi) * opb.getf(chi), 1.0f), chi);
  }

  return colour;
}

inline Colour operator/(const Colour &opa, const Colour &opb)
{
  Colour colour;
  for (int i = 0; i < 4; ++i)
  {
    const auto chi = static_cast<Colour::Channel>(i);
    colour.setf(std::min(opa.getf(chi) / opb.getf(chi), 1.0f), chi);
  }

  return colour;
}


inline Colour operator+(const Colour &opa, const Colour &opb)
{
  Colour colour;
  for (int i = 0; i < 4; ++i)
  {
    // Should we add the squares of the channels, then sqrt the result?
    // See this Minute Physics video: https://youtu.be/LKnqECcg6Gw
    const auto chi = static_cast<Colour::Channel>(i);
    colour.setf(std::min(opa.getf(chi) + opb.getf(chi), 1.0f), chi);
  }

  return colour;
}


inline Colour operator-(const Colour &opa, const Colour &opb)
{
  Colour colour;
  for (int i = 0; i < 4; ++i)
  {
    // Should we add the squares of the channels, then sqrt the result?
    // See this Minute Physics video: https://youtu.be/LKnqECcg6Gw
    const auto chi = static_cast<Colour::Channel>(i);
    colour.setf(std::min(opa.getf(chi) - opb.getf(chi), 1.0f), chi);
  }

  return colour;
}


inline Colour Colour::fromHsv(float hue, float saturation, float value, float alpha)
{
  float red = {};
  float green = {};
  float blue = {};
  Colour::hsvToRgb(red, green, blue, hue, saturation, value);
  return { red, green, blue, alpha };
}


inline uint32_t Colour::ConverterUInt32::operator()(const std::array<uint8_t, 4> &storage) const
{
  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  return (storage[kRedIndex] << kRedShift) | (storage[kGreenIndex] << kGreenShift) |
         (storage[kBlueIndex] << kBlueShift) | (storage[kAlphaIndex] << kAlphaShift);
}


inline void Colour::ConverterUInt32::operator()(uint32_t colour,
                                                std::array<uint8_t, 4> &storage) const
{
  storage[kRedIndex] = static_cast<uint8_t>((colour >> kRedShift) & 0xffu);
  storage[kGreenIndex] = static_cast<uint8_t>((colour >> kGreenShift) & 0xffu);
  storage[kBlueIndex] = static_cast<uint8_t>((colour >> kBlueShift) & 0xffu);
  storage[kAlphaIndex] = static_cast<uint8_t>((colour >> kAlphaShift) & 0xffu);
}
}  // namespace tes

#endif  // TES_CORE_COLOUR_H
