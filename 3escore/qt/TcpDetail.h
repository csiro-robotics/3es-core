
//
// author: Kazys Stepanas
//
#ifndef TES_CORE_QT_TCP_DETAIL_H
#define TES_CORE_QT_TCP_DETAIL_H

#include <3escore/CoreConfig.h>

#include <QTcpServer>
#include <QTcpSocket>

namespace tes
{
struct TcpSocketDetail
{
  QTcpSocket *socket;
  int readTimeout;
  int writeTimeout;

  inline TcpSocketDetail()
    : socket(nullptr)
    , readTimeout(~0u)
    , writeTimeout(~0u)
  {}

  inline ~TcpSocketDetail() { delete socket; }
};

struct TcpListenSocketDetail
{
  QTcpServer listenSocket;
};
}  // namespace tes

#endif  // TES_CORE_QT_TCP_DETAIL_H
