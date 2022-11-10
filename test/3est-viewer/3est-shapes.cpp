//
// author: Kazys Stepanas
//

#include <3es-viewer/painter/3esbox.h>
#include <3es-viewer/3esviewer.h>

#include <gtest/gtest.h>

#include <chrono>
#include <iostream>
#include <list>
#include <mutex>
#include <random>
#include <thread>
#include <vector>


namespace tes::viewer
{
TEST(Shapes, Painter_WindowSimple)
{
  int argc = 1;
  std::string arg = "test";
  char *arg_ptr = arg.data();
  char **argv = &arg_ptr;
  Viewer viewer(Magnum::Platform::GlfwApplication::Arguments(argc, argv));
  // Make sure our viewable window works in the simple case:
  // - add shapes for N frames
  // - keep a window W where W < N
  // - make sure the window is always valid
  // - make sure expired shapes are not valid.
  auto culler = std::make_shared<BoundsCuller>();
  painter::Box painter(culler);

  const FrameNumber max_frames = frameWindow() + 10u;
  const FrameNumber window = 10u;

  FrameStamp stamp = {};
  const tes::Id id(1);
  Magnum::Matrix4 transform = {};
  Magnum::Color4 colour = {};

  for (stamp.frame_number = 0; stamp.frame_number < max_frames; ++stamp.frame_number)
  {
    transform = Magnum::Matrix4::translation(Magnum::Vector3(stamp.frame_number, 0, 0));
    colour = Magnum::Color4(stamp.frame_number);
    // Update a shape.
    if (stamp.frame_number > 0)
    {
      painter.update(id, stamp.frame_number, transform, colour);
    }
    else
    {
      painter.add(id, stamp.frame_number, painter::ShapePainter::Type::Solid, transform, colour);
    }
    painter.endFrame(stamp.frame_number);

    // Check the window.
    const FrameNumber frame_offset = frameWindow() - 1;
    for (unsigned i = std::max(stamp.frame_number, frame_offset) - frame_offset; i <= stamp.frame_number; ++i)
    {
      const FrameNumber frame = stamp.frame_number - i;
      if (painter.readProperties(id, i, transform, colour))
      {
        EXPECT_NEAR(colour.r(), Magnum::Float(i), Magnum::Float(1e-4));
        EXPECT_NEAR(transform[3].x(), Magnum::Float(i), Magnum::Float(1e-4));
      }
      else
      {
        FAIL();
      }
    }

    // Ensure we've expired outside the window.
    if (stamp.frame_number >= frameWindow())
    {
      EXPECT_FALSE(painter.readProperties(id, stamp.frame_number - frameWindow(), transform, colour));
    }
  }
}
}  // namespace tes::viewer
