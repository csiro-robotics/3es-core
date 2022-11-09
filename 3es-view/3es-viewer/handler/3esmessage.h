#ifndef TES_VIEWER_HANDLER_MESSAGE_H
#define TES_VIEWER_HANDLER_MESSAGE_H

#include "3es-viewer.h"

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
  /// @param frame_number A monotonic frame counter.
  /// @param render_mark The rendering mark which was used to cull the current frame.
  /// @param maintain_transient True to prevent flushing of transient objects.
  virtual void beginFrame(unsigned frame_number, unsigned render_mark, bool maintain_transient) = 0;

  /// Called at the end of a frame. In practice, this is likely to be called
  /// at the same time as @c beginFrame() .
  ///
  /// @param frame_number A monotonic frame counter.
  virtual void endFrame(unsigned frame_number, unsigned render_mark) = 0;

  /// Render the current objects.
  virtual void draw(DrawPass pass, unsigned frame_number, unsigned render_mark,
                    const Magnum::Matrix4 &projection_matrix) = 0;

  /// Read a message which has been predetermined to be belong to this handler.
  ///
  /// Any changes described by the message must not be effected until the next call to @c beginFrame() . Additionally,
  /// this function must be thread safe as it will generally be called from a different thread that the draw call.
  /// However, because of the way messages are processed, it it guaranteed that at the next frame boundary, the next
  /// call to @c beginFrame() , all the messages should be effected.
  ///
  /// @param reader The message data reader.
  virtual void readMessage(PacketReader &reader) = 0;

  virtual inline void serialise(Connection &out)
  {
    ServerInfoMessage info = {};
    serialise(out, info);
  }
  /// Serialise a snapshop of the renderable objects for the current frame. Serialisation is performed using the
  /// messages required to restore the current state.
  /// @param out Stream to write to.
  /// @param[out] info Provides information about about the serialisation.
  virtual void serialise(Connection &out, ServerInfoMessage &info) = 0;

protected:
  uint16_t _routing_id = 0u;
  unsigned _mode_flags = 0u;
  ServerInfoMessage _server_info = {};
  std::string _name;
};
}  // namespace tes::viewer::handler

#endif  // TES_VIEWER_HANDLER_MESSAGE_H
