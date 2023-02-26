//
// Author: Kazys Stepanas
//
#include "Camera.h"

#include <3escore/Connection.h>
#include <3escore/CoordinateFrame.h>
#include <3escore/Log.h>
#include <3escore/PacketWriter.h>

#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector3.h>

#include <array>

namespace tes::view::handler
{
Camera::Camera()
  : Message(MtCamera, "camera")
{}


size_t Camera::enumerate(std::vector<CameraId> &camera_ids) const
{
  std::lock_guard guard(_mutex);
  camera_ids.clear();
  for (size_t i = 0; i < _cameras.size(); ++i)
  {
    if (_cameras[i].second)
    {
      camera_ids.emplace_back(CameraId(i));
    }
  }
  return camera_ids.size();
}

bool Camera::lookup(CameraId camera_id, tes::camera::Camera &camera) const
{
  std::lock_guard guard(_mutex);
  camera = _cameras[camera_id].first;
  return _cameras[camera_id].second;
}


void Camera::initialise()
{}


void Camera::reset()
{
  std::lock_guard guard(_mutex);
  // Clear validity flags.
  for (auto &camera : _cameras)
  {
    camera.second = false;
  }
  _pending_cameras.clear();
}


void Camera::prepareFrame(const FrameStamp &stamp)
{
  (void)stamp;
}


void Camera::endFrame(const FrameStamp &stamp)
{
  (void)stamp;
  std::lock_guard guard(_mutex);
  for (auto &[id, camera] : _pending_cameras)
  {
    if (id < _cameras.size())
    {
      _cameras[id] = std::pair(camera, true);
      _first_valid = std::min(_first_valid, id);
    }
  }
  _pending_cameras.clear();
}


void Camera::draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params)
{
  (void)pass;
  (void)stamp;
  (void)params;
}


void Camera::readMessage(PacketReader &reader)
{
  tes::CameraMessage msg = {};
  if (!msg.read(reader))
  {
    log::error("Failed to read camera message.");
    return;
  }

  tes::camera::Camera camera = {};
  camera.position = Magnum::Vector3(msg.x, msg.y, msg.z);
  camera.clip_near = msg.near;
  camera.clip_far = msg.far;
  camera.fov_horizontal_deg = msg.fov;
  camera.frame = tes::CoordinateFrame(_server_info.coordinate_frame);

  // Determine pitch and yaw by a deviation from the expected axis.
  Magnum::Vector3 ref_dir;
  Magnum::Vector3 ref_up;
  getWorldAxes(camera.frame, nullptr, &ref_dir, &ref_up);
  calculatePitchYaw(Magnum::Vector3(msg.dirX, msg.dirY, msg.dirZ),
                    Magnum::Vector3(msg.upX, msg.upY, msg.upZ), ref_dir, ref_up, camera.pitch,
                    camera.yaw);

  std::lock_guard guard(_mutex);
  _pending_cameras.emplace_back(std::pair(msg.camera_id, camera));
}


void Camera::serialise(Connection &out, ServerInfoMessage &info)
{
  (void)info;
  std::lock_guard guard(_mutex);
  CameraMessage msg = {};
  const std::string error_str = "<error>";
  bool ok = true;

  const uint16_t buffer_size = 1024u;
  std::vector<uint8_t> packet_buffer(buffer_size, 0u);
  PacketWriter writer(packet_buffer.data(), buffer_size);
  Magnum::Vector3 dir;
  Magnum::Vector3 up;
  Magnum::Vector3 world_fwd;
  Magnum::Vector3 world_up;
  for (size_t i = 0; i < _cameras.size(); ++i)
  {
    if (!_cameras[i].second)
    {
      // Not valid.
      continue;
    }
    const auto camera = _cameras[i].first;
    msg.camera_id = uint8_t(i);
    msg.flags = 0;
    msg.reserved = 0;

    msg.x = camera.position.x();
    msg.y = camera.position.y();
    msg.z = camera.position.z();

    msg.near = camera.clip_near;
    msg.far = camera.clip_far;
    msg.fov = camera.fov_horizontal_deg;

    getWorldAxes(camera.frame, nullptr, &world_fwd, &world_up);
    calculateCameraAxes(camera.pitch, camera.yaw, world_fwd, world_up, dir, up);

    msg.dirX = dir[0];
    msg.dirY = dir[1];
    msg.dirZ = dir[2];

    msg.upX = up[0];
    msg.upY = up[1];
    msg.upZ = up[2];

    writer.reset(routingId(), 0);
    ok = msg.write(writer) && ok;
    ok = writer.finalise() && ok;
    ok = out.send(writer) >= 0 && ok;
  }

  if (!ok)
  {
    log::error("Camera serialisation failed.");
  }
}


void Camera::getWorldAxes(tes::CoordinateFrame frame, Magnum::Vector3 *side, Magnum::Vector3 *fwd,
                          Magnum::Vector3 *up)
{
  Magnum::Vector3 ref_side;
  Magnum::Vector3 ref_dir;
  Magnum::Vector3 ref_up;
  switch (frame)
  {
  case XYZ:
    ref_side = { 1, 0, 0 };
    ref_dir = { 0, 1, 0 };
    ref_up = { 0, 0, 1 };
    break;
  case XZYNeg:
    ref_side = { 1, 0, 0 };
    ref_dir = { 0, 0, 1 };
    ref_up = { 0, -1, 0 };
    break;
  case YXZNeg:
    ref_side = { 0, 1, 0 };
    ref_dir = { 1, 0, 0 };
    ref_up = { 0, 0, -1 };
    break;
  case YZX:
    ref_side = { 0, 1, 0 };
    ref_dir = { 0, 0, 1 };
    ref_up = { 1, 0, 0 };
    break;
  case ZXY:
    ref_side = { 0, 0, 1 };
    ref_dir = { 1, 0, 0 };
    ref_up = { 0, 1, 0 };
    break;
  case ZYXNeg:
    ref_side = { 0, 0, 1 };
    ref_dir = { 0, 1, 0 };
    ref_up = { -1, 0, 0 };
    break;
  case XYZNeg:
    ref_side = { 1, 0, 0 };
    ref_dir = { 0, 1, 0 };
    ref_up = { 0, 0, -1 };
    break;
  case XZY:
    ref_side = { 1, 0, 0 };
    ref_dir = { 0, 0, 1 };
    ref_up = { 0, 1, 0 };
    break;
  case YXZ:
    ref_side = { 0, 1, 0 };
    ref_dir = { 1, 0, 0 };
    ref_up = { 0, 0, 1 };
    break;
  case YZXNeg:
    ref_side = { 0, 1, 0 };
    ref_dir = { 0, 0, 1 };
    ref_up = { -1, 0, 0 };
    break;
  case ZXYNeg:
    ref_side = { 0, 0, 1 };
    ref_dir = { 1, 0, 0 };
    ref_up = { 0, -1, 0 };
    break;
  case ZYX:
    ref_side = { 0, 0, 1 };
    ref_dir = { 0, 1, 0 };
    ref_up = { -1, 0, 0 };
    break;
  default:
    break;
  }

  if (side)
  {
    *side = ref_side;
  }
  if (fwd)
  {
    *fwd = ref_dir;
  }
  if (up)
  {
    *up = ref_up;
  }
}


void Camera::calculatePitchYaw(const Magnum::Vector3 &camera_fwd, const Magnum::Vector3 &camera_up,
                               const Magnum::Vector3 &world_fwd, const Magnum::Vector3 &world_up,
                               float &pitch, float &yaw)
{
  // Pitch
  Magnum::Vector3 ref_fwd;
  auto fwd_up_dot = Magnum::Math::dot(camera_fwd, world_up);
  if (std::abs(std::abs(fwd_up_dot) - 1.0f) > 1e-6f)
  {
    // Calculate pitch as the angle between the camera and world forward vectors.
    // First calculate a world vector which is aligned with the camera forward.
    ref_fwd = Magnum::Math::cross(camera_fwd, world_up);
    ref_fwd = Magnum::Math::cross(world_up, camera_fwd);
    // We can just take the angle between them now.
    pitch = std::acos(Magnum::Math::dot(camera_fwd, ref_fwd));

    ref_fwd = camera_fwd;
  }
  else
  {
    // Edge case: forward ~ up
    // Pitch is 90 degrees.
    pitch = static_cast<Magnum::Float>(Magnum::Rad(Magnum::Deg(90.0f)));
    // We need to use the up vector to get the yaw as the pitch won't yield useful info.
    ref_fwd = camera_up;
  }
  pitch *= (fwd_up_dot > 0) ? -1.0f : 1.0f;

  // Yaw
  // Calculate yaw as the deviation between the reference forward and the camera forward projected
  // onto the plane perpendicular to the world up axis.
  fwd_up_dot = Magnum::Math::dot(ref_fwd, world_up);
  ref_fwd -= world_up * fwd_up_dot;
  ref_fwd = ref_fwd.normalized();

  yaw = std::acos(Magnum::Math::dot(ref_fwd, world_fwd));

  // Check the direction of rotation.
  const auto world_side = Magnum::Math::cross(world_fwd, world_up);
  if (Magnum::Math::dot(ref_fwd, world_side) < 0)
  {
    yaw *= -1.0f;
  }
}


void Camera::calculateCameraAxes(float pitch, float yaw, const Magnum::Vector3 &world_fwd,
                                 const Magnum::Vector3 &world_up, Magnum::Vector3 &camera_fwd,
                                 Magnum::Vector3 &camera_up)
{
  const auto transform = Magnum::Matrix4::rotation(Magnum::Rad(yaw), world_up) *  //
                         Magnum::Matrix4::rotation(Magnum::Rad(pitch), world_fwd);

  int fwd_axis = 0;
  int up_axis = 0;
  bool negate_fwd = false;
  bool negate_up = false;

  for (int i = 1; i < 3; ++i)
  {
    if (world_fwd[i] != 0)
    {
      fwd_axis = i;
      negate_fwd = world_fwd[i] < 0;
    }
    if (world_up[i] != 0)
    {
      up_axis = i;
      negate_up = world_up[i] < 0;
    }
  }

  camera_fwd = transform[fwd_axis].xyz();
  camera_up = transform[up_axis].xyz();

  camera_fwd *= Magnum::Float((negate_fwd) ? -1 : 1);
  camera_up *= Magnum::Float((negate_up) ? -1 : 1);
}
}  // namespace tes::view::handler
