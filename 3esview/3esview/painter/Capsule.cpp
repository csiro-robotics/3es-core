#include "Capsule.h"

#include "Cylinder.h"
#include "Sphere.h"

#include <3esview/mesh/Converter.h>
#include <3esview/shaders/ShaderLibrary.h>

#include <3escore/shapes/SimpleMesh.h>
#include <3escore/tessellate/Cylinder.h>
#include <3escore/tessellate/Sphere.h>

#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Matrix3.h>

#include <cassert>
#include <mutex>

namespace tes::view::painter
{
constexpr float Capsule::kDefaultRadius;
constexpr float Capsule::kDefaultHeight;
const Vector3f Capsule::kDefaultAxis = { 0.0f, 0.0f, 1.0f };

Capsule::Capsule(std::shared_ptr<BoundsCuller> culler, std::shared_ptr<shaders::ShaderLibrary> shaders)
  : ShapePainter(culler, shaders, { Part{ solidMeshCylinder() } }, { Part{ wireframeMeshCylinder() } },
                 { Part{ solidMeshCylinder() } }, calculateBounds)
{
  _solid_end_caps[0] =
    std::make_unique<ShapeCache>(culler, _solid_cache->shader(), Part{ solidMeshCapTop() }, calculateBounds);
  _solid_end_caps[1] =
    std::make_unique<ShapeCache>(culler, _solid_cache->shader(), Part{ solidMeshCapBottom() }, calculateBounds);

  _wireframe_end_caps[0] =
    std::make_unique<ShapeCache>(culler, _wireframe_cache->shader(), Part{ wireframeMeshCap() }, calculateBounds);
  _wireframe_end_caps[1] =
    std::make_unique<ShapeCache>(culler, _wireframe_cache->shader(), Part{ wireframeMeshCap() }, calculateBounds);

  _transparent_end_caps[0] =
    std::make_unique<ShapeCache>(culler, _transparent_cache->shader(), Part{ solidMeshCapTop() }, calculateBounds);
  _transparent_end_caps[1] =
    std::make_unique<ShapeCache>(culler, _transparent_cache->shader(), Part{ solidMeshCapBottom() }, calculateBounds);

  const auto top_cap_modifier = [](Magnum::Matrix4 &transform) { return endCapTransformModifier(transform, true); };
  const auto bottom_cap_modifier = [](Magnum::Matrix4 &transform) { return endCapTransformModifier(transform, false); };

  _solid_end_caps[0]->setTransformModifier(top_cap_modifier);
  _solid_end_caps[1]->setTransformModifier(bottom_cap_modifier);

  _wireframe_end_caps[0]->setTransformModifier(top_cap_modifier);
  _wireframe_end_caps[1]->setTransformModifier(bottom_cap_modifier);

  _transparent_end_caps[0]->setTransformModifier(top_cap_modifier);
  _transparent_end_caps[1]->setTransformModifier(bottom_cap_modifier);
}


void Capsule::reset()
{
  static_assert(sizeof(_solid_end_caps) == sizeof(_wireframe_end_caps));
  static_assert(sizeof(_solid_end_caps) == sizeof(_transparent_end_caps));
  for (size_t i = 0; i < _solid_end_caps.size(); ++i)
  {
    _solid_end_caps[i]->clear();
    _wireframe_end_caps[i]->clear();
    _transparent_end_caps[i]->clear();
  }
  ShapePainter::reset();
}


bool Capsule::update(const Id &id, const Magnum::Matrix4 &transform, const Magnum::Color4 &colour)
{
  const auto search = _id_index_map.find(id);
  if (search != _id_index_map.end())
  {
    if (ShapeCache *cache = cacheForType(search->second.type))
    {
      cache->update(search->second.index, transform, colour);
    }

    if (std::array<std::unique_ptr<ShapeCache>, 2> *end_caches = endCapCachesForType(search->second.type))
    {
      for (size_t i = 0; i < end_caches->size(); ++i)
      {
        (*end_caches)[i]->update(search->second.index, transform, colour);
      }
    }

    return true;
  }

  return false;
}


bool Capsule::remove(const Id &id)
{
  const auto search = _id_index_map.find(id);
  if (search != _id_index_map.end())
  {
    if (ShapeCache *cache = cacheForType(search->second.type))
    {
      cache->endShape(search->second.index);
    }
    if (std::array<std::unique_ptr<ShapeCache>, 2> *end_caches = endCapCachesForType(search->second.type))
    {
      for (auto &cache : *end_caches)
      {
        cache->endShape(search->second.index);
      }
    }
    return true;
  }

  return false;
}


void Capsule::drawOpaque(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix,
                         const Magnum::Matrix4 &view_matrix)
{
  _solid_cache->draw(stamp, projection_matrix, view_matrix);
  _solid_end_caps[0]->draw(stamp, projection_matrix, view_matrix);
  _solid_end_caps[1]->draw(stamp, projection_matrix, view_matrix);

  _wireframe_cache->draw(stamp, projection_matrix, view_matrix);
  _wireframe_end_caps[0]->draw(stamp, projection_matrix, view_matrix);
  _wireframe_end_caps[1]->draw(stamp, projection_matrix, view_matrix);
}


void Capsule::drawTransparent(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix,
                              const Magnum::Matrix4 &view_matrix)
{
  Magnum::GL::Renderer::setBlendFunction(Magnum::GL::Renderer::BlendFunction::SourceAlpha,
                                         Magnum::GL::Renderer::BlendFunction::OneMinusSourceAlpha);
  _transparent_cache->draw(stamp, projection_matrix, view_matrix);
  _transparent_end_caps[0]->draw(stamp, projection_matrix, view_matrix);
  _transparent_end_caps[1]->draw(stamp, projection_matrix, view_matrix);
  Magnum::GL::Renderer::setBlendFunction(Magnum::GL::Renderer::BlendFunction::One,
                                         Magnum::GL::Renderer::BlendFunction::Zero);
}


void Capsule::commit()
{
  static_assert(sizeof(_solid_end_caps) == sizeof(_wireframe_end_caps));
  static_assert(sizeof(_solid_end_caps) == sizeof(_transparent_end_caps));
  for (size_t i = 0; i < _solid_end_caps.size(); ++i)
  {
    _solid_end_caps[i]->commit();
    _wireframe_end_caps[i]->commit();
    _transparent_end_caps[i]->commit();
  }

  ShapePainter::commit();
}


void Capsule::calculateBounds(const Magnum::Matrix4 &transform, Bounds &bounds)
{
  return ShapeCache::calcCylindricalBounds(transform, kDefaultRadius, kDefaultHeight + kDefaultRadius, bounds);
}


std::array<std::unique_ptr<ShapeCache>, 2> *Capsule::endCapCachesForType(Type type)
{
  switch (type)
  {
  case Type::Solid:
    return &_solid_end_caps;
  case Type::Transparent:
    return &_transparent_end_caps;
  case Type::Wireframe:
    return &_wireframe_end_caps;
  default:
    break;
  }
  return nullptr;
}


void Capsule::endCapTransformModifier(Magnum::Matrix4 &transform, bool positive)
{
  // Remove
  Magnum::Vector4 z_basis = transform[2];
  float x_scale = transform[0].xyz().length();
  float z_scale = z_basis.xyz().length();
  float z_scale_inv = (z_scale > 1e-6f) ? 1.0f / z_scale : z_scale;
  z_basis *= x_scale * z_scale_inv;
  z_basis[3] = 0;
  transform[2] = z_basis;

  const Magnum::Matrix3 rotation = transform.rotation();
  const Magnum::Vector3 axis = rotation * Magnum::Vector3{ 0, 0, 0.5f * z_scale * kDefaultHeight };
  transform[3] += Magnum::Float(positive ? 1 : -1) * Magnum::Vector4(axis, 0.0f);
}


Magnum::GL::Mesh Capsule::solidMeshCylinder()
{
  static SimpleMesh build_mesh(0, 0, 0, DtTriangles, SimpleMesh::Vertex | SimpleMesh::Normal | SimpleMesh::Index);
  static std::mutex guard;

  std::unique_lock<std::mutex> lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    std::vector<tes::Vector3f> vertices;
    std::vector<tes::Vector3f> normals;
    std::vector<unsigned> indices;

    tes::cylinder::solid(vertices, indices, normals, Vector3f(0, 0, 1), kDefaultHeight, kDefaultRadius, 24, true);

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setNormals(0, normals.data(), normals.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}


Magnum::GL::Mesh Capsule::wireframeMeshCylinder()
{
  return Cylinder::wireframeMesh();
}


Magnum::GL::Mesh Capsule::solidMeshCapTop()
{
  static SimpleMesh build_mesh(0, 0, 0, DtTriangles, SimpleMesh::Vertex | SimpleMesh::Normal | SimpleMesh::Index);
  static std::mutex guard;

  std::unique_lock<std::mutex> lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    buildEndCapSolid(build_mesh, false);
  }

  return mesh::convert(build_mesh);
}


Magnum::GL::Mesh Capsule::solidMeshCapBottom()
{
  static SimpleMesh build_mesh(0, 0, 0, DtTriangles, SimpleMesh::Vertex | SimpleMesh::Normal | SimpleMesh::Index);
  static std::mutex guard;

  std::unique_lock<std::mutex> lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    buildEndCapSolid(build_mesh, true);
  }

  return mesh::convert(build_mesh);
}


Magnum::GL::Mesh Capsule::wireframeMeshCap()
{
  return Sphere::wireframeMesh();
}


util::ResourceListId Capsule::addShape(const Id &shape_id, Type type, const Magnum::Matrix4 &transform,
                                       const Magnum::Color4 &colour, bool hidden, const ParentId &parent_id,
                                       unsigned *child_index)
{
  // Add as is for the cylinder part.
  util::ResourceListId index =
    ShapePainter::addShape(shape_id, type, transform, colour, hidden, parent_id, child_index);
  std::array<std::unique_ptr<ShapeCache>, 2> *end_caches = endCapCachesForType(type);
  if (index == ~0u || !end_caches)
  {
    return index;
  }

  ShapeCache::ShapeFlag flags = ShapeCache::ShapeFlag::None;
  flags |= (shape_id.isTransient()) ? ShapeCache::ShapeFlag::Transient : ShapeCache::ShapeFlag::None;
  flags |= (hidden) ? ShapeCache::ShapeFlag::Hidden : ShapeCache::ShapeFlag::None;
  for (size_t i = 0; i < end_caches->size(); ++i)
  {
    (*end_caches)[i]->add(shape_id, transform, colour, flags, parent_id.resourceId(), nullptr);
  }
  return index;
}


void Capsule::buildEndCapSolid(SimpleMesh &mesh, bool bottomCap)
{
  std::vector<Vector3f> vertices;
  std::vector<Vector3f> normals;
  std::vector<unsigned> indices;

  tes::sphere::solidLatLong(vertices, indices, normals, kDefaultRadius, Vector3f(0.0f), 4, 24,
                            Vector3f(0, 0, (bottomCap) ? -1.0f : 1.0f), true);

  mesh.setVertexCount(vertices.size());
  mesh.setIndexCount(indices.size());

  mesh.setVertices(0, vertices.data(), vertices.size());
  mesh.setNormals(0, normals.data(), normals.size());
  mesh.setIndices(0, indices.data(), indices.size());
}
}  // namespace tes::view::painter
