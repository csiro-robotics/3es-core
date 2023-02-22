#ifndef TES_CORE_TCP_TCP_BASE_H
#define TES_CORE_TCP_TCP_BASE_H

#include <3escore/CoreConfig.h>

#include <cinttypes>

struct timeval;

namespace tes::tcpbase
{
enum SocketError
{

};

int TES_CORE_API create();
void TES_CORE_API close(int socket);

bool TES_CORE_API setReceiveTimeout(int socket, unsigned timeout_ms);
unsigned TES_CORE_API getReceiveTimeout(int socket);

bool TES_CORE_API setSendTimeout(int socket, unsigned timeout_ms);
unsigned TES_CORE_API getSendTimeout(int socket);

void TES_CORE_API enableBlocking(int socket);
void TES_CORE_API disableBlocking(int socket);

void TES_CORE_API timevalFromMs(timeval &tv, unsigned milliseconds);

void TES_CORE_API dumpSocketOptions(int socket);

uint16_t TES_CORE_API getSocketPort(int socket);

bool TES_CORE_API isConnected(int socket);

void TES_CORE_API setNoDelay(int socket, bool no_delay);

bool TES_CORE_API noDelay(int socket);

bool TES_CORE_API checkSend(int socket, int ret);

bool TES_CORE_API checkRecv(int socket, int ret);

int TES_CORE_API getSendBufferSize(int socket);

bool TES_CORE_API setSendBufferSize(int socket, int buffer_size);

int TES_CORE_API getReceiveBufferSize(int socket);

bool TES_CORE_API setReceiveBufferSize(int socket, int buffer_size);
}  // namespace tes::tcpbase

#endif  // TES_CORE_TCP_TCP_BASE_H
