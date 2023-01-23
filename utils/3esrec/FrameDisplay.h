#ifndef _3ESFRAMEDISPLAY_H
#define _3ESFRAMEDISPLAY_H

#include <atomic>
#include <cinttypes>
#include <memory>
#include <thread>

namespace tes
{
/// Secondary thread for displaying frame progress. Use start()/stop() to manage the thread.
class FrameDisplay
{
public:
  /// Constructor
  FrameDisplay();

  /// Destructor: attempt to ensure thread terminates cleanly.
  ~FrameDisplay();

  /// Increment the current frame value by 1.
  void incrementFrame() { ++_frameNumber; }

  /// Increment the current frame by a given value.
  /// @param increment The increment to add.
  void incrementFrame(int64_t increment) { _frameNumber += increment; }

  /// Reset frame number to zero.
  void reset() { _frameNumber.store(0); }

  /// Start the display thread. Ignored if already running.
  void start();

  /// Stop the display thread. Ok to call when not running.
  void stop();

private:
  /// Thread loop.
  void run();

  std::atomic_int64_t _frameNumber;
  std::atomic_bool _quit;
  std::unique_ptr<std::thread> _thread;
};
}  // namespace tes

#endif  // _3ESFRAMEDISPLAY_H
