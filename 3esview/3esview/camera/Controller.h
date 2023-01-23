#ifndef TES_VIEWER_CAMERA_CONTROLLER_H
#define TES_VIEWER_CAMERA_CONTROLLER_H

#include <3esview/ViewConfig.h>

#include "Camera.h"

namespace tes::camera
{
/// Base class for camera update.
class TES_VIEWER_API Controller
{
public:
  /// Control flags.
  enum class Flag : unsigned
  {
    Zero = 0,
    InvertKeyMoveX = (1 << 0u),
    InvertKeyMoveY = (1 << 1u),
    InvertKeyMoveZ = (1 << 2u),
    InvertKeyRotateX = (1 << 3u),
    InvertKeyRotateY = (1 << 4u),
    InvertKeyRotateZ = (1 << 5u),
    InvertMouseX = (1 << 6u),
    InvertMouseY = (1 << 7u),
    InvertKeyX = InvertKeyMoveX | InvertKeyRotateX,
    InvertKeyY = InvertKeyMoveY | InvertKeyRotateY,
    InvertKeyZ = InvertKeyMoveZ | InvertKeyRotateZ,
  };

  /// Default constructor.
  Controller();
  /// Virtual destructor.
  virtual ~Controller();

  /// Get the current control flags.
  inline Flag flags() const { return _flags; }
  /// Set the current control flags.
  inline void setFlags(Flag flags) { _flags = flags; }

  /// Set the given control flag(s). The @p flag value may contain multiple flags, but is generally expected to be a
  /// single flag.
  inline void set(Flag flag) { _flags = Flag(unsigned(_flags) | unsigned(flag)); }
  /// Clear the given control flag(s). The @p flag value may contain multiple flags, but is generally expected to be a
  /// single flag.
  inline void clear(Flag flag) { _flags = Flag(unsigned(_flags) & ~unsigned(flag)); }
  /// Check fi the given control flags are set. The @p flag value may contain multiple flags, but the return value is
  /// only true if *all* flag bits are set.
  inline bool isSet(Flag flag) const { return (unsigned(_flags) & unsigned(flag)) == unsigned(flag); }

  /// Perform mouse movement update logic.
  /// @param dx Mouse movement delta in x.
  /// @param dy Mouse movement delta in y.
  /// @param camera Camera to update.
  virtual void updateMouse(float dx, float dy, Camera &camera) = 0;

  /// Perform keyboard camera control update logic.
  /// @param dt Time delta (seconds).
  /// @param translate Current translation to apply per axis.
  /// @param rotate Current rotation to apply per axis.
  /// @param camera Camera to update. Only uses X and Y; X => pitch, Y => yaw.
  virtual void updateKeys(float dt, Magnum::Vector3i translate, Magnum::Vector3i rotate, Camera &camera) = 0;

  /// @overload
  void updateKeys(float dt, Magnum::Vector3i translate, Camera &camera)
  {
    updateKeys(dt, translate, Magnum::Vector3i(0), camera);
  }

  inline static void clampRotation(Camera &camera)
  {
    camera.pitch = std::max(-float(0.5 * M_PI), std::min(camera.pitch, float(0.5 * M_PI)));
    while (camera.yaw >= float(2.0 * M_PI))
    {
      camera.yaw -= float(2.0 * M_PI);
    }
    while (camera.yaw < 0.0f)
    {
      camera.yaw += float(2.0 * M_PI);
    }
  }

protected:
  Flag _flags = Flag::Zero;
};
}  // namespace tes::camera

#endif  // TES_VIEWER_CAMERA_CONTROLLER_H
