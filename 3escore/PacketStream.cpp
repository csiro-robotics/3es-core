//
// author: Kazys Stepanas
//
#include "PacketStream.h"

namespace tes
{
template class PacketStream<PacketHeader>;
template class PacketStream<const PacketHeader>;
}  // namespace tes
