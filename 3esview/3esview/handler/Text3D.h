//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_HANDLER_TEXT3D_H
#define TES_VIEW_HANDLER_TEXT3D_H

#include <3esview/ViewConfig.h>

#include "Text.h"

#include <3escore/shapes/Text3D.h>

namespace tes::view::handler
{

/// Affordances class for @c tes::Text2D use with the template @c Text handler.
class TES_VIEWER_API Text3DAffordances
{
public:
  /// Identifies the text drawing mode as using @c painter::Text::draw2D() - when @c true - or using
  /// @c painter::Text::draw3D() - when @c false .
  /// @return True to draw 2D text, false to draw 3D text.
  constexpr static bool is2D() { return false; }

  /// Configure a @c painter::Text::TextEntry from a @c tes::Text3D .
  /// @param shape The source shape data.
  /// @param entry The entry to configure.
  static void configure(const tes::Text3D &shape, painter::Text::TextEntry &entry)
  {
    tes::Text3D mutable_shape = shape;
    entry.text = mutable_shape.text();
    // Read font size which comes from scale.
    entry.font_size = Magnum::Float(mutable_shape.fontSize());
    // Remove the font size scaling before we compose the transform.
    mutable_shape.setFontSize(1.0);
    entry.transform = Message::composeTransform(mutable_shape.attributes());
    entry.colour = convert(mutable_shape.colour());
    entry.flags |= (mutable_shape.screenFacing()) ? painter::Text::TextFlag::ScreenFacing :
                                                    painter::Text::TextFlag::Zero;
  }

  /// Configure a @c tes::Text3D from a @c painter::Text::TextEntry .
  /// @param entry The source entry data.
  /// @param shape The shape to configure.
  static void configure(const painter::Text::TextEntry &entry, tes::Text3D &shape)
  {
    shape.setText(entry.text);

    Message::ObjectAttributes attrs = {};
    Message::decomposeTransform(entry.transform, attrs);
    shape.setPosition(tes::Vector3f(attrs.position[0], attrs.position[1], attrs.position[2]));
    shape.setRotation(
      tes::Quaternionf(attrs.rotation[0], attrs.rotation[1], attrs.rotation[2], attrs.rotation[3]));
    shape.setScale(tes::Vector3f(attrs.scale[0], attrs.scale[1], attrs.scale[2]));
    // Set the font size, which will adjust the scale.
    shape.setFontSize(entry.font_size);

    shape.setScreenFacing((entry.flags & painter::Text::TextFlag::ScreenFacing) ==
                          painter::Text::TextFlag::ScreenFacing);
  }
};

/// Message handler for drawing 3D positioned text.
class TES_VIEWER_API Text3D : public Text<tes::Text3D, Text3DAffordances>
{
public:
  /// Superclass alias.
  using Super = Text<tes::Text3D, Text3DAffordances>;

  /// Construct using the given text painter interface.
  /// @param painter The text drawing API.
  Text3D(std::shared_ptr<painter::Text> painter)
    : Super(SIdText3D, "text 3D", painter)
  {}
};
}  // namespace tes::view::handler

#endif  // TES_VIEW_HANDLER_TEXT3D_H
