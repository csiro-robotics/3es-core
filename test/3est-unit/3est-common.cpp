//
// author: Kazys Stepanas
//
#include "3est-common.h"

#include <shapes/3esshapes.h>
#include <shapes/3essimplemesh.h>
#include <tessellate/3essphere.h>

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
  unsigned meshStride, refStride = 0;
  if (reference.vertexCount() && mesh.vertexCount() == reference.vertexCount())
  {
    const float *meshVerts = mesh.vertices(meshStride);
    const float *refVerts = reference.vertices(refStride);

    ASSERT_NE(meshVerts, nullptr);
    ASSERT_NE(refVerts, nullptr);

    for (unsigned i = 0; i < mesh.vertexCount(); ++i)
    {
      if (meshVerts[0] != refVerts[0] || meshVerts[1] != refVerts[1] || meshVerts[2] != refVerts[2])
      {
        FAIL() << "vertex[" << i << "]: (" << meshVerts[0] << ',' << meshVerts[1] << ',' << meshVerts[2] << ") != ("
               << refVerts[0] << ',' << refVerts[1] << ',' << refVerts[2] << ")";
      }

      meshVerts += meshStride / sizeof(float);
      refVerts += refStride / sizeof(float);
    }

    // Check normals.
    if (reference.normals(refStride))
    {
      ASSERT_TRUE(mesh.normals(meshStride)) << "Mesh missing normals.";

      const float *meshNorms = mesh.normals(meshStride);
      const float *refNorms = reference.normals(refStride);

      for (unsigned i = 0; i < mesh.vertexCount(); ++i)
      {
        if (meshNorms[0] != refNorms[0] || meshNorms[1] != refNorms[1] || meshNorms[2] != refNorms[2])
        {
          FAIL() << "normal[" << i << "]: (" << meshNorms[0] << ',' << meshNorms[1] << ',' << meshNorms[2] << ") != ("
                 << refNorms[0] << ',' << refNorms[1] << ',' << refNorms[2] << ")";
        }

        meshNorms += meshStride / sizeof(float);
        refNorms += refStride / sizeof(float);
      }
    }

    // Check colours.
    if (reference.colours(refStride))
    {
      ASSERT_TRUE(mesh.colours(meshStride)) << "Mesh missing colours.";

      const uint32_t *meshColours = mesh.colours(meshStride);
      const uint32_t *refColours = reference.colours(refStride);

      for (unsigned i = 0; i < mesh.vertexCount(); ++i)
      {
        if (*meshColours != *refColours)
        {
          FAIL() << "colour[" << i << "]: 0x" << std::hex << std::setw(8) << std::setfill('0') << *meshColours
                 << " != 0x" << *refColours << std::dec << std::setw(0) << std::setfill(' ');
        }

        meshColours += meshStride / sizeof(uint32_t);
        refColours += refStride / sizeof(uint32_t);
      }
    }

    // Check UVs.
    if (reference.uvs(refStride))
    {
      ASSERT_TRUE(mesh.uvs(meshStride)) << "Mesh missing UVs.";

      const float *meshUVs = mesh.uvs(meshStride);
      const float *refUVs = reference.uvs(refStride);

      for (unsigned i = 0; i < mesh.vertexCount(); ++i)
      {
        if (meshUVs[0] != refUVs[0] || meshUVs[1] != refUVs[1])
        {
          FAIL() << "uv[" << i << "]: (" << meshUVs[0] << ',' << meshUVs[1] << ") != (" << refUVs[0] << ',' << refUVs[1]
                 << ")";
        }

        meshUVs += meshStride / sizeof(float);
        refUVs += refStride / sizeof(float);
      }
    }
  }

  // Check indices.
  if (reference.indexCount() && mesh.indexCount() == reference.indexCount())
  {
    unsigned meshWidth = 0, refWidth = 0;
    const uint8_t *meshInds = mesh.indices(meshStride, meshWidth);
    const uint8_t *refInds = reference.indices(refStride, refWidth);

    ASSERT_NE(meshInds, nullptr);
    ASSERT_NE(refInds, nullptr);

    // Handle index widths.
    std::function<unsigned(const uint8_t *)> meshGetIndex, refGetIndex;

    auto getIndex1 = [](const uint8_t *mem) { return unsigned(*mem); };
    auto getIndex2 = [](const uint8_t *mem) { return unsigned(*reinterpret_cast<const uint16_t *>(mem)); };
    auto getIndex4 = [](const uint8_t *mem) { return unsigned(*reinterpret_cast<const uint32_t *>(mem)); };

    switch (meshWidth)
    {
    case 1:
      meshGetIndex = getIndex1;
      break;
    case 2:
      meshGetIndex = getIndex2;
      break;
    case 4:
      meshGetIndex = getIndex4;
      break;
    default:
      ASSERT_TRUE(false) << "Unexpected index width.";
    }

    switch (refWidth)
    {
    case 1:
      refGetIndex = getIndex1;
      break;
    case 2:
      refGetIndex = getIndex2;
      break;
    case 4:
      refGetIndex = getIndex4;
      break;
    default:
      ASSERT_TRUE(false) << "Unexpected index width.";
    }

    for (unsigned i = 0; i < mesh.indexCount(); ++i)
    {
      EXPECT_EQ(meshGetIndex(meshInds), refGetIndex(refInds));

      meshInds += meshStride;
      refInds += refStride;
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
  EXPECT_EQ(shape.vertexCount(), reference.vertexCount());
  EXPECT_EQ(shape.vertexStride(), reference.vertexStride());
  EXPECT_EQ(shape.normalsCount(), reference.normalsCount());
  EXPECT_EQ(shape.normalsStride(), reference.normalsStride());
  EXPECT_EQ(shape.indexCount(), reference.indexCount());

  // Validate vertices.
  Vector3f v, r;
  if (shape.vertexCount() == reference.vertexCount() && shape.vertexCount())
  {
    ASSERT_NE(shape.vertices(), nullptr);
    ASSERT_NE(reference.vertices(), nullptr);
    for (unsigned i = 0; i < shape.vertexCount(); ++i)
    {
      v = Vector3f(shape.vertices() + i * shape.vertexStride());
      r = Vector3f(reference.vertices() + i * reference.vertexStride());

      if (v != r)
      {
        std::cerr << "Vertex mismatch at " << i << '\n';
        EXPECT_EQ(v, r);
      }
    }
  }

  if (shape.indexCount() == reference.indexCount() && shape.indexCount())
  {
    ASSERT_NE(shape.indices(), nullptr);
    ASSERT_NE(reference.indices(), nullptr);
    unsigned is, ir;
    for (unsigned i = 0; i < shape.indexCount(); ++i)
    {
      is = shape.indices()[i];
      ir = reference.indices()[i];

      if (is != ir)
      {
        std::cerr << "Index mismatch at " << i << '\n';
        EXPECT_EQ(is, ir);
      }
    }
  }

  if (shape.normalsCount() == reference.normalsCount() && shape.normalsCount())
  {
    ASSERT_NE(shape.normals(), nullptr);
    ASSERT_NE(reference.normals(), nullptr);
    for (unsigned i = 0; i < shape.normalsCount(); ++i)
    {
      v = Vector3f(shape.normals() + i * shape.normalsStride());
      r = Vector3f(reference.normals() + i * reference.normalsStride());

      if (v != r)
      {
        std::cerr << "Normal mismatch at " << i << '\n';
        EXPECT_EQ(v, r);
      }
    }
  }

  if (reference.colours())
  {
    ASSERT_NE(shape.colours(), nullptr);
  }

  if (shape.vertexCount() == reference.vertexCount() && reference.colours())
  {
    Colour cs, cr;
    for (unsigned i = 0; i < shape.vertexCount(); ++i)
    {
      cs = shape.colours()[i];
      cr = reference.colours()[i];

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
