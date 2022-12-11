//
// Author: Kazys Stepanas
//
#include "3esflat.h"

#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Color.h>

namespace tes::viewer::shaders
{
Flat::Flat()
  : _shader(std::make_shared<Magnum::Shaders::Flat3D>(Magnum::Shaders::Flat3D::Flag::VertexColor |
                                                      Magnum::Shaders::Flat3D::Flag::InstancedTransformation))
{}


Flat::~Flat() = default;


Shader &Flat::setProjectionMatrix(const Magnum::Matrix4 &projection)
{
  _shader->setTransformationProjectionMatrix(projection);
  return *this;
}

Shader &Flat::setColour(const Magnum::Color4 &colour)
{
  _shader->setColor(colour);
  return *this;
}

Shader &Flat::draw(Magnum::GL::Mesh &mesh)
{
  _shader->draw(mesh);
  return *this;
}

Shader &Flat::draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer, size_t instance_count)
{
  mesh.setInstanceCount(Magnum::Int(instance_count))
    .addVertexBufferInstanced(buffer, 1, 0, Magnum::Shaders::Flat3D::TransformationMatrix{},
                              Magnum::Shaders::Flat3D::Color4{});
  _shader->draw(mesh);
  return *this;
}
}  // namespace tes::viewer::shaders
