//
// Author: Kazys Stepanas
//
#ifndef TES_VIEWER_PAINTER_TEXT_H
#define TES_VIEWER_PAINTER_TEXT_H

#include "3es-viewer.h"

#include "util/3esenum.h"

#include <Corrade/PluginManager/Manager.h>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Shaders/DistanceFieldVector.h>
#include <Magnum/Text/Renderer.h>
#include <Magnum/Text/AbstractFont.h>

#include <memory>
#include <functional>

namespace Magnum::Text
{
class AbstractFont;
class DistanceFieldGlyphCache;
}  // namespace Magnum::Text

namespace tes::viewer
{
struct DrawParams;
}  // namespace tes::viewer

namespace tes::viewer::painter
{
/// This class renders 2D and 3D text using an immediate mode style rendering.
///
/// The two public draw methods - @c draw2D() and @c draw3D() - are designed to draw a collection of @c TextEntry items.
/// The expected usage is to have @c TextEntry items in either a vector or unordered map. As such we don't explicitly
/// know how to interface with the containers. Instead these methods accept forward iterators - begin and end pairs -
/// and a @c resolver object function which resolves the iterator to a @c TextEntry.
///
/// We can render a vector or unordered map of @c TextEntry items using:
///
/// ```c++
/// void draw(Text &renderer, std::vector<Text::TextEntry> &text, const DrawParams &params)
/// {
///   renderer.draw2D(text.begin(), text.end(),
///   /* resolver */  [] (std::vector<Text::TextEntry>::iterator &iter) { return *iter; },
///                   params);
/// }
///
/// void draw(Text &renderer, std::unordered_map<int, Text::TextEntry> &text, const DrawParams &params)
/// {
///   renderer.draw2D(text.begin(), text.end(),
///   /* resolver */  [] (std::unordered_map<int, Text::TextEntry>::iterator &iter) { return iter->second; },
///                   params);
/// }
/// ```
///
/// This interface also supports rendering from a container which indirectly contains @c TextEntry items.
class TES_VIEWER_API Text
{
public:
  /// A hard limit to the number of characters we can render in a text string.
  static constexpr unsigned kMaxTextLength = 1024u;

  /// Flags which affect text rendering.
  enum class TextFlag : unsigned
  {
    /// No flags.
    Zero = 0,
    /// A flag for 2D text, which projects a 3D world position on the 2D overlay.
    ///
    /// This is considered 2D text because it is rendered in the 2D overlay. It may dissapear from view as the camera
    /// pans around.
    ScreenProjected = (1 << 0),
    /// A flag for 3D text which keeps the text facing the camera.
    ScreenFacing = (1 << 1),
  };

  /// A text entry to render.
  struct TES_VIEWER_API TextEntry
  {
    /// The text to render. May be truncated by the rendering limit.
    std::string text;
    /// The text transformation matrix.
    Magnum::Matrix4 transform;
    /// Text colour.
    Magnum::Color4 colour;
    /// Text render scale.
    Magnum::Float font_size = 1;
    /// Rendering flags.
    TextFlag flags = TextFlag::Zero;
  };

  /// Construct a text renderer.
  /// @param font_manager The font manager plugin to load fonts from.
  /// @param font_resource_name The true type font resource name.
  /// @param fonts_resource_section The resource data section containing the font.
  /// @param font_plugin The font plugin name.
  Text(Corrade::PluginManager::Manager<Magnum::Text::AbstractFont> &font_manager,
       const std::string &font_resource_name = "SourceSansPro-Regular.ttf",
       const std::string &fonts_resource_section = "fonts", const std::string &font_plugin = "TrueTypeFont");

  /// Check if we are able to render text.
  /// @return True if text rendering is availabe.
  bool isAvailable() const;

  /// Draw a collection of 2D @c TextEntry items.
  ///
  /// See class comments for usage examples.
  ///
  /// @tparam Iter The forward iterator type for a collection of @c TextEntry items.
  /// @tparam Resolver The @c TextEntry resolver. A callable object of the form:
  ///   `std::function<const TextEntry &(const Iter &)>`
  /// @param begin The first item to render.
  /// @param end The end iterator.
  /// @param resolver A functional object which resolves an @c Iter object to a @c TextEntry reference.
  /// @param params Draw parameters.
  template <typename Iter, typename Resolver>
  void draw2D(const Iter &begin, const Iter &end, const Resolver &resolver, const DrawParams &params);

  /// A convenience function for drawing a single text item.
  ///
  /// Uses the template overload of @c draw2D().
  ///
  /// @param text The text item to draw.
  /// @param params Draw parameters.
  void draw2D(const TextEntry &text, const DrawParams &params)
  {
    const auto draw_item = [&text](const int) { return text; };
    draw2D(0, 1, draw_item, params);
  }

  /// Draw a collection of 3D @c TextEntry items.
  ///
  /// See class comments for usage examples.
  ///
  /// @tparam Iter The forward iterator type for a collection of @c TextEntry items.
  /// @tparam Resolver The @c TextEntry resolver. A callable object of the form:
  ///   `std::function<const TextEntry &(const Iter &)>`
  /// @param begin The first item to render.
  /// @param end The end iterator.
  /// @param resolver A functional object which resolves an @c Iter object to a @c TextEntry reference.
  /// @param params Draw parameters.
  template <typename Iter, typename Resolver>
  void draw3D(const Iter &begin, const Iter &end, const Resolver &resolver, const DrawParams &params);

  /// A convenience function for drawing a single text item.
  ///
  /// Uses the template overload of @c draw3D().
  ///
  /// @param text The text item to draw.
  /// @param params Draw parameters.
  void draw3D(const TextEntry &text, const DrawParams &params)
  {
    const auto draw_item = [&text](const int) { return text; };
    draw3D(0, 1, draw_item, params);
  }

private:
  /// Common setup for drawing of 2D or 3D text.
  void beginDraw();
  /// Common tear down for drawing of 2D or 3D text.
  void endDraw();

  /// Setup drawing of 2D text.
  void beginDraw2D();
  /// Tear down drawing of 2D text.
  void endDraw2D();

  /// Setup drawing of 3D text.
  void beginDraw3D();
  /// Tear down drawing of 3D text.
  void endDraw3D();

  /// Draw 2D text.
  ///
  /// 2D text is rendered in an overlay and always the same size. This should only be called during the overlay draw
  /// pass.
  void draw2DText(const TextEntry &text, const DrawParams &params);

  /// Draw 3D text.
  ///
  /// 3D text is rendered in the scene and is perspective scaled. 3D text should be rendered during the opaque draw
  /// pass. This should only be called during the opaque or transparent rendering passes.
  void draw3DText(const TextEntry &text, const DrawParams &params);

  /// Draw a single text item.
  /// @tparam Renderer The text renderer type: `[Magnum::Text::Renderer2D, Magnum::Text::Renderer3D]`
  /// @tparam Matrix The text transformation matrix type of rank 3 or 4.
  /// @tparam Shader The text shader type:
  ///   `[Magnum::Shader::DistanceFieldVector2D, Magnum::Shader::DistanceFieldVector3D]`
  /// @param text The text item to draw.
  /// @param full_projection_matrix The full transformation matrix including projection, camera and model transforms.
  /// @param renderer The text renderer.
  /// @param shader The text shader.
  template <typename Matrix, typename Renderer, typename Shader>
  void draw(const TextEntry &text, const Matrix &full_projection_matrix, Renderer &renderer, Shader &shader);

  std::unique_ptr<Magnum::Text::Renderer2D> _renderer_2d;
  std::unique_ptr<Magnum::Text::Renderer3D> _renderer_3d;
  /// Default transformation matrix required to get the text from facing along Z to align with the world up vector,
  /// facing back along the forward vector.
  Magnum::Matrix4 _default_transform;

  std::unique_ptr<Magnum::Text::AbstractFont> _font = nullptr;
  Magnum::Shaders::DistanceFieldVector2D _shader_2d;
  Magnum::Shaders::DistanceFieldVector3D _shader_3d;
  std::unique_ptr<Magnum::Text::DistanceFieldGlyphCache> _cache;
};

template <typename Iter, typename Resolver>
void Text::draw2D(const Iter &begin, const Iter &end, const Resolver &resolver, const DrawParams &params)
{
  if (!isAvailable())
  {
    return;
  }

  beginDraw2D();
  _shader_2d.bindVectorTexture(_cache->texture());
  for (auto iter = begin; iter != end; ++iter)
  {
    draw2DText(resolver(iter), params);
  }
  endDraw2D();
}

TES_ENUM_FLAGS(Text::TextFlag, unsigned);

template <typename Iter, typename Resolver>
void Text::draw3D(const Iter &begin, const Iter &end, const Resolver &resolver, const DrawParams &params)
{
  if (!isAvailable())
  {
    return;
  }

  beginDraw3D();
  _shader_3d.bindVectorTexture(_cache->texture());
  for (auto iter = begin; iter != end; ++iter)
  {
    draw3DText(resolver(iter), params);
  }
  endDraw3D();
}
}  // namespace tes::viewer::painter

#endif  // TES_VIEWER_PAINTER_TEXT_H
