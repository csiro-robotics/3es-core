#ifndef TES_VIEWER_HANDLER_MESSAGE_H
#define TES_VIEWER_HANDLER_MESSAGE_H

#include "3es-viewer.h"

#include "3esframestamp.h"

#include <Magnum/Magnum.h>

#include <3esmessages.h>

#include <string>

namespace tes
{
class Connection;
}  // namespace tes

namespace tes::viewer::handler
{
/// The base class for a 3es message handler.
///
/// @par Thread safety
/// A @c Message handler will typically have functions called from at least two different threads. In particular the
/// @c readMessage() function is called from the data processing thread, while @c beginFrame(), @c endFrame(),
/// and @c draw() are called from the render thread - likely the main thread. Other functions are called from the main
/// thread.
///
/// As such, the @c readMessage() function must be thread safe with respect to @c beginFrame(), @c endFrame() and
/// @c draw().
class Message
{
public:
  /// Flags modifying the normal operating behaviour of a message handler.
  enum class ModeFlag
  {
    /// Ignore messages for transient objects. Do not create new transient objects.
    IgnoreTransient = (1u << 0u)
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
  /// For example, this method may be called to clear the scene.
  virtual void reset() = 0;

  /// Called on all handlers whenever the server info changes.
  virtual void updateServerInfo(const ServerInfoMessage &info);

  /// Called at the start of a new frame, before processing new messages.
  ///
  /// In practice, this method is called when the @c ControlId::CIdEnd message arrives, just prior to processing all
  /// messages for the completed frame.
  ///
  /// @param frame_stamp The frame render stamp to begin.
  /// @param render_mark The rendering mark which was used to cull the current frame.
  virtual void beginFrame(const FrameStamp &stamp) = 0;

  /// Called at the end of a frame. In practice, this is likely to be called
  /// at the same time as @c beginFrame() .
  ///
  /// @param frame_stamp The frame render stamp to end.
  virtual void endFrame(const FrameStamp &stamp) = 0;

  /// Render the current objects.
  virtual void draw(DrawPass pass, const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix) = 0;

  /// Read a message which has been predetermined to be belong to this handler.
  ///
  /// Any changes described by the message must not be effected until the next call to @c beginFrame() with matching
  /// @p frame_number. Additionally, see thread safety requirements described in the class documentation.
  ///
  /// @param reader The message data reader.
  virtual void readMessage(PacketReader &reader) = 0;

  virtual inline void serialise(Connection &out)
  {
    ServerInfoMessage info = {};
    serialise(out, info);
  }
  /// Serialise a snapshot of the renderable objects for the specified frame. Serialisation is performed using the
  /// messages required to restore the current state.
  /// @param out Stream to write to.
  /// @param[out] info Provides information about about the serialisation.
  virtual void serialise(Connection &out, ServerInfoMessage &info) = 0;

protected:
  uint16_t _routing_id = 0u;
  unsigned _mode_flags = 0u;
  FrameStamp _frame_stamp;
  ServerInfoMessage _server_info = {};
  std::string _name;
};
}  // namespace tes::viewer::handler

#endif  // TES_VIEWER_HANDLER_MESSAGE_H
