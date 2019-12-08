//
// author: Kazys Stepanas
//
#ifndef _3ESSERVERUTIL_H_
#define _3ESSERVERUTIL_H_

#include "3es-core.h"

#include <3espacketwriter.h>

namespace tes
{
/// A helper function for sending an arbitrary message structure via a @c Connection or @c Server object.
///
/// The @c MESSAGE structure must support the following message signature:
/// <tt>bool write(PacketWriter &writer) const</tt>, returning true on successfully writing data to @c writer.
///
/// @tparam MESSAGE The message structure containing the data to pack.
/// @tparam BufferSize Size of the buffer used to pack data from @c MESSAGE into. Stack allocated.
///
/// @param connection The @c Server or @c Connection to send the message via (@c Connection::send()).
/// @param routingId The ID of the message handler which will decode the message.
/// @param message The message data to pack.
/// @param allowCollation Allow collation with other messages? False to disallow collation and compression for @p
/// message.
/// @return The number of bytes written to @p connection, or -1 on failure.
template <class MESSAGE, unsigned BufferSize = 256>
int sendMessage(Connection &connection, uint16_t routingId, uint16_t messageId, const MESSAGE &message,
                bool allowCollation = true)
{
  uint8_t buffer[BufferSize];
  PacketWriter writer(buffer, BufferSize);
  writer.reset(routingId, messageId);
  if (message.write(writer) && writer.finalise())
  {
    return connection.send(writer.data(), writer.packetSize(), allowCollation);
  }

  return -1;
}
}  // namespace tes

#endif  // _3ESSERVERUTIL_H_
