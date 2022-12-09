#ifndef TES_VIEWER_EDL_EFFECT_H
#define TES_VIEWER_EDL_EFFECT_H

#include "3es-viewer.h"

#include "3esfboeffect.h"

#include <Magnum/Math/Range.h>
#include <Magnum/Math/Vector2.h>

#include <memory>

namespace tes::viewer
{
struct EdlEffectDetail;

/// Eye dome lighting frame buffer effect.
class TES_VIEWER_API EdlEffect : public FboEffect
{
public:
  EdlEffect(const Magnum::Range2Di &viewport = { Magnum::Vector2i(0), Magnum::Vector2i(1) });
  ~EdlEffect();

  void setRadius(float radius);
  float radius() const;
  void setLinearScale(float linear_scale);
  float linearScale() const;
  void setExponentialScale(float exponential_scale);
  float exponentialScale() const;
  void setLightDirection(const Magnum::Vector3 &light_direction);
  const Magnum::Vector3 &lightDirection() const;

  /// Prepare for rendering the frame buffer effect.
  /// @param projection_matrix The projection matrix.
  /// @param projection_type Identifies the type of projection.
  /// @param view_size The size of the target viewport in pixels.
  /// @param near_clip The near clip plane distance.
  /// @param far_clip The far clip plane distance.
  void prepareFrame(const Magnum::Matrix4 &projection_matrix, ProjectionType projection_type, float near_clip,
                    float far_clip) override;

  /// Complete rendering of the frame. This must blit back to the active frame buffer, normally the default.
  void completeFrame() override;

  /// Called when the viewport changes. Allows the frame buffer to resize if required.
  /// @param viewport The new viewport dimensions.
  void viewportChange(const Magnum::Range2Di &viewport) override;

private:
  void makeBuffers(const Magnum::Range2Di &viewport);

  std::unique_ptr<EdlEffectDetail> _imp;
};
}  // namespace tes::viewer

#endif  // TES_VIEWER_EDL_EFFECT_H
