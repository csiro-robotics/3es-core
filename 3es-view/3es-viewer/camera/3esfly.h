#ifndef TES_VIEWER_CAMERA_FLY
#define TES_VIEWER_CAMERA_FLY
#include "3es-viewer.h"

#include "3escontroller.h"

namespace tes::camera
{
class Fly : public Controller
{
public:
  Fly();

  /// Get the movement speed for key translation updates: m/s.
  inline float moveSpeed() const { return _move_speed; }
  /// Set the movement speed for key translation updates: m/s.
  inline void setMoveSpeed(float move_speed) { _move_speed = move_speed; }
  /// Get the rotation speed for key rotation updates: radians/s.
  inline float rotationSpeed() const { return _rotation_speed; }
  /// Set the rotation speed for key rotation updates: radians/s.
  inline void setRotationSpeed(float rotation_speed) { _rotation_speed = rotation_speed; }
  /// Get the mouse sensitivity: radians/pixel.
  inline float mouseSensitivity() const { return _mouse_sensitivity; }
  /// Set the mouse sensitivity: radians/pixel.
  inline void setMouseSensitivity(float mouse_sensitivity) { _mouse_sensitivity = mouse_sensitivity; }
  /// Get the movement key speed multiplier.
  inline float moveMultiplier() const { return _move_multiplier; }
  /// Set the movement key speed multiplier.
  inline void setMoveMultiplier(float move_multiplier) { _move_multiplier = move_multiplier; }
  /// Get the rotation key speed multiplier.
  inline float rotationMultiplier() const { return _rotation_multiplier; }
  /// Set the rotation key speed multiplier.
  inline void setRotationMultiplier(float rotation_multiplier) { _rotation_multiplier = rotation_multiplier; }
  /// Get the mouse sensitivity multiplier.
  inline float mouseMultiplier() const { return _mouse_multiplier; }
  /// Set the mouse sensitivity multiplier.
  inline void setMouseMultiplier(float mouse_multiplier) { _mouse_multiplier = mouse_multiplier; }

  void updateMouse(float dx, float dy, Camera &camera) override;

  void updateKeys(float dt, Magnum::Vector3i translate, Magnum::Vector3i rotate, Camera &camera) override;

private:
  /// Movement speed for key translation updates: m/s.
  float _move_speed = 8.0f;
  /// Rotation speed for key rotation updates: radians/s.
  float _rotation_speed = float(Magnum::Rad(Magnum::Deg(90.0f)));
  /// Mouse sensitivity: radians/pixel.
  float _mouse_sensitivity = float(Magnum::Rad(Magnum::Deg(2.0f)));
  /// Current movement multiplier.
  float _move_multiplier = 1.0f;
  /// Current rotation multiplier.
  float _rotation_multiplier = 1.0f;
  /// Current mouse sensitivity multiplier.
  float _mouse_multiplier = 1.0f;
};
}  // namespace tes::camera

#endif  // TES_VIEWER_CAMERA_FLY
