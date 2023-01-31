#include "TestViewer.h"

#include <3escore/Server.h>

#include <3esview/ThirdEyeScene.h>

#include <3esview/data/NetworkThread.h>
#include <3esview/data/StreamThread.h>

#include <Magnum/Math/Vector2.h>

#include <fstream>

namespace tes::view
{
uint16_t TestViewer::defaultPort()
{
  return ServerSettings().listenPort;
}

TestViewer::TestViewer(const Arguments &arguments)
  : TestViewer{ arguments, Configuration{} }
{}


TestViewer::TestViewer(const Arguments &arguments, const Configuration &configuration)
  : Application{ arguments, configuration }
  , _tes(std::make_shared<ThirdEyeScene>())
{}


TestViewer::~TestViewer()
{
  closeOrDisconnect();
}


bool TestViewer::open(const std::filesystem::path &path)
{
  closeOrDisconnect();
  std::ifstream file(path.string(), std::ios::binary);
  if (!file.is_open())
  {
    return false;
  }

  std::scoped_lock guard(_mutex);
  _data_thread = std::make_shared<StreamThread>(_tes, std::make_shared<std::ifstream>(std::move(file)));
  // Do not allow looping in the windowless/test context.
  _data_thread->setLooping(false);
  return true;
}


bool TestViewer::connect(const std::string &host, uint16_t port)
{
  closeOrDisconnect();
  std::scoped_lock guard(_mutex);
  // Do not allow auto reconnect in the windowless/test context.
  auto net_thread = std::make_shared<NetworkThread>(_tes, host, port, false);
  _data_thread = net_thread;
  // Reconnection not allowed. Wait until the network thread has tried to connect...
  const auto start_time = std::chrono::steady_clock::now();
  // ...but don't wait forever.
  const auto timeout = std::chrono::seconds(5);
  while (!net_thread->connectionAttempted() && (std::chrono::steady_clock::now() - start_time) < timeout)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return net_thread->connected();
}


bool TestViewer::closeOrDisconnect()
{
  std::scoped_lock guard(_mutex);
  if (_data_thread)
  {
    _data_thread->stop();
    _data_thread->join();
    _data_thread = nullptr;
    return true;
  }
  return false;
}


bool TestViewer::shouldQuit()
{
  std::scoped_lock guard(_mutex);
  // Check the render mark, not the frame number. We may not be increasing the frame number.
  if (_quit_after_frames.has_value() && _tes->frameStamp().render_mark >= *_quit_after_frames)
  {
    return true;
  }

  if (_end_time.has_value())
  {
    const auto now = Clock::now();
    if (now >= *_end_time)
    {
      return true;
    }
  }

  return false;
}


int TestViewer::run(const FrameFunction &frame_function)
{
  _frame_function = frame_function;
  return exec();
}


int TestViewer::exec()
{
  using namespace Magnum::Math::Literals;

  auto last_sim_time = Clock::now();

  bool can_continue = true;
  do
  {
    const auto now = Clock::now();
    const auto delta_time = now - last_sim_time;
    const float dt = std::chrono::duration_cast<std::chrono::duration<float>>(delta_time).count();
    if (_frame_function)
    {
      can_continue = _frame_function();
    }
    _tes->render(dt, Magnum::Vector2i(1024, 768));
    last_sim_time = now;
  } while (!shouldQuit() || !can_continue);

  return 0;
}
}  // namespace tes::view
