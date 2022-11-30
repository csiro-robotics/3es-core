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
  void draw(DrawPass pass, const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix) override;
  void readMessage(PacketReader &reader) override;
  void serialise(Connection &out, ServerInfoMessage &info) override;

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
