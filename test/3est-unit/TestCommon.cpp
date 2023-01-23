//
// author: Kazys Stepanas
//
#include "TestCommon.h"

#include <3escore/shapes/Shapes.h>
#include <3escore/shapes/SimpleMesh.h>
#include <3escore/tessellate/Sphere.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <functional>

namespace tes
{
void makeHiResSphere(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> *normals)
{
  makeSphere(vertices, indices, normals, 5);
}

void makeLowResSphere(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> *normals)
{
  makeSphere(vertices, indices, normals, 0);
}

void makeSphere(std::vector<Vector3f> &vertices, std::vector<unsigned> &indices, std::vector<Vector3f> *normals,
                int iterations)
{
  // Start with a unit sphere so we have normals precalculated.
  // Use a fine subdivision to ensure we need multiple data packets to transfer vertices.
  sphere::solid(vertices, indices, 1.0f, Vector3f::zero, iterations);

  // Normals as vertices. Scale and offset.
  if (normals)
  {
    normals->resize(vertices.size());
    for (size_t i = 0; i < normals->size(); ++i)
    {
      (*normals)[i] = vertices[i];
    }
  }

  const float radius = 5.5f;
  const Vector3f sphereCentre(0.5f, 0, -0.25f);
  for (size_t i = 0; i < vertices.size(); ++i)
  {
    vertices[i] = sphereCentre + vertices[i] * radius;
  }
}

// Validate a mesh resource.
void validateMesh(const MeshResource &mesh, const MeshResource &reference)
{
  // Check members.
  EXPECT_EQ(mesh.id(), reference.id());
  EXPECT_EQ(mesh.typeId(), reference.typeId());
  EXPECT_EQ(mesh.uniqueKey(), reference.uniqueKey());

  EXPECT_TRUE(mesh.transform().isEqual(reference.transform()));
  EXPECT_EQ(mesh.tint(), reference.tint());
  EXPECT_EQ(mesh.vertexCount(), reference.vertexCount());
  EXPECT_EQ(mesh.indexCount(), reference.indexCount());

  // Check vertices and vertex related components.
  if (reference.vertexCount() && mesh.vertexCount() == reference.vertexCount())
  {
    DataBuffer meshVerts = mesh.vertices();
    DataBuffer refVerts = reference.vertices();

    ASSERT_TRUE(meshVerts.isValid());
    ASSERT_TRUE(refVerts.isValid());
    ASSERT_EQ(meshVerts.count(), refVerts.count());

    for (unsigned i = 0; i < mesh.vertexCount(); ++i)
    {
      const Vector3f meshVert(meshVerts.get<float>(i, 0), meshVerts.get<float>(i, 1), meshVerts.get<float>(i, 2));
      const Vector3f refVert(meshVerts.get<float>(i, 0), refVerts.get<float>(i, 1), refVerts.get<float>(i, 2));
      if (meshVert[0] != refVert[0] || meshVert[1] != refVert[1] || meshVert[2] != refVert[2])
      {
        FAIL() << "vertex[" << i << "]: (" << meshVert[0] << ',' << meshVert[1] << ',' << meshVert[2] << ") != ("
               << refVert[0] << ',' << refVert[1] << ',' << refVert[2] << ")";
      }
    }

    // Check normals.
    if (reference.normals().isValid())
    {
      ASSERT_TRUE(mesh.normals().isValid()) << "Mesh missing normals.";

      DataBuffer meshNorms = mesh.normals();
      DataBuffer refNorms = reference.normals();

      ASSERT_EQ(meshNorms.count(), refNorms.count());

      for (unsigned i = 0; i < meshNorms.count(); ++i)
      {
        const Vector3f meshNorm(meshNorms.get<float>(i, 0), meshNorms.get<float>(i, 1), meshNorms.get<float>(i, 2));
        const Vector3f refNorm(meshNorms.get<float>(i, 0), refNorms.get<float>(i, 1), refNorms.get<float>(i, 2));
        if (meshNorm[0] != refNorm[0] || meshNorm[1] != refNorm[1] || meshNorm[2] != refNorm[2])
        {
          FAIL() << "normal[" << i << "]: (" << meshNorm[0] << ',' << meshNorm[1] << ',' << meshNorm[2] << ") != ("
                 << refNorm[0] << ',' << refNorm[1] << ',' << refNorm[2] << ")";
        }
      }
    }

    // Check colours.
    if (reference.colours().isValid())
    {
      ASSERT_TRUE(mesh.colours().isValid()) << "Mesh missing colours.";

      DataBuffer meshColours = mesh.colours();
      DataBuffer refColours = reference.colours();

      ASSERT_EQ(meshColours.count(), refColours.count());

      for (unsigned i = 0; i < meshColours.count(); ++i)
      {
        if (meshColours.get<uint32_t>(i) != refColours.get<uint32_t>(i))
        {
          FAIL() << "colour[" << i << "]: 0x" << std::hex << std::setw(8) << std::setfill('0')
                 << meshColours.get<uint32_t>(i) << " != 0x" << refColours.get<uint32_t>(i) << std::dec << std::setw(0)
                 << std::setfill(' ');
        }
      }
    }

    // Check UVs.
    if (reference.uvs().isValid())
    {
      ASSERT_TRUE(mesh.uvs().isValid()) << "Mesh missing UVs.";

      DataBuffer meshUVs = mesh.uvs();
      DataBuffer refUVs = reference.uvs();

      ASSERT_EQ(meshUVs.count(), refUVs.count());

      for (unsigned i = 0; i < meshUVs.count(); ++i)
      {
        const float meshUV[2] = { meshUVs.get<float>(i, 1), meshUVs.get<float>(i, 2) };
        const float refUV[2] = { refUVs.get<float>(i, 1), refUVs.get<float>(i, 2) };
        if (meshUV[0] != refUV[0] || meshUV[1] != refUV[1])
        {
          FAIL() << "uv[" << i << "]: (" << meshUV[0] << ',' << meshUV[1] << ") != (" << refUV[0] << ',' << refUV[1]
                 << ")";
        }
      }
    }
  }

  // Check indices.
  if (reference.indexCount() && mesh.indexCount() == reference.indexCount())
  {
    DataBuffer meshInds = mesh.indices();
    DataBuffer refInds = reference.indices();

    ASSERT_TRUE(meshInds.isValid());
    ASSERT_TRUE(refInds.isValid());

    ASSERT_EQ(meshInds.count(), refInds.count());

    // Handle index widths.
    for (unsigned i = 0; i < meshInds.count(); ++i)
    {
      EXPECT_EQ(meshInds.get<uint32_t>(i), refInds.get<uint32_t>(i));
    }
  }
}


void validateShape(const Shape &shape, const Shape &reference, const ResourceMap &resources)
{
  TES_UNUSED(resources);
  EXPECT_EQ(shape.routingId(), reference.routingId());
  EXPECT_EQ(shape.isComplex(), reference.isComplex());

  EXPECT_EQ(shape.data().id, reference.data().id);
  EXPECT_EQ(shape.data().category, reference.data().category);
  EXPECT_EQ(shape.data().flags, reference.data().flags);
  EXPECT_EQ(shape.data().reserved, reference.data().reserved);

  EXPECT_EQ(shape.attributes().colour, reference.attributes().colour);

  EXPECT_EQ(shape.attributes().position[0], reference.attributes().position[0]);
  EXPECT_EQ(shape.attributes().position[1], reference.attributes().position[1]);
  EXPECT_EQ(shape.attributes().position[2], reference.attributes().position[2]);

  EXPECT_EQ(shape.attributes().rotation[0], reference.attributes().rotation[0]);
  EXPECT_EQ(shape.attributes().rotation[1], reference.attributes().rotation[1]);
  EXPECT_EQ(shape.attributes().rotation[2], reference.attributes().rotation[2]);
  EXPECT_EQ(shape.attributes().rotation[3], reference.attributes().rotation[3]);

  EXPECT_EQ(shape.attributes().scale[0], reference.attributes().scale[0]);
  EXPECT_EQ(shape.attributes().scale[1], reference.attributes().scale[1]);
  EXPECT_EQ(shape.attributes().scale[2], reference.attributes().scale[2]);
}


template <typename T>
void validateText(const T &shape, const T &reference, const ResourceMap &resources)
{
  validateShape(static_cast<const Shape>(shape), static_cast<const Shape>(reference), resources);
  EXPECT_EQ(shape.textLength(), reference.textLength());
  EXPECT_STREQ(shape.text(), reference.text());
}


void validateShape(const Text2D &shape, const Text2D &reference, const ResourceMap &resources)
{
  validateText(shape, reference, resources);
}


void validateShape(const Text3D &shape, const Text3D &reference, const ResourceMap &resources)
{
  validateText(shape, reference, resources);
}


void validateShape(const MeshShape &shape, const MeshShape &reference, const ResourceMap &resources)
{
  validateShape(static_cast<const Shape>(shape), static_cast<const Shape>(reference), resources);

  EXPECT_EQ(shape.drawType(), reference.drawType());
  if (reference.vertices().count())
  {
    EXPECT_EQ(shape.vertices().count(), reference.vertices().count());
    EXPECT_EQ(shape.vertices().componentCount(), reference.vertices().componentCount());
    EXPECT_EQ(shape.vertices().elementStride(), reference.vertices().elementStride());
  }
  if (reference.indices().count())
  {
    EXPECT_EQ(shape.indices().count(), reference.indices().count());
    EXPECT_EQ(shape.indices().componentCount(), reference.indices().componentCount());
    EXPECT_EQ(shape.indices().elementStride(), reference.indices().elementStride());
  }
  if (reference.normals().count())
  {
    EXPECT_EQ(shape.normals().count(), reference.normals().count());
    EXPECT_EQ(shape.normals().componentCount(), reference.normals().componentCount());
    EXPECT_EQ(shape.normals().elementStride(), reference.normals().elementStride());
  }
  if (reference.colours().count())
  {
    EXPECT_EQ(shape.colours().count(), reference.colours().count());
    EXPECT_EQ(shape.colours().componentCount(), reference.colours().componentCount());
    EXPECT_EQ(shape.colours().elementStride(), reference.colours().elementStride());
  }

  // Validate vertices.
  Vector3f v, r;
  if (shape.vertices().count() == reference.vertices().count() && shape.vertices().count())
  {
    ASSERT_TRUE(shape.vertices().isValid());
    ASSERT_TRUE(reference.vertices().isValid());
    for (unsigned i = 0; i < shape.vertices().count(); ++i)
    {
      v = Vector3f(shape.vertices().ptr<float>(i));
      r = Vector3f(reference.vertices().ptr<float>(i));

      if (v != r)
      {
        std::cerr << "Vertex mismatch at " << i << '\n';
        EXPECT_EQ(v, r);
      }
    }
  }

  if (shape.indices().count() == reference.indices().count() && shape.indices().count())
  {
    ASSERT_NE(shape.indices().ptr<uint32_t>(), nullptr);
    ASSERT_NE(reference.indices().ptr<uint32_t>(), nullptr);
    unsigned is, ir;
    for (unsigned i = 0; i < shape.indices().count(); ++i)
    {
      is = *shape.indices().ptr<uint32_t>(i);
      ir = *reference.indices().ptr<uint32_t>(i);

      if (is != ir)
      {
        std::cerr << "Index mismatch at " << i << '\n';
        EXPECT_EQ(is, ir);
      }
    }
  }

  if (shape.normals().count() == reference.normals().count() && shape.normals().count())
  {
    ASSERT_TRUE(shape.vertices().isValid());
    ASSERT_TRUE(reference.vertices().isValid());
    for (unsigned i = 0; i < shape.normals().count(); ++i)
    {
      v = Vector3f(shape.normals().ptr<float>(i));
      r = Vector3f(reference.normals().ptr<float>(i));

      if (v != r)
      {
        std::cerr << "Normal mismatch at " << i << '\n';
        EXPECT_EQ(v, r);
      }
    }
  }

  if (shape.colours().count() == reference.colours().count() && reference.colours().count() &&
      shape.colours().count() == shape.vertices().count())
  {
    Colour cs, cr;
    for (unsigned i = 0; i < shape.colours().count(); ++i)
    {
      cs = *shape.colours().ptr<uint32_t>(i);
      cr = *reference.colours().ptr<uint32_t>(i);

      if (cs != cr)
      {
        std::cerr << "Colour mismatch at " << i << '\n';
        EXPECT_EQ(cs, cr);
      }
    }
  }
}


void validateShape(const PointCloudShape &shape, const PointCloudShape &reference, const ResourceMap &resources)
{
  validateShape(static_cast<const Shape>(shape), static_cast<const Shape>(reference), resources);

  EXPECT_EQ(shape.pointScale(), reference.pointScale());
  EXPECT_EQ(shape.indexCount(), reference.indexCount());

  // Note: We can't compare the contents of shape.mesh() as it is a placeholder reference.
  // The real mesh is received and validated separately.
  ASSERT_NE(shape.mesh(), nullptr);
  ASSERT_NE(reference.mesh(), nullptr);
  EXPECT_EQ(shape.mesh()->id(), reference.mesh()->id());
  EXPECT_EQ(shape.mesh()->typeId(), reference.mesh()->typeId());
  EXPECT_EQ(shape.mesh()->uniqueKey(), reference.mesh()->uniqueKey());

  if (shape.indexCount() == reference.indexCount())
  {
    for (unsigned i = 0; i < shape.indexCount(); ++i)
    {
      EXPECT_EQ(shape.indices()[i], reference.indices()[i]);
    }
  }

  // Validate resources. Fetch the transferred resource and compare against the reference resource.
  auto resIter = resources.find(shape.mesh()->uniqueKey());
  ASSERT_NE(resIter, resources.end());
  ASSERT_EQ(resIter->second->typeId(), reference.mesh()->typeId());

  const MeshResource *mesh = static_cast<const MeshResource *>(resIter->second);
  validateMesh(*mesh, *reference.mesh());
}


void validateShape(const MeshSet &shape, const MeshSet &reference, const ResourceMap &resources)
{
  validateShape(static_cast<const Shape>(shape), static_cast<const Shape>(reference), resources);

  EXPECT_EQ(shape.partCount(), reference.partCount());

  for (unsigned i = 0; i < std::min(shape.partCount(), reference.partCount()); ++i)
  {
    // Remember, the mesh in shape is only a placeholder for the ID. The real mesh is in resources.
    // Validate resources. Fetch the transferred resource and compare against the reference resource.
    auto resIter = resources.find(shape.partResource(i)->uniqueKey());
    ASSERT_NE(resIter, resources.end());
    ASSERT_EQ(resIter->second->typeId(), reference.partResource(i)->typeId());

    const MeshResource *part = static_cast<const MeshResource *>(resIter->second);
    const MeshResource *refPart = reference.partResource(i);

    EXPECT_TRUE(shape.partTransform(i).isEqual(reference.partTransform(i)));
    EXPECT_EQ(shape.partColour(i), reference.partColour(i));
    validateMesh(*part, *refPart);
  }
}
}  // namespace tes
