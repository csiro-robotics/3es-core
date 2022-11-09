#ifndef TES_CLIENT_VIEW_H
#define TES_CLIENT_VIEW_H

#include "3es-view.h"

#include <memory>

namespace tes
{
class ViewDetail;
/// Provides a view window into the 3rd eye scene.
class View
{
private:
  std::unique_ptr<ViewDetail> _imp;
};
}  // namespace tes

#endif  // TES_CLIENT_VIEW_H
