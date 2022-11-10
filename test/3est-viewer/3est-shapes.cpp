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
/// Initialise a viewer for use in unit tests. Test which use any of the 3D graphics API require a viewer first.
/// @note This will not be thread safe using OpenGL. It may with Vulkan - untested.
/// @return A new viewer.
std::unique_ptr<Viewer> initViewer()
{
  int argc = 1;
  std::string arg = "test";
  char *arg_ptr = arg.data();
  char **argv = &arg_ptr;
  return std::make_unique<Viewer>(Magnum::Platform::GlfwApplication::Arguments(argc, argv));
}

TEST(Shapes, Painter_Parents)
{
  // Test creating a shapes with a parent;
  // - Basic parenting affecting transformations.
  // - Updating a parent affects children.
  // We only adjust translation, with children ranging in x and the parent moving in y. Children also move in z each
  // frame.
  auto viewer = initViewer();
  painter::Box painter(viewer->culler());

  FrameStamp stamp = {};
  const Id id(1);
  Magnum::Matrix4 transform = {};
  Magnum::Color4 colour = Magnum::Color4(0.5f);

  // Start with an identity transform for the parent.
  auto parent_id = painter.add(id, stamp.frame_number, painter::Box::Type::Solid, transform, colour);

  // Add some children.
  const unsigned child_count = 10;
  for (unsigned i = 0; i < child_count; ++i)
  {
    transform = Magnum::Matrix4::translation({ Magnum::Float(i), 0, 0 });
    painter.addChild(parent_id, stamp.frame_number, painter::Box::Type::Solid, transform, colour);
  }

  // Validation function. Ensures each child ranges in x by its child number, while all shapes range in y by the frame
  // number. We check all frames up to the given frame number.
  const auto validate_shapes = [id, parent_id, &painter, child_count](const FrameNumber frame) {
    for (FrameNumber f = 0; f < frame; ++f)
    {
      const float expect_y = f;
      float expect_x = 0;
      float expect_z = 0;

      Magnum::Matrix4 transform = {};
      Magnum::Color4 colour = {};

      // Check the parent.
      painter.readShape(id, f, transform, colour);
      auto pos = transform[3].xyz();
      const float epsilon = 1e-5;
      EXPECT_NEAR(pos.x(), expect_x, epsilon);
      EXPECT_NEAR(pos.y(), expect_y, epsilon);
      EXPECT_NEAR(pos.z(), expect_z, epsilon);

      // Children move each frame.
      expect_z = f;
      for (unsigned i = 0; i < child_count; ++i)
      {
        // Check child.
        expect_x = i;
        // Read without parent transform.
        painter.readChildShape(painter::ShapePainter::ChildId(id, i), f, false, transform, colour);
        pos = transform[3].xyz();
        const float epsilon = 1e-5;
        EXPECT_NEAR(pos.x(), expect_x, epsilon);
        EXPECT_NEAR(pos.y(), 0, epsilon);
        EXPECT_NEAR(pos.z(), 0.0f, epsilon);
        // Read with parent transform.
        painter.readChildShape(painter::ShapePainter::ChildId(id, i), f, true, transform, colour);
        pos = transform[3].xyz();
        EXPECT_NEAR(pos.x(), expect_x, epsilon);
        EXPECT_NEAR(pos.y(), 0, epsilon);
        EXPECT_NEAR(pos.z(), 0.0f, epsilon);
      }
    }
  };

  validate_shapes(stamp.frame_number);

  // Check ea
  for (stamp.frame_number = 1; stamp.frame_number < 100; ++stamp.frame_number)
  {
    // Update for next frame.
    // Parent update
    transform = Magnum::Matrix4::translation({ 0, Magnum::Float(stamp.frame_number), 0 });
    painter.update(id, stamp.frame_number, transform, colour);

    // Child update.
    for (unsigned i = 0; i < child_count; ++i)
    {
      painter::ShapePainter::ChildId child_id(id, i);
      transform = Magnum::Matrix4::translation({ Magnum::Float(i), 0, Magnum::Float(stamp.frame_number) });
      painter.update(id, stamp.frame_number, transform, colour);
    }

    // Validate
    validate_shapes(stamp.frame_number);
  }
}


TEST(Shapes, Painter_WindowSimple)
{
  // Make sure our viewable window works in the simple case:
  // - add shapes for N frames
  // - keep a window W where W < N
  // - make sure the window is always valid
  // - make sure expired shapes are not valid.
  auto viewer = initViewer();
  painter::Box painter(viewer->culler());

  const FrameNumber max_frames = frameWindow() + 10u;
  const FrameNumber window = 10u;

  FrameStamp stamp = {};
  const Id id(1);
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
      if (painter.readShape(id, i, transform, colour))
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
      EXPECT_FALSE(painter.readShape(id, stamp.frame_number - frameWindow(), transform, colour));
    }
  }
}

TEST(Shapes, Painter_WindowParents)
{
  auto viewer = initViewer();
}
}  // namespace tes::viewer
