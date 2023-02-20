//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_HANDLER_TEXT2D_H
#define TES_VIEW_HANDLER_TEXT2D_H

#include <3esview/ViewConfig.h>

#include "Text.h"

#include <3escore/shapes/Text2d.h>

namespace tes::view::handler
{
/// Affordances class for @c tes::Text2D use with the template @c Text handler.
class TES_VIEWER_API Text2DAffordances
{
public:
  /// Identifies the text drawing mode as using @c painter::Text::draw2D() - when @c true - or using
  /// @c painter::Text::draw3D() - when @c false .
  /// @return True to draw 2D text, false to draw 3D text.
  constexpr static bool is2D() { return true; }

  /// Configure a @c painter::Text::TextEntry from a @c tes::Text2D .
  /// @param shape The source shape data.
  /// @param entry The entry to configure.
  static void configure(const tes::Text2D &shape, painter::Text::TextEntry &entry)
  {
    entry.text = shape.text();
    entry.transform = Magnum::Matrix4::translation(convert(shape.position()));
    entry.colour = convert(shape.colour());
    entry.flags |= (shape.inWorldSpace()) ? painter::Text::TextFlag::ScreenProjected :
                                            painter::Text::TextFlag::Zero;
  }

  /// Configure a @c tes::Text2D from a @c painter::Text::TextEntry .
  /// @param entry The source entry data.
  /// @param shape The shape to configure.
  static void configure(const painter::Text::TextEntry &entry, tes::Text2D &shape)
  {
    shape.setText(entry.text);
    shape.setPosition(convert(entry.transform[3].xyz()));
    shape.setInWorldSpace((entry.flags & painter::Text::TextFlag::ScreenProjected) ==
                          painter::Text::TextFlag::ScreenProjected);
  }
};

/// Message handler for drawing 2D overlay text.
class TES_VIEWER_API Text2D : public Text<tes::Text2D, Text2DAffordances>
{
public:
  /// Superclass alias.
  using Super = Text<tes::Text2D, Text2DAffordances>;

  /// Construct using the given text painter interface.
  /// @param painter The text drawing API.
  Text2D(std::shared_ptr<painter::Text> painter)
    : Super(SIdText2D, "text 2D", painter)
  {}
};
}  // namespace tes::view::handler

#endif  // TES_VIEW_HANDLER_TEXT2D_H
