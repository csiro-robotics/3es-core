#include "OpenFile.h"

#include <3esview/Viewer.h>

#include <nfd.h>

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
  std::string filename;
  if (!args.empty())
  {
    filename = args.at<std::string>(0);
  }
  else
  {
    filename = fromDialog();
  }

  if (filename.empty())
  {
    return { CommandResult::Code::Cancel };
  }

  if (!viewer.open(filename))
  {
    return { CommandResult::Code::Failed, "Failed to open " + filename };
  }
  return { CommandResult::Code::Ok };
}


std::string OpenFile::fromDialog()
{
  // const nfdchar_t *filter_list = "3rd Eye Scene files (*.3es),*.3es";
  // Seems the vcpkg version of nativefiledialog only supports file extension strings.
  const nfdchar_t *filter_list = "3es";
  // TODO(KS): cache the last use path and reload it.
  const nfdchar_t *default_path = "";
  nfdchar_t *selected_path = nullptr;
  nfdresult_t result = NFD_OpenDialog(filter_list, default_path, &selected_path);

  const std::string path = (selected_path) ? selected_path : std::string();
  free(selected_path);

  if (result == NFD_OKAY)
  {
    return path;
  }
  return std::string();
}
}  // namespace tes::view::command::connection
