//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SPIN_LOCK_H
#define TES_CORE_SPIN_LOCK_H

#include "CoreConfig.h"

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
  bool try_lock();

  /// Unlock the lock. Should only ever be called by the scope which called @c lock()
  /// or succeeded at @c try_lock.
  void unlock();

private:
  SpinLockImp *_imp;  ///< Implementation detail.
};
}  // namespace tes

#endif  // TES_CORE_SPIN_LOCK_H
