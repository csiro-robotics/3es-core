//
// Author: Kazys Stepanas
//
#include "ShaderLibrary.h"

#include <3escore/Log.h>

#include <array>

namespace tes::view::shaders
{
namespace
{
const std::array<std::string, unsigned(ShaderLibrary::ID::Count)> &shaderNames()
{
  static const std::array<std::string, unsigned(ShaderLibrary::ID::Count)> names =  //
    {
      "Flat",          //
      "VertexColour",  //
      "Line",          //
      "PointCloud",    //
      "Voxel",         //
    };
  return names;
}
};  // namespace


ShaderLibrary::ShaderLibrary()
  : _core_shaders(unsigned(ID::Count))
{}


ShaderLibrary::~ShaderLibrary() = default;


std::string ShaderLibrary::shaderName(ID id)
{
  const unsigned idx = static_cast<unsigned>(id);
  const auto &names = shaderNames();
  return (idx < names.size()) ? names[idx] : std::string();
}


std::shared_ptr<Shader> ShaderLibrary::lookup(ID id) const
{
  const unsigned idx = static_cast<unsigned>(id);
  return (idx < _core_shaders.size()) ? _core_shaders[idx] : nullptr;
}


std::shared_ptr<Shader> ShaderLibrary::lookup(const std::string &name) const
{
  const auto search = _shaders.find(name);
  return (search != _shaders.end()) ? search->second : nullptr;
}


std::shared_ptr<Shader> ShaderLibrary::lookupForDrawType(DrawType draw_type) const
{
  switch (draw_type)
  {
  case DtPoints:
    return lookup(shaders::ShaderLibrary::ID::PointCloud);
  case DtLines:
    return lookup(shaders::ShaderLibrary::ID::Line);
  case DtTriangles:
    return lookup(shaders::ShaderLibrary::ID::VertexColour);
  case DtVoxels:
    return lookup(shaders::ShaderLibrary::ID::Voxel);
  default:
    log::error("Unsupported mesh draw type: ", int(draw_type));
    break;
  }
  return nullptr;
}


void ShaderLibrary::registerShader(ID id, std::shared_ptr<Shader> shader)
{
  const unsigned idx = static_cast<unsigned>(id);
  _core_shaders[idx] = shader;
  _shaders.emplace(shaderName(id), shader);
}


void ShaderLibrary::registerShader(const std::string &name, std::shared_ptr<Shader> shader)
{
  _shaders.emplace(name, shader);
}
}  // namespace tes::view::shaders
