#ifndef _3ESTCPBASE_H
#define _3ESTCPBASE_H

#include <3escore/CoreConfig.h>

struct timeval;

namespace tes
{
namespace tcpbase
{
enum SocketError
{

};

int TES_CORE_API create();
void TES_CORE_API close(int socket);

bool TES_CORE_API setReceiveTimeout(int socket, unsigned timeoutMs);
unsigned TES_CORE_API getReceiveTimeout(int socket);

bool TES_CORE_API setSendTimeout(int socket, unsigned timeoutMs);
unsigned TES_CORE_API getSendTimeout(int socket);

void TES_CORE_API enableBlocking(int socket);
void TES_CORE_API disableBlocking(int socket);

void TES_CORE_API timevalFromMs(timeval &tv, unsigned milliseconds);

void TES_CORE_API dumpSocketOptions(int socket);

unsigned short TES_CORE_API getSocketPort(int socket);

bool TES_CORE_API isConnected(int socket);

void TES_CORE_API setNoDelay(int socket, bool noDelay);

bool TES_CORE_API noDelay(int socket);

bool TES_CORE_API checkSend(int socket, int ret);

bool TES_CORE_API checkRecv(int socket, int ret);

int TES_CORE_API getSendBufferSize(int socket);

bool TES_CORE_API setSendBufferSize(int socket, int bufferSize);

int TES_CORE_API getReceiveBufferSize(int socket);

bool TES_CORE_API setReceiveBufferSize(int socket, int bufferSize);
}  // namespace tcpbase
}  // namespace tes

#endif  // _3ESTCPBASE_H
