//
// author Kazys Stepanas
//
// Copyright (c) Kazys Stepanas 2014
//
#include "Colour.h"

#include <algorithm>

namespace tes
{
Colour::Colour(NamedColour name)
{
  thread_local const std::array<Colour, Colour::PredefinedLast + 1> colours =  //
    {
      Colour(220, 220, 220),  //
      Colour(211, 211, 211),  //
      Colour(192, 192, 192),  //
      Colour(169, 169, 169),  //
      Colour(128, 128, 128),  //
      Colour(105, 105, 105),  //
      Colour(119, 136, 153),  //
      Colour(112, 128, 144),  //
      Colour(47, 79, 79),     //
      Colour(0, 0, 0),        //
      Colour(255, 255, 255),  //
      Colour(255, 250, 250),  //
      Colour(240, 255, 240),  //
      Colour(245, 255, 250),  //
      Colour(240, 255, 255),  //
      Colour(240, 248, 255),  //
      Colour(248, 248, 255),  //
      Colour(245, 245, 245),  //
      Colour(255, 245, 238),  //
      Colour(245, 245, 220),  //
      Colour(253, 245, 230),  //
      Colour(255, 250, 240),  //
      Colour(255, 255, 240),  //
      Colour(250, 235, 215),  //
      Colour(250, 240, 230),  //
      Colour(255, 240, 245),  //
      Colour(255, 228, 225),  //
      Colour(255, 192, 203),  //
      Colour(255, 182, 193),  //
      Colour(255, 105, 180),  //
      Colour(255, 20, 147),   //
      Colour(219, 112, 147),  //
      Colour(199, 21, 133),   //
      Colour(255, 160, 122),  //
      Colour(250, 128, 114),  //
      Colour(233, 150, 122),  //
      Colour(240, 128, 128),  //
      Colour(205, 92, 92),    //
      Colour(220, 20, 60),    //
      Colour(178, 34, 34),    //
      Colour(139, 0, 0),      //
      Colour(255, 0, 0),      //
      Colour(255, 69, 0),     //
      Colour(255, 99, 71),    //
      Colour(255, 127, 80),   //
      Colour(255, 140, 0),    //
      Colour(255, 165, 0),    //
      Colour(255, 255, 0),    //
      Colour(255, 255, 224),  //
      Colour(255, 250, 205),  //
      Colour(250, 250, 210),  //
      Colour(255, 239, 213),  //
      Colour(255, 228, 181),  //
      Colour(255, 218, 185),  //
      Colour(238, 232, 170),  //
      Colour(240, 230, 140),  //
      Colour(189, 183, 107),  //
      Colour(255, 215, 0),    //
      Colour(255, 248, 220),  //
      Colour(255, 235, 205),  //
      Colour(255, 228, 196),  //
      Colour(255, 222, 173),  //
      Colour(245, 222, 179),  //
      Colour(222, 184, 135),  //
      Colour(210, 180, 140),  //
      Colour(188, 143, 143),  //
      Colour(244, 164, 96),   //
      Colour(218, 165, 32),   //
      Colour(184, 134, 11),   //
      Colour(205, 133, 63),   //
      Colour(210, 105, 30),   //
      Colour(139, 69, 19),    //
      Colour(160, 82, 45),    //
      Colour(165, 42, 42),    //
      Colour(128, 0, 0),      //
      Colour(85, 107, 47),    //
      Colour(128, 128, 0),    //
      Colour(107, 142, 35),   //
      Colour(154, 205, 50),   //
      Colour(50, 205, 50),    //
      Colour(0, 255, 0),      //
      Colour(124, 252, 0),    //
      Colour(127, 255, 0),    //
      Colour(173, 255, 47),   //
      Colour(0, 255, 127),    //
      Colour(0, 250, 154),    //
      Colour(144, 238, 144),  //
      Colour(152, 251, 152),  //
      Colour(143, 188, 143),  //
      Colour(60, 179, 113),   //
      Colour(46, 139, 87),    //
      Colour(34, 139, 34),    //
      Colour(0, 128, 0),      //
      Colour(0, 100, 0),      //
      Colour(102, 205, 170),  //
      Colour(0, 255, 255),    //
      Colour(0, 255, 255),    //
      Colour(224, 255, 255),  //
      Colour(175, 238, 238),  //
      Colour(127, 255, 212),  //
      Colour(64, 224, 208),   //
      Colour(72, 209, 204),   //
      Colour(0, 206, 209),    //
      Colour(32, 178, 170),   //
      Colour(95, 158, 160),   //
      Colour(0, 139, 139),    //
      Colour(0, 128, 128),    //
      Colour(176, 196, 222),  //
      Colour(176, 224, 230),  //
      Colour(173, 216, 230),  //
      Colour(135, 206, 235),  //
      Colour(135, 206, 250),  //
      Colour(0, 191, 255),    //
      Colour(30, 144, 255),   //
      Colour(100, 149, 237),  //
      Colour(70, 130, 180),   //
      Colour(65, 105, 225),   //
      Colour(0, 0, 255),      //
      Colour(0, 0, 205),      //
      Colour(0, 0, 139),      //
      Colour(0, 0, 128),      //
      Colour(25, 25, 112),    //
      Colour(230, 230, 250),  //
      Colour(216, 191, 216),  //
      Colour(221, 160, 221),  //
      Colour(238, 130, 238),  //
      Colour(218, 112, 214),  //
      Colour(255, 0, 255),    //
      Colour(255, 0, 255),    //
      Colour(186, 85, 211),   //
      Colour(147, 112, 219),  //
      Colour(138, 43, 226),   //
      Colour(148, 0, 211),    //
      Colour(153, 50, 204),   //
      Colour(139, 0, 139),    //
      Colour(128, 0, 128),    //
      Colour(75, 0, 130),     //
      Colour(72, 61, 139),    //
      Colour(106, 90, 205),   //
      Colour(123, 104, 238),  //
    };

  *this = colours[static_cast<int>(name)];
}


void Colour::rgbToHsv(float &hue, float &saturation, float &value, const float red,
                      const float green, const float blue)
{
  const float cmin = std::min<float>(red, std::min<float>(green, blue));
  const float cmax = std::max<float>(red, std::max<float>(green, blue));
  const float delta = cmax - cmin;

  const float yellow_to_magenta = (red == cmax && cmax != 0) ? 1.0f : 0.0f;
  const float cyan_to_yellow = (green == cmax && cmax != 0) ? 1.0f : 0.0f;
  const float magenta_to_cyan = (blue == cmax && cmax != 0) ? 1.0f : 0.0f;
  const float degrees_per_sector = 60.0f;

  value = cmax;
  saturation = (cmax != 0) ? delta / cmax : 0;
  hue = (yellow_to_magenta * ((green - blue) / delta) + cyan_to_yellow * ((green - blue) / delta) +
         magenta_to_cyan * ((green - blue) / delta)) *
        degrees_per_sector;
}


void Colour::hsvToRgb(float &red, float &green, float &blue, const float hue,
                      const float saturation, const float value)
{
  const float degrees_per_sector = 60.0f;
  const float hue_sector = hue / degrees_per_sector;  // sector 0 to 5
  const int sector_index =
    static_cast<int>(std::min<float>(std::max<float>(0.0f, std::floor(hue_sector)), 5.0f));
  // NOLINTBEGIN(readability-identifier-length)
  const float f = hue_sector - static_cast<float>(sector_index);
  const float p = value * (1 - saturation);
  const float q = value * (1 - saturation * f);
  const float t = value * (1 - saturation * (1 - f));
  // NOLINTEND(readability-identifier-length)

  static const std::array<int, 6> vindex = { 0, 1, 1, 2, 2, 0 };
  static const std::array<int, 6> pindex = { 2, 2, 0, 0, 1, 1 };
  static const std::array<int, 6> qindex = { 3, 0, 3, 1, 3, 2 };
  static const std::array<int, 6> tindex = { 1, 3, 2, 3, 0, 3 };

  std::array<float, 4> rgb;
  rgb[vindex[sector_index]] = value;
  rgb[pindex[sector_index]] = p;
  rgb[qindex[sector_index]] = q;
  rgb[tindex[sector_index]] = t;

  // Handle achromatic here by testing saturation inline.
  red = (saturation != 0) ? rgb[0] : value;
  green = (saturation != 0) ? rgb[1] : value;
  blue = (saturation != 0) ? rgb[2] : value;
}


void Colour::hsvToRgb(uint8_t &red, uint8_t &green, uint8_t &blue, const float hue,
                      const float saturation, const float value)
{
  float redf = {};
  float greenf = {};
  float bluef = {};
  hsvToRgb(redf, greenf, bluef, hue, saturation, value);
  const auto scale = 255.0f;
  red = static_cast<uint8_t>(redf * scale);
  green = static_cast<uint8_t>(greenf * scale);
  blue = static_cast<uint8_t>(bluef * scale);
}


ColourSet::ColourSet() = default;


ColourSet::ColourSet(std::initializer_list<Colour> colours)
  : _colours(colours)
{}


ColourSet::~ColourSet() = default;


ColourSet::ColourSet(ColourSet &&other) noexcept = default;
ColourSet::ColourSet(const ColourSet &other) = default;
ColourSet &ColourSet::operator=(ColourSet &&other) noexcept = default;
ColourSet &ColourSet::operator=(const ColourSet &other) = default;


Colour ColourSet::cycle(size_t number) const
{
  if (!_colours.empty())
  {
    return _colours[number % _colours.size()];
  }
  return { 0 };
}


const ColourSet &ColourSet::predefined(PredefinedSet name)
{
  thread_local std::array<ColourSet, LastSet + 1> sets = {
    // Standard
    ColourSet{ {
      Colour(Colour::Red),
      Colour(Colour::Green),
      Colour(Colour::Blue),
      Colour(Colour::MediumOrchid),
      Colour(Colour::Olive),
      Colour(Colour::Teal),
      Colour(Colour::Black),
      Colour(Colour::OrangeRed),
      Colour(Colour::Yellow),
      Colour(Colour::MediumAquamarine),
      Colour(Colour::Gainsboro),
      Colour(Colour::White),
      Colour(Colour::Pink),
      Colour(Colour::LightSalmon),
      Colour(Colour::Tomato),
      Colour(Colour::DarkOliveGreen),
      Colour(Colour::Aqua),
      Colour(Colour::LightSteelBlue),
      Colour(Colour::Silver),
      Colour(Colour::HotPink),
      Colour(Colour::Salmon),
      Colour(Colour::Coral),
      Colour(Colour::Wheat),
      Colour(Colour::Olive),
      Colour(Colour::PowderBlue),
      Colour(Colour::Thistle),
      Colour(Colour::DarkGrey),
      Colour(Colour::DeepPink),
      Colour(Colour::DarkSalmon),
      Colour(Colour::DarkOrange),
      Colour(Colour::Moccasin),
      Colour(Colour::BurlyWood),
      Colour(Colour::OliveDrab),
      Colour(Colour::Aquamarine),
      Colour(Colour::LightBlue),
      Colour(Colour::Plum),
      Colour(Colour::DimGrey),
      Colour(Colour::PaleVioletRed),
      Colour(Colour::LightCoral),
      Colour(Colour::Orange),
      Colour(Colour::PeachPuff),
      Colour(Colour::Tan),
      Colour(Colour::YellowGreen),
      Colour(Colour::Turquoise),
      Colour(Colour::SkyBlue),
      Colour(Colour::Violet),
      Colour(Colour::SlateGrey),
      Colour(Colour::MediumVioletRed),
      Colour(Colour::IndianRed),
      Colour(Colour::RosyBrown),
      Colour(Colour::LimeGreen),
      Colour(Colour::MediumTurquoise),
      Colour(Colour::DeepSkyBlue),
      Colour(Colour::Orchid),
      Colour(Colour::DarkSlateGrey),
      Colour(Colour::Crimson),
      Colour(Colour::Khaki),
      Colour(Colour::SandyBrown),
      Colour(Colour::Lime),
      Colour(Colour::DarkTurquoise),
      Colour(Colour::CornflowerBlue),
      Colour(Colour::Fuchsia),
      Colour(Colour::FireBrick),
      Colour(Colour::DarkKhaki),
      Colour(Colour::DarkGoldenrod),
      Colour(Colour::LawnGreen),
      Colour(Colour::LightSeaGreen),
      Colour(Colour::SteelBlue),
      Colour(Colour::MediumPurple),
      Colour(Colour::DarkRed),
      Colour(Colour::Gold),
      Colour(Colour::Peru),
      Colour(Colour::MediumSpringGreen),
      Colour(Colour::CadetBlue),
      Colour(Colour::RoyalBlue),
      Colour(Colour::BlueViolet),
      Colour(Colour::Chocolate),
      Colour(Colour::LightGreen),
      Colour(Colour::DarkCyan),
      Colour(Colour::DarkBlue),
      Colour(Colour::DarkViolet),
      Colour(Colour::SaddleBrown),
      Colour(Colour::DarkSeaGreen),
      Colour(Colour::MidnightBlue),
      Colour(Colour::Purple),
      Colour(Colour::Sienna),
      Colour(Colour::MediumSeaGreen),
      Colour(Colour::Indigo),
      Colour(Colour::Brown),
      Colour(Colour::SeaGreen),
      Colour(Colour::DarkSlateBlue),
      Colour(Colour::Maroon),
      Colour(Colour::DarkGreen),
      Colour(Colour::SlateBlue),
    } },
    // Deuteranomaly
    ColourSet{ {
      Colour(Colour::RoyalBlue),
      Colour(Colour::Yellow),
      Colour(Colour::Silver),
      Colour(Colour::Black),
      Colour(Colour::Blue),
      Colour(Colour::Khaki),
      Colour(Colour::Gainsboro),
      Colour(Colour::Beige),
      Colour(Colour::Navy),
      Colour(Colour::DarkKhaki),
      Colour(Colour::White),
      Colour(Colour::Grey),
      Colour(Colour::MidnightBlue),
      Colour(Colour::SlateGrey),
      Colour(Colour::Ivory),
      Colour(Colour::Gold),
      Colour(Colour::DarkSlateBlue),
      Colour(Colour::MediumSlateBlue),
    } },
    // Protanomaly
    ColourSet{ {
      Colour(Colour::Blue),
      Colour(Colour::Yellow),
      Colour(Colour::Black),
      Colour(Colour::Silver),
      Colour(Colour::CornflowerBlue),
      Colour(Colour::Gainsboro),
      Colour(Colour::MediumSlateBlue),
      Colour(Colour::Khaki),
      Colour(Colour::Grey),
      Colour(Colour::DarkBlue),
      Colour(Colour::Beige),
      Colour(Colour::DarkKhaki),
      Colour(Colour::MidnightBlue),
      Colour(Colour::SlateGrey),
      Colour(Colour::RoyalBlue),
      Colour(Colour::Ivory),
      Colour(Colour::DarkSlateBlue),
    } },
    // Tritanomaly
    ColourSet{ {
      Colour(Colour::DeepSkyBlue),
      Colour(Colour::DeepPink),
      Colour(Colour::PaleTurquoise),
      Colour(Colour::Black),
      Colour(Colour::Crimson),
      Colour(Colour::LightSeaGreen),
      Colour(Colour::Gainsboro),
      Colour(Colour::Blue),
      Colour(Colour::DarkRed),
      Colour(Colour::Silver),
      Colour(Colour::Brown),
      Colour(Colour::DarkTurquoise),
      Colour(Colour::Grey),
      Colour(Colour::Maroon),
      Colour(Colour::Teal),
      Colour(Colour::SlateGrey),
      Colour(Colour::MidnightBlue),
      Colour(Colour::DarkSlateGrey),
    } },
    // Grey
    ColourSet{ {
      Colour(Colour::Black),
      Colour(Colour::Silver),
      Colour(Colour::DarkSlateGrey),
      Colour(Colour::Grey),
      Colour(Colour::Gainsboro),
      Colour(Colour::SlateGrey),
    } },
  };

  return sets[static_cast<int>(name)];
}
}  // namespace tes
