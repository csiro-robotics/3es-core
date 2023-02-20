#ifndef TES_VIEW_UTIL_PENDING_ACTION_QUEUE_H
#define TES_VIEW_UTIL_PENDING_ACTION_QUEUE_H

#include <3esview/ViewConfig.h>

#include "PendingQueue.h"

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
/// We have to queue actions from the background thread into the same queue to preserve the order
/// of operations. This struct provides the simplest way to amalgamate the available actions into
/// a single queue. Data for all actions are present.
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

  uint32_t shape_id = 0;     ///< ID of the shape to affect. Used for all actions.
  Kind action = Kind::None;  ///< The action type.
  Create create;
  Update update;
  Destroy destroy;

  PendingAction() = default;
  PendingAction(Kind action)
    : action(action){};
  PendingAction(const PendingAction &other) = default;
  PendingAction(PendingAction &&other) noexcept = default;
  PendingAction &operator=(const PendingAction &other) = default;
  PendingAction &operator=(PendingAction &&other) noexcept = default;
};

template <typename Shape>
class PendingActionQueue : public PendingQueue<PendingAction<Shape>>
{
public:
  using Action = typename PendingAction<Shape>;
  using ActionKind = typename Action::Kind;
};
}  // namespace tes::view::util

#endif  // TES_VIEW_UTIL_PENDING_ACTION_QUEUE_H
