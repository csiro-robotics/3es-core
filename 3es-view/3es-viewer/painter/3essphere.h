
#include "3es-viewer.h"

#include "3esshapepainter.h"

namespace tes::viewer::painter
{
class Sphere : public ShapePainter
{
public:
  Sphere(std::shared_ptr<BoundsCuller> culler);

  static Magnum::GL::Mesh solidMesh();
  static Magnum::GL::Mesh wireframeMesh();

private:
};
}  // namespace tes::viewer::painter
