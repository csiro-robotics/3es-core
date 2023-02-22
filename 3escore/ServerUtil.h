//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SERVER_UTIL_H
#define TES_CORE_SERVER_UTIL_H

#include "CoreConfig.h"

#include "PacketWriter.h"

#include <array>

namespace tes
{
/// A helper function for sending an arbitrary message structure via a @c Connection or @c Server
/// object.
///
/// The @c MESSAGE structure must support the following message signature:
/// `bool write(PacketWriter &writer) const`, returning true on successfully writing data to
/// @c writer.
///
/// @tparam MESSAGE The message structure containing the data to pack.
/// @tparam BufferSize Size of the buffer used to pack data from @c MESSAGE into. Stack allocated.
///
/// @param connection The @c Server or @c Connection to send the message via (@c
/// Connection::send()).
/// @param routing_id The ID of the message handler which will decode the message.
/// @param message_id The ID of the message for the handler to process.
/// @param message The message data to pack.
/// @param allow_collation Allow collation with other messages? False to disallow collation and
/// compression for @p message.
/// @return The number of bytes written to @p connection, or -1 on failure.
template <class MESSAGE, unsigned BufferSize = 256>
int sendMessage(Connection &connection, uint16_t routing_id, uint16_t message_id,
                const MESSAGE &message, bool allow_collation = true)
{
  std::array<uint8_t, BufferSize> buffer;
  PacketWriter writer(buffer.data(), BufferSize);
  writer.reset(routing_id, message_id);
  if (message.write(writer) && writer.finalise())
  {
    return connection.send(writer.data(), writer.packetSize(), allow_collation);
  }

  return -1;
}
}  // namespace tes

#endif  // TES_CORE_SERVER_UTIL_H
