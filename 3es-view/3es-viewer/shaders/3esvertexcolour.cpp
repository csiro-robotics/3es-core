//
// Author: Kazys Stepanas
//
#include "3esvertexcolour.h"

#include <3eslog.h>

#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Color.h>

namespace tes::viewer::shaders
{
VertexColour::VertexColour()
  : _shader(std::make_shared<Magnum::Shaders::VertexColour3D>())
{}


VertexColour::~VertexColour() = default;


Shader &VertexColour::setProjectionMatrix(const Magnum::Matrix4 &projection)
{
  _shader->setTransformationProjectionMatrix(projection);
  return *this;
}

Shader &VertexColour::setColour(const Magnum::Color4 &colour)
{
  _shader->setColor(colour);
  return *this;
}

Shader &VertexColour::draw(Magnum::GL::Mesh &mesh)
{
  _shader->draw(mesh);
  return *this;
}

Shader &VertexColour::draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer, size_t instance_count)
{
  log::error("VertexColour shader does not support instanced rendering.") return *this;
}
}  // namespace tes::viewer::shaders
