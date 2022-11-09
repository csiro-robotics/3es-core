#include "3escapsule.h"

#include "3escylinder.h"
#include "3essphere.h"

#include "mesh/3esconverter.h"

#include <shapes/3essimplemesh.h>
#include <tessellate/3escylinder.h>
#include <tessellate/3essphere.h>

#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Matrix3.h>

#include <cassert>
#include <mutex>

namespace tes::viewer::painter
{
constexpr float Capsule::kDefaultRadius;
constexpr float Capsule::kDefaultHeight;
const Vector3f Capsule::kDefaultAxis = { 0.0f, 0.0f, 1.0f };

Capsule::Capsule(std::shared_ptr<BoundsCuller> culler)
  : ShapePainter(culler, { Part{ solidMeshCylinder() } }, { Part{ wireframeMeshCylinder() } },
                 { Part{ solidMeshCylinder() } }, calculateBounds)
{
  _solid_end_caps[0] =
    std::make_unique<ShapeCache>(culler, Part{ solidMeshCapTop() }, _solid_cache->shader(), calculateBounds);
  _solid_end_caps[1] =
    std::make_unique<ShapeCache>(culler, Part{ solidMeshCapBottom() }, _solid_cache->shader(), calculateBounds);

  _wireframe_end_caps[0] =
    std::make_unique<ShapeCache>(culler, Part{ wireframeMeshCap() }, _wireframe_cache->shader(), calculateBounds);
  _wireframe_end_caps[1] =
    std::make_unique<ShapeCache>(culler, Part{ wireframeMeshCap() }, _wireframe_cache->shader(), calculateBounds);

  _transparent_end_caps[0] =
    std::make_unique<ShapeCache>(culler, Part{ solidMeshCapTop() }, _transparent_cache->shader(), calculateBounds);
  _transparent_end_caps[1] =
    std::make_unique<ShapeCache>(culler, Part{ solidMeshCapBottom() }, _transparent_cache->shader(), calculateBounds);
}

void Capsule::add(const Id &id, Type type, const Magnum::Matrix4 &transform, const Magnum::Color4 &colour)
{
  // Add as is for the cylinder part.
  ShapePainter::add(id, type, transform, colour);
  std::array<std::unique_ptr<ShapeCache>, 2> *end_caches = endCapCachesForType(type);
  if (!end_caches)
  {
    return;
  }
  // Find what we just added to add to the end cap caches.
  const auto search = _id_index_map.find(id);
  if (search != _id_index_map.end())
  {
    // Modify the transform for the end caps. We change the Z scale to Z translation, then match Z scale to X/Y.
    // This makes the spherical end caps position and scale correctly.
    const auto end_transforms = calcEndCapTransforms(transform);
    for (size_t i = 0; i < end_transforms.size(); ++i)
    {
      (*end_caches)[i]->add(end_transforms[i], colour);
    }
  }
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
      const auto end_transforms = calcEndCapTransforms(transform);
      for (size_t i = 0; i < end_transforms.size(); ++i)
      {
        (*end_caches)[i]->add(end_transforms[i], colour);
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
      cache->remove(search->second.index);
    }
    if (std::array<std::unique_ptr<ShapeCache>, 2> *end_caches = endCapCachesForType(search->second.type))
    {
      for (auto &cache : *end_caches)
      {
        cache->remove(search->second.index);
      }
    }
    return true;
  }

  return false;
}


void Capsule::drawOpaque(unsigned render_mark, const Magnum::Matrix4 &projection_matrix)
{
  _solid_cache->draw(render_mark, projection_matrix);
  _solid_end_caps[0]->draw(render_mark, projection_matrix);
  _solid_end_caps[1]->draw(render_mark, projection_matrix);

  _wireframe_cache->draw(render_mark, projection_matrix);
  _wireframe_end_caps[0]->draw(render_mark, projection_matrix);
  _wireframe_end_caps[1]->draw(render_mark, projection_matrix);
}


void Capsule::drawTransparent(unsigned render_mark, const Magnum::Matrix4 &projection_matrix)
{
  Magnum::GL::Renderer::setBlendFunction(Magnum::GL::Renderer::BlendFunction::SourceAlpha,
                                         Magnum::GL::Renderer::BlendFunction::OneMinusSourceAlpha);
  _transparent_cache->draw(render_mark, projection_matrix);
  _transparent_end_caps[0]->draw(render_mark, projection_matrix);
  _transparent_end_caps[1]->draw(render_mark, projection_matrix);
  Magnum::GL::Renderer::setBlendFunction(Magnum::GL::Renderer::BlendFunction::One,
                                         Magnum::GL::Renderer::BlendFunction::Zero);
}


void Capsule::calculateBounds(const Magnum::Matrix4 &transform, Magnum::Vector3 &centre, Magnum::Vector3 &halfExtents)
{
  return ShapeCache::calcCylindricalBounds(transform, kDefaultRadius, kDefaultHeight + kDefaultRadius, centre,
                                           halfExtents);
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


std::array<Magnum::Matrix4, 2> Capsule::calcEndCapTransforms(const Magnum::Matrix4 &transform)
{
  // Modify the transform for the end caps. We change the Z scale to Z translation, then match Z scale to X/Y.
  // This makes the spherical end caps position and scale correctly.
  std::array<Magnum::Matrix4, 2> cap_transforms = { transform, transform };
  Magnum::Vector4 z_vec = transform[2];
  float x_scale = transform[0].xyz().length();
  float z_scale = z_vec.xyz().length();
  float z_scale_inv = (z_scale > 1e-6f) ? 1.0f / z_scale : z_scale;
  z_vec[0] *= x_scale * z_scale_inv;
  z_vec[1] *= x_scale * z_scale_inv;
  z_vec[2] *= x_scale * z_scale_inv;
  cap_transforms[0][2] = z_vec;
  cap_transforms[1][2] = z_vec;

  const Magnum::Matrix3 rotation = transform.rotation();
  const Magnum::Vector3 axis = rotation * Magnum::Vector3{ 0, 0, z_scale * 0.5f * kDefaultHeight };

  cap_transforms[0][3] += Magnum::Vector4(axis, 0.0f);
  cap_transforms[1][3] -= Magnum::Vector4(axis, 0.0f);
  return cap_transforms;
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

    tes::cylinder::solid(vertices, indices, normals, Vector3f(0, 0, 1), 1.0f, 1.0f, 24, true);

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
}  // namespace tes::viewer::painter
