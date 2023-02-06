//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SPIN_LOCK_H
#define TES_CORE_SPIN_LOCK_H

#include "CoreConfig.h"

#include <memory>

namespace tes
{
struct SpinLockImp;

/// A spin lock implementation. Prefered over std::mutex as that class
/// can be very slow (i.e, Clang OSX).
///
/// This is a naive implementation and does not support re-locking.
///
/// Best used with @c std::unique_lock as an exception and scope safe guard.
class TES_CORE_API SpinLock
{
public:
  /// Construct a spin lock (unlocked).
  SpinLock();
  /// Destructor.
  ~SpinLock();

  /// Block until the spin lock can be attained.
  void lock();

  /// Try attain the lock without blocking.
  /// @return True if the lock is attained, false if it could not be attained.
  [[nodiscard]] bool tryLock();

  /// Alias for @c tryLock().
  /// @return True if the lock is attained, false if it could not be attained.
  [[nodiscard]] bool try_lock();  // NOLINT(readability-identifier-naming)

  /// Unlock the lock. Should only ever be called by the scope which called @c lock()
  /// or succeeded at @c try_lock.
  void unlock();

private:
  std::unique_ptr<SpinLockImp> _imp;  ///< Implementation detail.
};

inline bool SpinLock::try_lock()
{
  return tryLock();
}
}  // namespace tes

#endif  // TES_CORE_SPIN_LOCK_H
