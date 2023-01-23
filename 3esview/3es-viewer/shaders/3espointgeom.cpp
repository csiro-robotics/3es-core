//
// Author: Kazys Stepanas
//
#include "3espointgeom.h"

#include <3eslog.h>

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
PointGeom::PointGeom()
  : _shader(std::make_shared<PointGeomProgram>())
{
  _shader->setTint(Magnum::Color4(1, 1, 1, 1));
  _shader->setPointSize(1.0f);
}


PointGeom::~PointGeom() = default;


std::shared_ptr<Magnum::GL::AbstractShaderProgram> PointGeom::shader() const
{
  return _shader;
}


Shader &PointGeom::setProjectionMatrix(const Magnum::Matrix4 &matrix)
{
  _pvm.setProjection(matrix);
  return *this;
}


Shader &PointGeom::setViewMatrix(const Magnum::Matrix4 &matrix)
{
  _pvm.setView(matrix);
  return *this;
}


Shader &PointGeom::setModelMatrix(const Magnum::Matrix4 &matrix)
{
  _pvm.setModel(matrix);
  return *this;
}


Shader &PointGeom::setViewportSize(const Magnum::Vector2i &size)
{
  _shader->setViewportSize(size);
  return *this;
}


Shader &PointGeom::setColour(const Magnum::Color4 &colour)
{
  _shader->setTint(colour);
  return *this;
}

Shader &PointGeom::setDrawScale(float scale)
{
  _shader->setPointSize(scale);
  return *this;
}

Shader &PointGeom::draw(Magnum::GL::Mesh &mesh)
{
  updateTransform();
  _shader->draw(mesh);
  return *this;
}

Shader &PointGeom::draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer, size_t instance_count)
{
  (void)mesh;
  (void)buffer;
  (void)instance_count;
  log::error("PointGeom shader does not support instanced rendering.");
  return *this;
}


void PointGeom::updateTransform()
{
  if (_pvm.dirtyProjection())
  {
    _shader->setProjectionMatrix(_pvm.projection());
  }
  if (_pvm.dirtyVm())
  {
    _shader->setViewModelTransform(_pvm.vm());
  }
  _pvm.clearDirty();
}


PointGeomProgram::PointGeomProgram()
{
  namespace GL = Magnum::GL;

  const GL::Version version = GL::Context::current().supportedVersion(
    { GL::Version::GL320, GL::Version::GL310, GL::Version::GL300, GL::Version::GL210 });

  GL::Shader vert{ version, GL::Shader::Type::Vertex };
  GL::Shader geom{ version, GL::Shader::Type::Geometry };
  GL::Shader frag{ version, GL::Shader::Type::Fragment };

  const std::string vert_code =
#include "3espoint.vert"
    ;
  const std::string geom_code =
#include "3espoint.geom"
    ;
  const std::string frag_code =
#include "3espoint.frag"
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
    _view_model_matrix_uniform = uniformLocation("viewModelMatrix");
    _tint_uniform = uniformLocation("tint");
    _projection_matrix_uniform = uniformLocation("projectionMatrix");
    _screen_params_uniform = uniformLocation("screenParams");
    _point_size_uniform = uniformLocation("pointSize");
  }
}


PointGeomProgram &PointGeomProgram::setProjectionMatrix(const Magnum::Matrix4 &matrix)
{
  setUniform(_projection_matrix_uniform, matrix);
  return *this;
}


PointGeomProgram &PointGeomProgram::setViewModelTransform(const Magnum::Matrix4 &matrix)
{
  setUniform(_view_model_matrix_uniform, matrix);
  return *this;
}


PointGeomProgram &PointGeomProgram::setTint(const Magnum::Color4 &colour)
{
  setUniform(_tint_uniform, colour);
  return *this;
}


PointGeomProgram &PointGeomProgram::setPointSize(float size)
{
  setUniform(_point_size_uniform, size);
  return *this;
}


PointGeomProgram &PointGeomProgram::setViewportSize(const Magnum::Vector2i &size)
{
  const Magnum::Math::Vector4<float> screen_params = Magnum::Vector4(  //
    float(size.x()), float(size.y()),                                  //
    1 + 1 / Magnum::Float(size.x()),                                   //
    1 + 1 / Magnum::Float(size.y()));
  setUniform(_screen_params_uniform, screen_params);
  return *this;
}
}  // namespace tes::viewer::shaders
