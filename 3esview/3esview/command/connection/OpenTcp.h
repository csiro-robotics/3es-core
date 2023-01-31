#ifndef TES_VIEW_COMMAND_CONNECTION_OPEN_TCP_H
#define TES_VIEW_COMMAND_CONNECTION_OPEN_TCP_H

#include <3esview/ViewConfig.h>

#include <3esview/command/Command.h>

namespace tes::view::command::connection
{
class TES_VIEWER_API OpenTcp : public Command
{
public:
  OpenTcp();

protected:
  bool checkAdmissible(Viewer &viewer) const override;
  CommandResult invoke(Viewer &viewer, const ExecInfo &info, const Args &args) override;
};
}  // namespace tes::view::command::connection

#endif  // TES_VIEW_COMMAND_CONNECTION_OPEN_TCP_H
