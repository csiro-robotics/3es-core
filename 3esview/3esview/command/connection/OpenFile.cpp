#include "OpenFile.h"

#include <3esview/Viewer.h>

namespace tes::view::command::connection
{
OpenFile::OpenFile()
  : Command("openFile", Args(std::string()))
{}


bool OpenFile::checkAdmissible(Viewer &viewer) const
{
  return viewer.dataThread() == nullptr;
}


CommandResult OpenFile::invoke(Viewer &viewer, const ExecInfo &info, const Args &args)
{
  (void)info;
  const auto filename = args.at<std::string>(0);
  if (!viewer.open(filename))
  {
    return { CommandResult::Code::Failed, "Failed to open " + filename };
  }
  return { CommandResult::Code::Ok };
}
}  // namespace tes::view::command::connection
