//
// author: Kazys Stepanas
//
#include "SpinLock.h"

#include <atomic>
#include <thread>

namespace tes
{
struct SpinLockImp
{
  std::atomic_bool lock;

  inline SpinLockImp()
    : lock(false)
  {}
};

SpinLock::SpinLock()
  : _imp(std::make_unique<SpinLockImp>())
{}


SpinLock::~SpinLock() = default;


void SpinLock::lock()
{
  while (_imp->lock.exchange(true))
  {
    std::this_thread::yield();
  }
}


bool SpinLock::tryLock()
{
  return !_imp->lock.exchange(true);
}


void SpinLock::unlock()
{
  _imp->lock = false;
}
}  // namespace tes
