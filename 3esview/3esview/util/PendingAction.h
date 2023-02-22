#ifndef TES_VIEW_UTIL_PENDING_ACTION_QUEUE_H
#define TES_VIEW_UTIL_PENDING_ACTION_QUEUE_H

#include <3esview/ViewConfig.h>

#include <3escore/Colour.h>
#include <3escore/Quaternion.h>
#include <3escore/Vector3.h>

namespace tes::view::util
{
/// The action type for a @c PendingAction .
enum class ActionKind : uint32_t
{
  None,
  Create,
  Update,
  Destroy
};

/// Details for a pending actions queued by the background thread to effect in the main thread.
///
/// Message and shape handlers must process messages for the upcoming frame, but not effect those
/// changes until the frame completes - until @c handlers::Message::endFrame() is called. The
/// @c PendingAction provides a data structure which can be added to a vector (or queue) to track
/// actions to be effected on the next @c handlers::Message::endFrame() call. We typically expect
/// to only enqueue create, update and destroy actions, as determined by @c ActionKind , as data
/// messages always occur after create, but before the next frame.
///
/// This structure contains data for all three potential message types, with the @c kind member
/// identifying which data section is currently relevant.
///
/// While this union (mathematical terminology rather than C++ @c union ) of types is somewhat
/// wasteful memory-wise, it is a very simple way to amalgamate the available actions into a single
/// queue.
///
/// Note the @c Create is made up of the template @c Shape type. This will generally be either a
/// @c shared_ptr to the @c tes::Shape type or value type thereof, depending on class size and
/// lifetime.
template <typename Shape>
struct PendingAction
{
  /// Alias @c ActionKind .
  using Kind = ActionKind;

  /// The shape for a create action. Valid with @c PendingAction::Create .
  struct Create
  {
    Shape shape;
  };

  /// Data for an update action. Valid with @c PendingAction::Update .
  ///
  /// We can only update the transform (partial or full) and colour. The @c flags indicate what
  /// to update - see @c tes::UpdateFlag .
  struct Update
  {
    Vector3d position;     ///< New position if @c UFPosition is set.
    Quaterniond rotation;  ///< New rotation if @c UFRotation is set.
    Vector3d scale;        ///< New scale if @c UFScale is set.
    Colour colour;         ///< New colour if @c UFColour is set.
    unsigned flags = 0;    ///< Flags indicating what to update - see @c tes::UpdateFlag .
  };

  /// Details specific to a destroy action: @c PendingAction::Destroy .
  struct Destroy
  {
  };

  uint32_t shape_id = 0;   ///< ID of the shape to affect. Used for all actions.
  Kind kind = Kind::None;  ///< The action type.
  Create create;
  Update update;
  Destroy destroy;

  PendingAction() = default;
  PendingAction(Kind kind)
    : kind(kind){};
  PendingAction(const PendingAction &other) = default;
  PendingAction(PendingAction &&other) noexcept = default;
  PendingAction &operator=(const PendingAction &other) = default;
  PendingAction &operator=(PendingAction &&other) noexcept = default;
};
}  // namespace tes::view::util

#endif  // TES_VIEW_UTIL_PENDING_ACTION_QUEUE_H
