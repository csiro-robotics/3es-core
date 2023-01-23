//
// author: Kazys Stepanas
//
#include "3espacketstream.h"

namespace tes
{
template class PacketStream<PacketHeader>;
template class PacketStream<const PacketHeader>;
}  // namespace tes
