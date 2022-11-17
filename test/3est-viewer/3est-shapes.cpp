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
class Shapes : public testing::Test
{
public:
  /// Initialise a viewer for use in unit tests. Test which use any of the 3D graphics API require a viewer first.
  /// @note This will not be thread safe using OpenGL. It may with Vulkan - untested.
  void SetUp()
  {
    int argc = 1;
    std::string arg = "test";
    char *arg_ptr = arg.data();
    char **argv = &arg_ptr;
    _viewer = std::make_unique<Viewer>(Magnum::Platform::GlfwApplication::Arguments(argc, argv));
  }

  void TearDown() { _viewer.release(); }

  Viewer &viewer() { return *_viewer; }
  const Viewer &viewer() const { return *_viewer; }

private:
  std::unique_ptr<Viewer> _viewer;
};

/// A helper class for running painter parent shape tests.
///
/// The test starts by allocating a shape with @p child_count children. The test simulates updating the children for
/// @p frame_count, adjusting the parent and child positions each frame. The position of each shape is filled with
/// information about the current frame and the child id. We then validate the position for each frame so far, within
/// the overall @c frameWindow(). If @c frame_count exceeds the frame window, then we also validate there is no
/// transform information outside the window.
/// @tparam Painter The @c painter::ShapePainter instance to simulate swith.
template <typename Painter>
struct ParentsTest
{
  /// Number of children to allocate.
  unsigned child_count = 10;
  /// Number of frames to simulate.
  unsigned frame_count = 100;

  /// Run the test.
  /// @param viewer The viewer framework.
  void run(Viewer &viewer)
  {
    _painter = std::make_unique<Painter>(viewer.culler());

    Magnum::Matrix4 transform = {};
    Magnum::Color4 colour = Magnum::Color4(0.5f);

    // Start with an identity transform for the parent.
    auto parent_id = _painter->add(_shape_id, _stamp.frame_number, painter::Box::Type::Solid, transform, colour);

    // Add some children.
    for (unsigned i = 0; i < child_count; ++i)
    {
      transform = Magnum::Matrix4::translation({ Magnum::Float(i), 0, 0 });
      _painter->addChild(parent_id, _stamp.frame_number, painter::Box::Type::Solid, transform, colour);
    }

    _painter->endFrame(_stamp.frame_number);
    validate(_stamp.frame_number);

    // Run a series of frames where we update the parent, then the children and validate the transforms.
    for (_stamp.frame_number = 1; _stamp.frame_number < frame_count; ++_stamp.frame_number)
    {
      // Update for next frame.
      // Parent update
      transform = Magnum::Matrix4::translation({ 0, Magnum::Float(_stamp.frame_number), 0 });
      _painter->update(_shape_id, _stamp.frame_number, transform, colour);

      // Child update.
      for (unsigned i = 0; i < child_count; ++i)
      {
        painter::ShapePainter::ChildId child_id(_shape_id, i);
        transform = Magnum::Matrix4::translation({ Magnum::Float(i), 0, Magnum::Float(_stamp.frame_number) });
        _painter->updateChildShape(child_id, _stamp.frame_number, transform, colour);
      }

      _painter->endFrame(_stamp.frame_number);
      validate(_stamp.frame_number);
    }

    // Validate shape removal and expiry.
    ++_stamp.frame_number;
    _painter->remove(_shape_id, _stamp.frame_number);
    _painter->endFrame(_stamp.frame_number);
    validateExpired(_stamp.frame_number);
    // Validate the frame before still has data.
    --_stamp.frame_number;
    validate(_stamp.frame_number);
    // End a frame beyond the window and ensure we have nothing valid left.
    _painter->endFrame(_stamp.frame_number + frameWindow());
    validateExpired(_stamp.frame_number);

    _painter.release();
  }

private:
  /// Perform validation. Uses GTest macros to validate.
  void validate(FrameNumber frame_number)
  {
    Magnum::Matrix4 transform = {};
    Magnum::Color4 colour = {};

    const FrameNumber start_frame = (frame_number >= frameWindow()) ? frame_number - frameWindow() + 1 : 0u;
    for (FrameNumber f = start_frame; f <= frame_number; ++f)
    {
      const float expect_y = f;
      float expect_x = 0;
      float expect_z = 0;

      // Check the parent.
      _painter->readShape(_shape_id, f, transform, colour);
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
        _painter->readChildShape(painter::ShapePainter::ChildId(_shape_id, i), f, false, transform, colour);
        pos = transform[3].xyz();
        const float epsilon = 1e-5;
        EXPECT_NEAR(pos.x(), expect_x, epsilon);
        EXPECT_NEAR(pos.y(), 0, epsilon);
        EXPECT_NEAR(pos.z(), expect_z, epsilon);
        // Read with parent transform.
        _painter->readChildShape(painter::ShapePainter::ChildId(_shape_id, i), f, true, transform, colour);
        pos = transform[3].xyz();
        EXPECT_NEAR(pos.x(), expect_x, epsilon);
        EXPECT_NEAR(pos.y(), expect_y, epsilon);
        EXPECT_NEAR(pos.z(), expect_z, epsilon);
      }
    }

    // Ensure we have nothing valid just outside the window.
    if (frame_number >= frameWindow())
    {
      validateExpired(frame_number - frameWindow());
    }
  }

  void validateExpired(FrameNumber at_frame)
  {
    Magnum::Matrix4 transform = {};
    Magnum::Color4 colour = {};
    EXPECT_FALSE(_painter->readShape(_shape_id, at_frame - frameWindow(), transform, colour));
    for (unsigned i = 0; i < child_count; ++i)
    {
      EXPECT_FALSE(_painter->readChildShape(painter::ShapePainter::ChildId(_shape_id, i), at_frame - frameWindow(),
                                            false, transform, colour));
    }
  }

  FrameStamp _stamp = {};
  Id _shape_id = { 1 };
  std::unique_ptr<Painter> _painter;
};

TEST_F(Shapes, Painter_Add)
{
  painter::Box painter(viewer().culler());

  FrameNumber frame = 0;
  const Id id(1);
  const Magnum::Matrix4 transform = Magnum::Matrix4::translation({ 1, 2, 3 });
  const Magnum::Color4 colour = { 3, 2, 1, 0 };

  painter.add(Id(1), frame, painter::ShapePainter::Type::Solid, transform, colour);
  painter.add(Id(2), frame, painter::ShapePainter::Type::Transparent, transform, colour);
  painter.add(Id(3), frame, painter::ShapePainter::Type::Wireframe, transform, colour);

  // Assert we have shapes.
  Magnum::Matrix4 t = {};
  Magnum::Color4 c = {};
  EXPECT_TRUE(painter.readShape(Id(1), frame, t, c));
  EXPECT_EQ(t, transform);
  EXPECT_EQ(c, colour);
  EXPECT_TRUE(painter.readShape(Id(2), frame, t, c));
  EXPECT_EQ(t, transform);
  EXPECT_EQ(c, colour);
  EXPECT_TRUE(painter.readShape(Id(3), frame, t, c));
  EXPECT_EQ(t, transform);
  EXPECT_EQ(c, colour);
}

TEST_F(Shapes, Painter_Remove)
{
  painter::Box painter(viewer().culler());

  FrameNumber frame = 0;
  const Id id(1);
  const Magnum::Matrix4 transform = Magnum::Matrix4::translation({ 1, 2, 3 });
  const Magnum::Color4 colour = { 3, 2, 1, 0 };

  painter.add(Id(1), frame, painter::ShapePainter::Type::Solid, transform, colour);
  painter.add(Id(2), frame, painter::ShapePainter::Type::Transparent, transform, colour);
  painter.add(Id(3), frame, painter::ShapePainter::Type::Wireframe, transform, colour);

  // Assert we have shapes.
  Magnum::Matrix4 t = {};
  Magnum::Color4 c = {};
  EXPECT_TRUE(painter.readShape(Id(1), frame, t, c));
  EXPECT_EQ(t, transform);
  EXPECT_EQ(c, colour);
  EXPECT_TRUE(painter.readShape(Id(2), frame, t, c));
  EXPECT_EQ(t, transform);
  EXPECT_EQ(c, colour);
  EXPECT_TRUE(painter.readShape(Id(3), frame, t, c));
  EXPECT_EQ(t, transform);
  EXPECT_EQ(c, colour);

  // Remove the next frame.
  ++frame;
  EXPECT_TRUE(painter.remove(Id(1), frame));
  EXPECT_TRUE(painter.remove(Id(2), frame));
  EXPECT_TRUE(painter.remove(Id(3), frame));

  // Validate removal.
  EXPECT_FALSE(painter.readShape(Id(1), frame, t, c));
  EXPECT_FALSE(painter.readShape(Id(2), frame, t, c));
  EXPECT_FALSE(painter.readShape(Id(3), frame, t, c));
}

TEST_F(Shapes, Painter_ReAdd)
{
  // Validate we can add a shape, remove it, then add it again all in the same frame.
  // This isn't an expected use case, but it should not break.
  painter::Box painter(viewer().culler());

  FrameNumber frame = 1;
  const Id id(1);
  Magnum::Matrix4 transform = Magnum::Matrix4::translation({ 1, 2, 3 });
  Magnum::Color4 colour = { 3, 2, 1, 0 };

  painter.add(Id(1), frame, painter::ShapePainter::Type::Solid, transform, colour);
  painter.add(Id(2), frame, painter::ShapePainter::Type::Transparent, transform, colour);
  painter.add(Id(3), frame, painter::ShapePainter::Type::Wireframe, transform, colour);

  // Assert we have shapes.
  EXPECT_TRUE(painter.readShape(Id(1), frame, transform, colour));
  EXPECT_TRUE(painter.readShape(Id(2), frame, transform, colour));
  EXPECT_TRUE(painter.readShape(Id(3), frame, transform, colour));

  // Remove
  EXPECT_TRUE(painter.remove(Id(1), frame));
  EXPECT_TRUE(painter.remove(Id(2), frame));
  EXPECT_TRUE(painter.remove(Id(3), frame));

  // Validate removal.
  EXPECT_FALSE(painter.readShape(Id(1), frame, transform, colour));
  EXPECT_FALSE(painter.readShape(Id(2), frame, transform, colour));
  EXPECT_FALSE(painter.readShape(Id(3), frame, transform, colour));

  // Re add
  transform = Magnum::Matrix4::translation({ 4, 5, 6 });
  colour = Magnum::Color4{ 6, 5, 4, 3 };
  painter.add(Id(1), frame, painter::ShapePainter::Type::Solid, transform, colour);
  painter.add(Id(2), frame, painter::ShapePainter::Type::Transparent, transform, colour);
  painter.add(Id(3), frame, painter::ShapePainter::Type::Wireframe, transform, colour);

  // Validate re-add.
  Magnum::Matrix4 t = {};
  Magnum::Color4 c = {};
  EXPECT_TRUE(painter.readShape(Id(1), frame, t, c));
  EXPECT_EQ(t, transform);
  EXPECT_EQ(c, colour);
  EXPECT_TRUE(painter.readShape(Id(2), frame, t, c));
  EXPECT_EQ(t, transform);
  EXPECT_EQ(c, colour);
  EXPECT_TRUE(painter.readShape(Id(3), frame, t, c));
  EXPECT_EQ(t, transform);
  EXPECT_EQ(c, colour);
}

TEST_F(Shapes, Painter_Parents)
{
  // Test creating a shapes with a parent;
  // - Basic parenting affecting transformations.
  // - Updating a parent affects children.
  // We only adjust translation, with children ranging in x and the parent moving in y. Children also move in z each
  // frame.
  //
  // The following semantics hold true for the parent shape position:
  // - x = z = 0 => constant
  // - y => frame number
  // The following are true for the children:
  // - x => child index
  // - y = 0 => constant without parent transform, frame number with parent transform.
  // - z => fame number
  ParentsTest<painter::Box> test;
  test.child_count = 10;
  test.frame_count = std::min(100u, frameWindow() - 1);
  test.run(viewer());
}


TEST_F(Shapes, Painter_WindowSimple)
{
  // Make sure our viewable window works in the simple case:
  // - add shapes for N frames
  // - keep a window W where W < N
  // - make sure the window is always valid
  // - make sure expired shapes are not valid.
  painter::Box painter(viewer().culler());

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

TEST_F(Shapes, Painter_WindowParents)
{
  // This test combines Painter_Parents and Painter_WindowSimple such that we ensure that data outside the window are
  // no longer valid.
  // children. We repeat this process often enough to ensure we start expiring shapes.
  ParentsTest<painter::Box> test;
  test.child_count = 10;
  test.frame_count = frameWindow() + 1;
  test.run(viewer());
}
}  // namespace tes::viewer
