//
// Author: Kazys Stepanas
//
#include "VoxelGeom.h"

#include <3escore/Log.h>

#include <Magnum/GL/Context.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include "Magnum/GL/Shader.h"
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Color.h>

#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/Assert.h>

namespace tes::viewer::shaders
{
VoxelGeom::VoxelGeom()
  : _shader(std::make_shared<VoxelGeomProgram>())
{
  _shader->setTint(Magnum::Color4(1, 1, 1, 1));
  _shader->setVoxelScale(Magnum::Vector3(0.1f));
}


VoxelGeom::~VoxelGeom() = default;


std::shared_ptr<Magnum::GL::AbstractShaderProgram> VoxelGeom::shader() const
{
  return _shader;
}


Shader &VoxelGeom::setProjectionMatrix(const Magnum::Matrix4 &matrix)
{
  _pvm.setProjection(matrix);
  return *this;
}


Shader &VoxelGeom::setViewMatrix(const Magnum::Matrix4 &matrix)
{
  _pvm.setView(matrix);
  return *this;
}


Shader &VoxelGeom::setModelMatrix(const Magnum::Matrix4 &matrix)
{
  _pvm.setModel(matrix);
  return *this;
}


Shader &VoxelGeom::setColour(const Magnum::Color4 &colour)
{
  _shader->setTint(colour);
  return *this;
}

Shader &VoxelGeom::setDrawScale(float scale)
{
  _shader->setVoxelScale(Magnum::Vector3(scale));
  return *this;
}

Shader &VoxelGeom::draw(Magnum::GL::Mesh &mesh)
{
  updateTransform();
  _shader->draw(mesh);
  return *this;
}

Shader &VoxelGeom::draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer, size_t instance_count)
{
  (void)mesh;
  (void)buffer;
  (void)instance_count;
  log::error("VoxelGeom shader does not support instanced rendering.");
  return *this;
}


void VoxelGeom::updateTransform()
{
  if (_pvm.dirtyPv())
  {
    _shader->setProjectionViewTransform(_pvm.pv());
  }
  if (_pvm.dirtyModel())
  {
    _shader->setModelMatrix(_pvm.model());
  }
  _pvm.clearDirty();
}


VoxelGeomProgram::VoxelGeomProgram()
{
  namespace GL = Magnum::GL;

  const GL::Version version = GL::Context::current().supportedVersion(
    { GL::Version::GL320, GL::Version::GL310, GL::Version::GL300, GL::Version::GL210 });

  GL::Shader vert{ version, GL::Shader::Type::Vertex };
  GL::Shader geom{ version, GL::Shader::Type::Geometry };
  GL::Shader frag{ version, GL::Shader::Type::Fragment };

  const std::string vert_code =
#include "3esvoxel.vert"
    ;
  const std::string geom_code =
#include "3esvoxel.geom"
    ;
  const std::string frag_code =
#include "3esvoxel.frag"
    ;
  vert.addSource(vert_code);
  geom.addSource(geom_code);
  frag.addSource(frag_code);

  CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({ vert, geom, frag }));

  attachShaders({ vert, geom, frag });

/* ES3 has this done in the shader directly and doesn't even provide
   bindFragmentDataLocation() */
#if !defined(MAGNUM_TARGET_GLES) || defined(MAGNUM_TARGET_GLES2)
#ifndef MAGNUM_TARGET_GLES
  // if (!GL::Context::current().isExtensionSupported<GL::Extensions::ARB::explicit_attrib_location>(version))
#endif
  {
    bindAttributeLocation(Position::Location, "position");
    bindAttributeLocation(Color3::Location, "colour");  // Color4 is the same
  }
#endif

  CORRADE_INTERNAL_ASSERT_OUTPUT(link());

#ifndef MAGNUM_TARGET_GLES
  // if (!GL::Context::current().isExtensionSupported<GL::Extensions::ARB::explicit_uniform_location>(version))
#endif
  {
    _model_matrix_uniform = uniformLocation("modelMatrix");
    _tint_uniform = uniformLocation("tint");
    _pv_matrix_uniform = uniformLocation("pvTransform");
    _scale_uniform = uniformLocation("scale");
  }
}


VoxelGeomProgram &VoxelGeomProgram::setProjectionViewTransform(const Magnum::Matrix4 &matrix)
{
  setUniform(_pv_matrix_uniform, matrix);
  return *this;
}


VoxelGeomProgram &VoxelGeomProgram::setModelMatrix(const Magnum::Matrix4 &matrix)
{
  setUniform(_model_matrix_uniform, matrix);
  return *this;
}


VoxelGeomProgram &VoxelGeomProgram::setTint(const Magnum::Color4 &colour)
{
  setUniform(_tint_uniform, colour);
  return *this;
}


VoxelGeomProgram &VoxelGeomProgram::setVoxelScale(const Magnum::Vector3 &scale)
{
  setUniform(_scale_uniform, scale);
  return *this;
}
}  // namespace tes::viewer::shaders
