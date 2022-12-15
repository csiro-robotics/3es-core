#ifndef TES_VIEWER_THIRD_EYE_SCENE_H
#define TES_VIEWER_THIRD_EYE_SCENE_H

#include "3es-viewer.h"

#include "camera/3esfly.h"

#include "3esboundsculler.h"
#include "3esframestamp.h"
#include "painter/3esshapecache.h"

#include <3esmessages.h>

#include <Corrade/PluginManager/Manager.h>

#include <Magnum/GL/FrameBuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Platform/GlfwApplication.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/Text/AbstractFont.h>

#include <array>
#include <chrono>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// TODO(KS): abstract away Magnum so it's not in any public headers.
namespace tes::viewer
{
struct DrawParams;
class EdlEffect;
class FboEffect;

namespace handler
{
class Message;
}

namespace painter
{
class ShapePainter;
class Text;
}  // namespace painter

namespace shaders
{
class ShaderLibrary;
}  // namespace shaders

class TES_VIEWER_API ThirdEyeScene
{
public:
  ThirdEyeScene();
  ~ThirdEyeScene();

  /// Get the list of names of known message handlers, keyed by routing ID.
  /// @return The known routing ID names.
  static const std::unordered_map<uint32_t, std::string> defaultHandlerNames();

  inline std::shared_ptr<BoundsCuller> culler() const { return _culler; }

  void setCamera(const camera::Camera &camera) { _camera = camera; }
  camera::Camera &camera() { return _camera; }
  const camera::Camera &camera() const { return _camera; }

  void setActiveFboEffect(std::shared_ptr<FboEffect> effect);
  void clearActiveFboEffect();
  std::shared_ptr<FboEffect> activeFboEffect() { return _active_fbo_effect; }
  const std::shared_ptr<FboEffect> &activeFboEffect() const { return _active_fbo_effect; }

  /// Access the shader library. This is for mesh rendering shaders.
  /// @return The shader library.
  std::shared_ptr<shaders::ShaderLibrary> shaderLibrary() const { return _shader_library; }

  /// Reset the current state, clearing all the currently visible data.
  void reset();

  void render(float dt, const Magnum::Vector2i &window_size);

  /// Update to the target frame number on the next @c render() call.
  ///
  /// Typically, this is called with a monotonic, increasing @p frame number, progressing on frame at a time.
  /// However, the frame number will jump when stepping and skipping frames.
  ///
  /// This function is called from the @c DataThread and is thread safe. The changes are not effected until the
  /// next @c render() call.
  ///
  /// @param frame The new frame number.
  void updateToFrame(FrameNumber frame);

  /// Updates the server information details.
  ///
  /// This is called on making a new connection and when details of that connection, such as the coordinate frame,
  /// change.
  ///
  /// Threadsafe.
  /// @param server_info The new server info.
  void updateServerInfo(const ServerInfoMessage &server_info);

  /// Process a message from the server. This is routed to the appropriate message handler.
  ///
  /// This function is not called for any control messages where the routing ID is @c MtControl.
  ///
  /// @note Message handling must be thread safe as this method is mostly called from a background thread. This
  /// constraint is placed on the message handlers.
  ///
  /// @param packet
  void processMessage(PacketReader &packet);

  void createSampleShapes();

private:
  void initialiseFont();
  void initialiseHandlers();
  void initialiseShaders();

  void updateCamera(float dt);
  void drawShapes(float dt, const DrawParams &params);

  std::shared_ptr<FboEffect> _active_fbo_effect;

  camera::Camera _camera;

  std::shared_ptr<BoundsCuller> _culler;
  std::shared_ptr<shaders::ShaderLibrary> _shader_library;

  std::unordered_map<ShapeHandlerIDs, std::shared_ptr<painter::ShapePainter>> _painters;
  std::unordered_map<uint32_t, std::shared_ptr<handler::Message>> _messageHandlers;
  /// Message handers arranged by update order..
  std::vector<std::shared_ptr<handler::Message>> _orderedMessageHandlers;
  /// List of unknown message handlers for which we've raised warnings. Cleared on @c reset().
  std::unordered_set<uint32_t> _unknown_handlers;

  std::shared_ptr<painter::Text> _text_painter;

  FrameStamp _render_stamp = {};

  Corrade::PluginManager::Manager<Magnum::Text::AbstractFont> _font_manager;

  std::mutex _render_mutex;
  FrameNumber _new_frame = 0;
  ServerInfoMessage _server_info = {};
  bool _have_new_frame = false;
  bool _new_server_info = false;
  std::atomic_bool _reset = false;
};
}  // namespace tes::viewer

#endif  // TES_VIEWER_THIRD_EYE_SCENE_H
