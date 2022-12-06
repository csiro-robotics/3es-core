//
// Author: Kazys Stepanas
//
#ifndef TES_VIEWER_HANDLER_CAMERA_H
#define TES_VIEWER_HANDLER_CAMERA_H

#include "3es-viewer.h"

#include "3esmessage.h"
#include "camera/3escamera.h"

#include <3esmessages.h>

#include <array>
#include <limits>
#include <mutex>
#include <vector>

namespace tes::viewer::handler
{
class Camera : public Message
{
public:
  using CameraId = uint8_t;

  Camera();

  /// Enumerate all valid camera ids into @p camera_ids.
  /// @param camera_ids Container to enumerate into. Cleared before adding items.
  /// @return The number of cameras enumerated.
  size_t enumerate(std::vector<CameraId> &camera_ids) const;

  bool lookup(CameraId camera_id, tes::camera::Camera &camera) const;

  void initialise() override;
  void reset() override;
  void updateServerInfo(const ServerInfoMessage &info) override;
  void beginFrame(const FrameStamp &stamp) override;
  void endFrame(const FrameStamp &stamp) override;
  void draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params) override;
  void readMessage(PacketReader &reader) override;
  void serialise(Connection &out, ServerInfoMessage &info) override;

  static void getWorldAxes(tes::CoordinateFrame frame, Magnum::Vector3 *side, Magnum::Vector3 *fwd,
                           Magnum::Vector3 *up);

  /// Calculate the camera @p pitch and @p yaw given camera axes and world axes.
  ///
  /// The fwd/up axis pairs must be unit length and perpendicular.
  ///
  /// @param camera_fwd The camera forward axis.
  /// @param camera_up The camera up axis.
  /// @param world_fwd The world forward axis.
  /// @param world_up The world up axis.
  /// @param[out] pitch Output pitch value (radians).
  /// @param[out] yaw Output yaw value (radians).
  static void calculatePitchYaw(const Magnum::Vector3 &camera_fwd, const Magnum::Vector3 &camera_up,
                                const Magnum::Vector3 &world_fwd, const Magnum::Vector3 &world_up, float &pitch,
                                float &yaw);

  /// Calculate camera forward and up axes based on pitch and yaw values.
  /// @param pitch The camera pitch (radians).
  /// @param yaw The camera yaw (radians).
  /// @param world_fwd The world forward axis.
  /// @param world_up The world up axis.
  /// @param[out] camera_fwd Output forward axis.
  /// @param[out] camera_up Output up axis.
  static void calculateCameraAxes(float pitch, float yaw, const Magnum::Vector3 &world_fwd,
                                  const Magnum::Vector3 &world_up, Magnum::Vector3 &camera_fwd,
                                  Magnum::Vector3 &camera_up);

private:
  mutable std::mutex _mutex;
  /// Array of cameras. The boolean indicates the validity of the entry.
  using CameraEntry = std::pair<tes::camera::Camera, bool>;
  using CameraSet = std::array<CameraEntry, std::numeric_limits<uint8_t>::max()>;
  /// Main thread camera state.
  CameraSet _cameras;
  /// Pending thread camera state for next @c beginFrame().
  CameraSet _pending_cameras;
  ServerInfoMessage _server_info = {};
};
}  // namespace tes::viewer::handler

#endif  // TES_VIEWER_HANDLER_CAMERA_H
