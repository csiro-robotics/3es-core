#include "Fly.h"

namespace tes::camera
{
Fly::Fly() = default;

void Fly::updateMouse(float dx, float dy, Camera &camera)
{
  if (isSet(Flag::InvertMouseY))
  {
    dy *= -1.0f;
  }
  if (isSet(Flag::InvertMouseX))
  {
    dx *= -1.0f;
  }
  camera.pitch -= dy * _mouse_sensitivity * _mouse_multiplier;
  camera.yaw -= dx * _mouse_sensitivity * _mouse_multiplier;

  clampRotation(camera);
}

void Fly::updateKeys(float dt, Magnum::Vector3i translate, Magnum::Vector3i rotate, Camera &camera)
{
  if (isSet(Flag::InvertKeyMoveX))
  {
    translate.x() *= -1;
  }
  if (isSet(Flag::InvertKeyMoveY))
  {
    translate.y() *= -1;
  }
  if (isSet(Flag::InvertKeyMoveZ))
  {
    translate.z() *= -1;
  }
  if (isSet(Flag::InvertKeyRotateX))
  {
    rotate.x() *= -1;
  }
  if (isSet(Flag::InvertKeyRotateY))
  {
    rotate.y() *= -1;
  }
  if (isSet(Flag::InvertKeyRotateZ))
  {
    rotate.z() *= -1;
  }

  const auto delta_translate =
    Magnum::Vector3(_move_speed * _move_multiplier * dt) * Magnum::Vector3(translate);
  const auto delta_rotate =
    Magnum::Vector3(_rotation_speed * _rotation_multiplier * dt) * Magnum::Vector3(rotate);

  camera.pitch += delta_rotate.x();
  camera.yaw += delta_rotate.y();
  clampRotation(camera);

  // Get the camera transform to extract the translation axes.
  Magnum::Matrix4 camera_transform = camera::matrix(camera);

  camera.position += camera_transform[0].xyz() * delta_translate.x();
  camera.position += camera_transform[1].xyz() * delta_translate.y();
  camera.position += camera_transform[2].xyz() * delta_translate.z();
}


}  // namespace tes::camera
