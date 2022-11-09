#include "3esshapepainter.h"

#include <Magnum/GL/Renderer.h>

namespace tes::viewer::painter
{
ShapePainter::ShapePainter(std::shared_ptr<BoundsCuller> culler, std::initializer_list<Part> solid_mesh,
                           std::initializer_list<Part> wireframe_mesh, std::initializer_list<Part> transparent_mesh,
                           BoundsCalculator bounds_calculator)
{
  _solid_cache = std::make_unique<ShapeCache>(culler, std::move(solid_mesh));
  _wireframe_cache = std::make_unique<ShapeCache>(culler, std::move(wireframe_mesh));
  _transparent_cache = std::make_unique<ShapeCache>(culler, std::move(transparent_mesh));

  _solid_cache->setBoundsCalculator(bounds_calculator);
  _wireframe_cache->setBoundsCalculator(bounds_calculator);
  _transparent_cache->setBoundsCalculator(bounds_calculator);
}

ShapePainter::~ShapePainter() = default;

void ShapePainter::add(const Id &id, Type type, const Magnum::Matrix4 &transform, const Magnum::Color4 &colour)
{
  if (ShapeCache *cache = cacheForType(type))
  {
    unsigned index = cache->add(transform, colour);
    if (id == Id())
    {
      // Transient object.
      switch (type)
      {
      case Type::Solid:
        _solid_transients.emplace_back(index);
        break;
      case Type::Wireframe:
        _wireframe_transients.emplace_back(index);
        break;
      case Type::Transparent:
        _transparent_transients.emplace_back(index);
        break;
      }
    }
    else
    {
      _id_index_map.emplace(id, CacheIndex{ type, index });
    }
  }
}


bool ShapePainter::update(const Id &id, const Magnum::Matrix4 &transform, const Magnum::Color4 &colour)
{
  const auto search = _id_index_map.find(id);
  if (search != _id_index_map.end())
  {
    if (ShapeCache *cache = cacheForType(search->second.type))
    {
      cache->update(search->second.index, transform, colour);
      return true;
    }
  }

  return false;
}


bool ShapePainter::remove(const Id &id)
{
  const auto search = _id_index_map.find(id);
  if (search != _id_index_map.end())
  {
    if (ShapeCache *cache = cacheForType(search->second.type))
    {
      cache->remove(search->second.index);
      return true;
    }
  }

  return false;
}


void ShapePainter::drawOpaque(unsigned render_mark, const Magnum::Matrix4 &projection_matrix)
{
  _solid_cache->draw(render_mark, projection_matrix);
  _wireframe_cache->draw(render_mark, projection_matrix);
}


void ShapePainter::drawTransparent(unsigned render_mark, const Magnum::Matrix4 &projection_matrix)
{
  Magnum::GL::Renderer::setBlendFunction(Magnum::GL::Renderer::BlendFunction::SourceAlpha,
                                         Magnum::GL::Renderer::BlendFunction::OneMinusSourceAlpha);
  _transparent_cache->draw(render_mark, projection_matrix);
  Magnum::GL::Renderer::setBlendFunction(Magnum::GL::Renderer::BlendFunction::One,
                                         Magnum::GL::Renderer::BlendFunction::Zero);
}


void ShapePainter::endFrame()
{
  for (const auto index : _solid_transients)
  {
    _solid_cache->remove(index);
  }
  _solid_transients.clear();
  for (const auto index : _wireframe_transients)
  {
    _wireframe_cache->remove(index);
  }
  _wireframe_transients.clear();
  for (const auto index : _transparent_transients)
  {
    _transparent_cache->remove(index);
  }
  _transparent_transients.clear();
}


ShapeCache *ShapePainter::cacheForType(Type type)
{
  switch (type)
  {
  case Type::Solid:
    return _solid_cache.get();
    break;
  case Type::Transparent:
    return _transparent_cache.get();
    break;
  case Type::Wireframe:
    return _wireframe_cache.get();
    break;
  default:
    break;
  }
  return nullptr;
}
}  // namespace tes::viewer::painter
