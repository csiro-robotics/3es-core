#include "3esshapepainter.h"

#include <Magnum/GL/Renderer.h>

namespace tes::viewer::painter
{
ShapePainter::ShapePainter(std::shared_ptr<BoundsCuller> culler, std::initializer_list<Part> solid_mesh,
                           std::initializer_list<Part> wireframe_mesh, std::initializer_list<Part> transparent_mesh,
                           BoundsCalculator bounds_calculator)
  : ShapePainter(std::move(culler), std::vector<Part>(std::move(solid_mesh)),
                 std::vector<Part>(std::move(wireframe_mesh)), std::vector<Part>(std::move(transparent_mesh)),
                 std::move(bounds_calculator))
{}

ShapePainter::ShapePainter(std::shared_ptr<BoundsCuller> culler, const std::vector<Part> &solid,
                           const std::vector<Part> &wireframe, const std::vector<Part> &transparent,
                           BoundsCalculator bounds_calculator)
{
  _solid_cache = std::make_unique<ShapeCache>(culler, solid);
  _wireframe_cache = std::make_unique<ShapeCache>(culler, wireframe);
  _transparent_cache = std::make_unique<ShapeCache>(culler, transparent);

  _solid_cache->setBoundsCalculator(bounds_calculator);
  _wireframe_cache->setBoundsCalculator(bounds_calculator);
  _transparent_cache->setBoundsCalculator(bounds_calculator);
}


ShapePainter::~ShapePainter() = default;


void ShapePainter::reset()
{
  _solid_cache->clear();
  _wireframe_cache->clear();
  _transparent_cache->clear();
  _id_index_map.clear();
}


ShapePainter::ParentId ShapePainter::add(const Id &id, FrameNumber frame_number, Type type,
                                         const Magnum::Matrix4 &transform, const Magnum::Color4 &colour)
{
  const bool transient = id.id() == 0;
  const ViewableWindow view_window(frame_number, (transient) ? 1 : 0, ViewableWindow::Interval::Relative);
  util::ResourceListId index = addShape(view_window, type, transform, colour);
  if (!transient)
  {
    _id_index_map.emplace(id, CacheIndex{ type, index });
  }
  return ParentId(index);
}


void ShapePainter::addSubShape(const ParentId &parent_id, FrameNumber frame_number, Type type,
                               const Magnum::Matrix4 &transform, const Magnum::Color4 &colour)
{
  const bool transient = _id_index_map.find(parent_id.id()) == _id_index_map.end();
  const ViewableWindow view_window(frame_number, (transient) ? 1 : 0, ViewableWindow::Interval::Relative);
  addShape(view_window, type, transform, colour, parent_id);
}


util::ResourceListId ShapePainter::addShape(const ViewableWindow &view_window, Type type,
                                            const Magnum::Matrix4 &transform, const Magnum::Color4 &colour,
                                            const ParentId &parent_id)
{
  if (ShapeCache *cache = cacheForType(type))
  {
    return cache->add(view_window, transform, colour, parent_id.id());
  }
  return ~0u;
}


bool ShapePainter::update(const Id &id, FrameNumber frame_number, const Magnum::Matrix4 &transform,
                          const Magnum::Color4 &colour)
{
  const auto search = _id_index_map.find(id);
  if (search != _id_index_map.end())
  {
    if (ShapeCache *cache = cacheForType(search->second.type))
    {
      cache->update(search->second.index, frame_number, transform, colour);
    }
    return true;
  }

  return false;
}


bool ShapePainter::readProperties(const Id &id, FrameNumber frame_number, bool include_parent_transform,
                                  Magnum::Matrix4 &transform, Magnum::Color4 &colour) const
{
  const auto search = _id_index_map.find(id);
  if (search != _id_index_map.end())
  {
    if (const ShapeCache *cache = cacheForType(search->second.type))
    {
      return cache->get(search->second.index, frame_number, include_parent_transform, transform, colour);
    }
  }
  return false;
}


bool ShapePainter::remove(const Id &id, FrameNumber frame_number)
{
  const auto search = _id_index_map.find(id);
  if (search != _id_index_map.end())
  {
    if (ShapeCache *cache = cacheForType(search->second.type))
    {
      cache->endShape(search->second.index, (frame_number > 0) ? frame_number - 1u : 0u);
      return true;
    }
  }

  return false;
}


void ShapePainter::drawOpaque(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix)
{
  _solid_cache->draw(stamp, projection_matrix);
  _wireframe_cache->draw(stamp, projection_matrix);
}


void ShapePainter::drawTransparent(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix)
{
  Magnum::GL::Renderer::setBlendFunction(Magnum::GL::Renderer::BlendFunction::SourceAlpha,
                                         Magnum::GL::Renderer::BlendFunction::OneMinusSourceAlpha);
  _transparent_cache->draw(stamp, projection_matrix);
  Magnum::GL::Renderer::setBlendFunction(Magnum::GL::Renderer::BlendFunction::One,
                                         Magnum::GL::Renderer::BlendFunction::Zero);
}


void ShapePainter::endFrame(FrameNumber frame_number)
{
  const FrameNumber kCacheWindow = frameWindow();
  if (frame_number >= kCacheWindow)
  {
    const FrameNumber expire_before = frame_number - kCacheWindow;
    _solid_cache->expireShapes(expire_before);
    _wireframe_cache->expireShapes(expire_before);
    _transparent_cache->expireShapes(expire_before);
  }
}


ShapeCache *ShapePainter::cacheForType(Type type)
{
  switch (type)
  {
  case Type::Solid:
    return _solid_cache.get();
  case Type::Transparent:
    return _transparent_cache.get();
  case Type::Wireframe:
    return _wireframe_cache.get();
  default:
    break;
  }
  return nullptr;
}


const ShapeCache *ShapePainter::cacheForType(Type type) const
{
  switch (type)
  {
  case Type::Solid:
    return _solid_cache.get();
  case Type::Transparent:
    return _transparent_cache.get();
  case Type::Wireframe:
    return _wireframe_cache.get();
  default:
    break;
  }
  return nullptr;
}
}  // namespace tes::viewer::painter
