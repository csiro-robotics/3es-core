//
// Author: Kazys Stepanas
//
#include "3escamera.h"

#include <3eslog.h>
#include <3escoordinateframe.h>
#include <3esconnection.h>
#include <3espacketwriter.h>

#include <Magnum/Math/Vector3.h>

#include <array>

namespace tes::viewer::handler
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
  for (auto &camera : _pending_cameras)
  {
    camera.second = false;
  }
}


void Camera::updateServerInfo(const ServerInfoMessage &info)
{}


void Camera::beginFrame(const FrameStamp &stamp)
{
  std::lock_guard guard(_mutex);
  std::copy(_pending_cameras.begin(), _pending_cameras.end(), _cameras.begin());
}


void Camera::endFrame(const FrameStamp &stamp)
{}


void Camera::draw(DrawPass pass, const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix)
{}


void Camera::readMessage(PacketReader &reader)
{
  bool ok = false;

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
  camera.fov_horizontal = msg.fov;
  camera.frame = tes::CoordinateFrame(_server_info.coordinateFrame);

  // Determine pitch and yaw by a deviation from the expected axis.
  Magnum::Vector3 ref_dir;
  Magnum::Vector3 ref_up;
  switch (camera.frame)
  {
  case XYZ:
    ref_dir = { 1, 0, 0 };
    ref_up = { 0, 0, 1 };
    break;
  case XZYNeg:
    ref_dir = { 1, 0, 0 };
    ref_up = { 0, -1, 0 };
    break;
  case YXZNeg:
    ref_dir = { 0, 1, 0 };
    ref_up = { 0, 0, -1 };
    break;
  case YZX:
    ref_dir = { 0, 1, 0 };
    ref_up = { 1, 0, 0 };
    break;
  case ZXY:
    ref_dir = { 0, 0, 1 };
    ref_up = { 0, 1, 0 };
    break;
  case ZYXNeg:
    ref_dir = { 0, 0, 1 };
    ref_up = { -1, 0, 0 };
    break;
  case XYZNeg:
    ref_dir = { 1, 0, 0 };
    ref_up = { 0, 0, -1 };
    break;
  case XZY:
    ref_dir = { 1, 0, 0 };
    ref_up = { 0, 1, 0 };
    break;
  case YXZ:
    ref_dir = { 0, 1, 0 };
    ref_up = { 0, 0, 1 };
    break;
  case YZXNeg:
    ref_dir = { 0, 1, 0 };
    ref_up = { -1, 0, 0 };
    break;
  case ZXYNeg:
    ref_dir = { 0, 0, 1 };
    ref_up = { 0, -1, 0 };
    break;
  case ZYX:
    ref_dir = { 0, 0, 1 };
    ref_up = { -1, 0, 0 };
    break;
  }

  // FIXME(KS): nope
  camera.yaw = std::acos(Magnum::Math::dot(ref_dir, Magnum::Vector3(msg.dirX, msg.dirY, msg.dirZ)));
  camera.pitch = std::acos(Magnum::Math::dot(ref_dir, Magnum::Vector3(msg.upX, msg.upY, msg.upZ)));

  std::lock_guard guard(_mutex);
  _pending_cameras[msg.cameraId] = std::make_pair(camera, true);
}


void Camera::serialise(Connection &out, ServerInfoMessage &info)
{
  std::lock_guard guard(_mutex);
  CameraMessage msg = {};
  const std::string error_str = "<error>";
  bool ok = true;

  std::vector<uint8_t> packet_buffer(1024u, 0u);
  PacketWriter writer(packet_buffer.data(), packet_buffer.size());
  for (size_t i = 0; i < _cameras.size(); ++i)
  {
    if (!_cameras[i].second)
    {
      // Not valid.
      continue;
    }
    const auto camera = _cameras[i].first;
    msg.cameraId = uint8_t(i);
    msg.flags = 0;
    msg.reserved = 0;

    msg.x = camera.position.x();
    msg.y = camera.position.y();
    msg.z = camera.position.z();

    msg.near = camera.clip_near;
    msg.far = camera.clip_far;
    msg.fov = camera.fov_horizontal;

    // TODO(KS):
    msg.dirX = 1;
    msg.dirY = 0;
    msg.dirZ = 0;

    msg.upX = 0;
    msg.upY = 0;
    msg.upZ = 1;

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

}  // namespace tes::viewer::handler
