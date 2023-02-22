
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
  std::unique_ptr<QTcpSocket> socket;
  int read_timeout = -1;
  int write_timeout = -1;
  inline ~TcpSocketDetail() { delete socket; }
};

struct TcpListenSocketDetail
{
  QTcpServer listen_socket;
};
}  // namespace tes

#endif  // TES_CORE_QT_TCP_DETAIL_H
