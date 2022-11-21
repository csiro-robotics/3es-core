#include "3esshapepainter.h"

#include <shapes/3esid.h>

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
  _pending_removal.clear();
}


ShapePainter::ParentId ShapePainter::add(const Id &id, Type type, const Magnum::Matrix4 &transform,
                                         const Magnum::Color4 &colour)
{
  util::ResourceListId index = addShape(id, type, transform, colour);
  if (!id.isTransient())
  {
    // Handle re-adding a shape which is already pending removal.
    const auto search = _id_index_map.find(id);
    if (search != _id_index_map.end())
    {
      _id_index_map.erase(search);
      for (auto iter = _pending_removal.begin(); iter != _pending_removal.end();)
      {
        if (*iter == id)
        {
          iter = _pending_removal.erase(iter);
        }
        else
        {
          ++iter;
        }
      }
    }
    _id_index_map.emplace(id, CacheIndex{ type, index });
  }
  return ParentId(id, index);
}


ShapePainter::ChildId ShapePainter::addChild(const ParentId &parent_id, Type type, const Magnum::Matrix4 &transform,
                                             const Magnum::Color4 &colour)
{
  unsigned child_index = 0;
  addShape(parent_id.shapeId(), type, transform, colour, parent_id, &child_index);
  return ChildId(parent_id.shapeId(), child_index);
}


util::ResourceListId ShapePainter::addShape(const Id &shape_id, Type type, const Magnum::Matrix4 &transform,
                                            const Magnum::Color4 &colour, const ParentId &parent_id,
                                            unsigned *child_index)
{
  if (ShapeCache *cache = cacheForType(type))
  {
    ShapeCache::ShapeFlag flags = ShapeCache::ShapeFlag::None;
    flags |= (shape_id.isTransient()) ? ShapeCache::ShapeFlag::Transient : ShapeCache::ShapeFlag::None;
    return cache->add(shape_id, transform, colour, flags, parent_id.resourceId(), child_index);
  }
  return ~0u;
}


bool ShapePainter::update(const Id &id, const Magnum::Matrix4 &transform, const Magnum::Color4 &colour)
{
  const auto search = _id_index_map.find(id);
  if (search != _id_index_map.end())
  {
    if (ShapeCache *cache = cacheForType(search->second.type))
    {
      cache->update(search->second.index, transform, colour);
    }
    return true;
  }

  return false;
}


bool ShapePainter::updateChildShape(const ChildId &child_id, const Magnum::Matrix4 &transform,
                                    const Magnum::Color4 &colour)
{
  const auto search = _id_index_map.find(child_id.shapeId());
  if (search != _id_index_map.end())
  {
    if (ShapeCache *cache = cacheForType(search->second.type))
    {
      // Lookup the child resource id.
      auto child_rid = cache->getChildId(search->second.index, child_id.index());
      if (child_rid != util::kNullResource)
      {
        cache->update(child_rid, transform, colour);
      }
    }
    return true;
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
      cache->endShape(search->second.index);
      _pending_removal.emplace_back(id);
      return true;
    }
  }

  return false;
}


bool ShapePainter::readShape(const Id &id, Magnum::Matrix4 &transform, Magnum::Color4 &colour) const
{
  const auto search = _id_index_map.find(id);
  if (search != _id_index_map.end())
  {
    if (const ShapeCache *cache = cacheForType(search->second.type))
    {
      return cache->get(search->second.index, false, transform, colour);
    }
  }
  return false;
}


bool ShapePainter::readChildShape(const ChildId &child_id, bool include_parent_transform, Magnum::Matrix4 &transform,
                                  Magnum::Color4 &colour) const
{
  const auto search = _id_index_map.find(child_id.shapeId());
  if (search != _id_index_map.end())
  {
    if (const ShapeCache *cache = cacheForType(search->second.type))
    {
      // Lookup the child resource id.
      auto child_rid = cache->getChildId(search->second.index, child_id.index());
      if (child_rid != util::kNullResource)
      {
        return cache->get(child_rid, include_parent_transform, transform, colour);
      }
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


void ShapePainter::commit()
{
  _solid_cache->commit();
  _wireframe_cache->commit();
  _transparent_cache->commit();

  for (auto id : _pending_removal)
  {
    const auto search = _id_index_map.find(id);
    if (search != _id_index_map.end())
    {
      _id_index_map.erase(search);
    }
  }
  _pending_removal.clear();
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
