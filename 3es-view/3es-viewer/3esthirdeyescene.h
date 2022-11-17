#ifndef TES_VIEWER_THIRD_EYE_SCENE_H
#define TES_VIEWER_THIRD_EYE_SCENE_H

#include "3es-viewer.h"

#include "camera/3esfly.h"

#include "3esboundsculler.h"
#include "3esframestamp.h"
#include "painter/3esshapecache.h"
#include "painter/3esshapepainter.h"

#include <3esmessages.h>

#include <Magnum/GL/FrameBuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Platform/GlfwApplication.h>
#include <Magnum/Shaders/Flat.h>

#include <array>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <memory>

// TODO(KS): abstract away Magnum so it's not in any public headers.
namespace tes::viewer
{
class EdlEffect;
class FboEffect;

class ThirdEyeScene
{
public:
  ThirdEyeScene();

  inline std::shared_ptr<BoundsCuller> culler() const { return _culler; }

  void setCamera(const camera::Camera &camera) { _camera = camera; }
  camera::Camera &camera() { return _camera; }
  const camera::Camera &camera() const { return _camera; }

  void setActiveFboEffect(std::shared_ptr<FboEffect> effect);
  void clearActiveFboEffect();
  std::shared_ptr<FboEffect> activeFboEffect() { return _active_fbo_effect; }
  const std::shared_ptr<FboEffect> &activeFboEffect() const { return _active_fbo_effect; }

  void update(float dt, const Magnum::Vector2 &window_size);

private:
  void initialisePainters();

  void updateCamera(float dt);
  void drawShapes(float dt, const Magnum::Matrix4 &projection_matrix);

  std::shared_ptr<FboEffect> _active_fbo_effect;

  camera::Camera _camera;

  std::shared_ptr<BoundsCuller> _culler;

  std::unordered_map<ShapeHandlerIDs, std::shared_ptr<painter::ShapePainter>> _painters;

  FrameStamp _render_stamp = {};
};
}  // namespace tes::viewer

#endif  // TES_VIEWER_THIRD_EYE_SCENE_H
