#ifndef TES_VIEW_HANDLER_MESSAGE_H
#define TES_VIEW_HANDLER_MESSAGE_H

#include <3esview/ViewConfig.h>

#include <3esview/camera/Camera.h>
#include <3esview/DrawParams.h>
#include <3esview/FrameStamp.h>
#include <3esview/util/Enum.h>

#include <3escore/Messages.h>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector2.h>

#include <string>

namespace tes
{
class Connection;
}  // namespace tes

namespace tes::view::handler
{
/// The base class for a 3es message handler.
///
/// @par Thread safety
/// A @c Message handler will typically have functions called from at least two different threads.
/// In particular the @c readMessage() and @c endFrame() functions are called from the data
/// processing thread, while @c prepareFrame() and @c draw() are called from the main thread.
/// Other functions are called from the main thread. As such, the @c readMessage() and @c endFrame()
/// functions must be thread safe with respect to @c prepareFrame() and @c draw().
///
/// Note that the data thread functions and draw functions - @c prepareFrame() and @c draw() - are
/// independent of one another. There are no call order of frequency guarantees relating
/// @c endFrame() to the two draw functions. That is, the data thread may run independently of draw
/// function calls, calling @c readMessage() zero or more times for each call to @c endFrame() .
/// Mean while, the main thread may or may not call @c prepareFrame() and @c draw() at any time
/// in between. However, it is worth noting that a @c prepareFrame() call which is followed by a
/// @c draw() call cannot have @c endFrame() called in between.
///
/// Generally we expect that every @c prepareFrame() call will be followed by @c draw() calls, but
/// not every @c draw() call is preceeded by a @c prepareFrame() call. This is partly because
/// @c draw() is called with multiple @c DrawPass values, but also because @c prepareFrame() is only
/// called when needed, which is whenever @c draw() calls must be made after @c endFrame() has been
/// called. Multiple sets of @c draw() calls may be made without @c prepareFrame() so long as
/// @c endFrame() has not been called. Meanwhile, @c readMessage() may continue to be called in
/// between @c prepareFrame() and @c draw() calls.
///
/// The @c ThirdEyeScene class manages and enforces this call relationship and associated
/// synchronisation and locking.
class TES_VIEWER_API Message
{
public:
  using ObjectAttributes = tes::ObjectAttributes<Magnum::Float>;

  /// Flags modifying the normal operating behaviour of a message handler.
  enum class ModeFlag
  {
    /// Ignore messages for transient objects. Do not create new transient objects.
    IgnoreTransient = (1u << 0u)
  };

  /// Flags commonly used to manage drawable items in a message handler.
  enum class DrawableFlag : unsigned
  {
    /// No flags set.
    Zero = 0u,
    /// Item is pending commit for render on a future frame.
    Pending = 1u << 0u,
    /// Pending item is ready to be committed on the next frame.
    Ready = 1u << 1u,
    /// Item is to be removed/disposed of on the next commit.
    MarkForDeath = 1u << 2u,
    /// Item has dirty @c ObjectAttributes - transform and/or colour.
    DirtyAttributes = 1u << 3u,
    /// Item has dirty mesh resources.
    DirtyMesh = 1u << 4u,

    /// A combination of @c DirtyAttributes and @c DirtyMesh .
    Dirty = DirtyAttributes | DirtyMesh
  };

  /// Draw pass identifier for @c draw() call semantics.
  enum class DrawPass
  {
    /// Draw opaque objects.
    Opaque,
    /// Draw transparent objects.
    Transparent,
    /// Draw overlay objects.
    Overlay
  };

  Message(uint16_t routing_id, const std::string &name);
  virtual ~Message();

  /// Returns the unique ID for the message handler. This identifies the type of
  /// handled and in some cases, such as Renderers, the type of object handled.
  /// ID ranges are described in the @c MessageTypeIDs enumeration.
  inline uint16_t routingId() const { return _routing_id; }

  /// Read the current @c ModeFlag values.
  inline unsigned modeFlags() const { return _mode_flags; }
  /// Set the @c ModeFlag values.
  /// @param flags New values.
  inline void setModeFlags(unsigned flags) { _mode_flags = flags; }

  /// Get the handler name.
  /// @return The handler name.
  inline const std::string &name() const { return _name; }

  /// Called to initialise the handler with various 3rd Eye Scene components.
  virtual void initialise() = 0;

  /// Clear all data in the handler. This resets it to the default, initialised state.
  ///
  /// For example, this method may be called to clear the scene.
  ///
  /// Called from the data thread. Some changes may need to be deferred until the next
  /// @c prepareFrame() call - e.g., releasing OpenGL resources.
  virtual void reset() = 0;

  /// Called on all handlers whenever the server info changes.
  virtual void updateServerInfo(const ServerInfoMessage &info);

  /// Called from the main thread to prepare the next @c draw() calls following and @c endFrame()
  /// call.
  ///
  /// A set of @c draw() calls (varying @c DrawPass values) with the same @c stamp will immediately
  /// follow before another @c endFrame() call can be made. See class comments for more data/main
  /// thread synchronisation details.
  ///
  /// The primary purpose of this function is to prepare render resources - OpenGL resources - of
  /// newly active objects for the next draw call. This finalises any objects pending such from the
  /// last @c endFrame() calls since the last @c draw() call.
  ///
  /// @param stamp The frame render stamp to begin.
  virtual void prepareFrame(const FrameStamp &stamp) = 0;

  /// Called by the data thread at the end of a frame.
  ///
  /// This indicates that the data thread has processed a @c CIdFrame @c ControlMessage and the
  /// state collected since the last @c endFrame() is now ready for visualisation.
  ///
  /// Generally an implementation has the following expectations:
  ///
  /// - This is threadsafe with respect to other functions modifying the internal visualsation state
  ///   in particular with respect to @c prepareFrame() and @c draw() calls.
  /// - Active transient shapes are discared.
  /// - Pending effects from @c readMessage() calls are effected and ready for visualisation on the
  ///   next @c draw() call. This includes:
  ///   - Activating new transient objects.
  ///   - Activating new persistent objects
  ///   - Removing destroyed persistent objects.
  ///   - Effecting object updates.
  /// - No direct render resources can be changed from this function when using rendering APIs such
  ///   as OpenGL. That is no OpenGL function calls can be made from here, directly or indirectly,
  ///   as this is called from the background thread.
  ///
  /// @param stamp The frame render stamp to end.
  virtual void endFrame(const FrameStamp &stamp) = 0;

  /// Render the current objects.
  /// @param pass The draw pass indicating what type of rendering to perform.
  /// @param stamp The frame stamp. The @c FrameStamp::frame_number always matches value given to
  /// the last @c prepareFrame() call, while the @c FrameStamp::render_mark is monotonic increasing.
  /// @param params Camera, view and projection parameters.
  virtual void draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params) = 0;

  /// Read a message which has been predetermined to be belong to this handler.
  ///
  /// Called by the data thread.
  ///
  /// Any changes described by the message must not be effected until the next call to
  /// @c endFrame() . Additionally, see thread safety requirements described in the class
  /// documentation.
  ///
  /// @param reader The message data reader.
  virtual void readMessage(PacketReader &reader) = 0;

  virtual inline void serialise(Connection &out)
  {
    ServerInfoMessage info = {};
    serialise(out, info);
  }

  /// Serialise a snapshot of the renderable objects for the specified frame. Serialisation is
  /// performed using the messages required to restore the current state.
  /// @param out Stream to write to.
  /// @param[out] info Provides information about about the serialisation.
  virtual void serialise(Connection &out, ServerInfoMessage &info) = 0;

  static Magnum::Matrix4 composeTransform(const ObjectAttributes &attrs);
  static void decomposeTransform(const Magnum::Matrix4 &transform, ObjectAttributes &attrs);

protected:
  uint16_t _routing_id = 0u;
  unsigned _mode_flags = 0u;
  ServerInfoMessage _server_info = {};
  std::string _name;
};

TES_ENUM_FLAGS(Message::DrawableFlag, unsigned);
}  // namespace tes::view::handler

#endif  // TES_VIEW_HANDLER_MESSAGE_H
