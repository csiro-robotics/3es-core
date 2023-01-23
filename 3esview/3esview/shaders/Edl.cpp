#include "Edl.h"

#include <Magnum/GL/Context.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Matrix4.h>

#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/Assert.h>

namespace tes::shaders
{
Edl::Edl()
{
  namespace GL = Magnum::GL;
  const GL::Version version = GL::Context::current().supportedVersion(
    { GL::Version::GL320, GL::Version::GL310, GL::Version::GL300, GL::Version::GL210 });

  GL::Shader vert{ version, GL::Shader::Type::Vertex };
  GL::Shader frag{ version, GL::Shader::Type::Fragment };

  const std::string vert_code =
#include "3esedl.vert"
    ;
  const std::string frag_code =
#include "3esedl.frag"
    ;
  vert.addSource(vert_code);
  frag.addSource(frag_code);

  CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({ vert, frag }));

  attachShaders({ vert, frag });

#if !defined(MAGNUM_TARGET_GLES) || defined(MAGNUM_TARGET_GLES2)
#ifndef MAGNUM_TARGET_GLES
  if (!GL::Context::current().isExtensionSupported<GL::Extensions::ARB::explicit_attrib_location>(version))
#endif  // MAGNUM_TARGET_GLES
  {
    bindAttributeLocation(Position::Location, "vertex");
    bindAttributeLocation(TextureCoordinates::Location, "uv0");
  }
#endif  // !defined(MAGNUM_TARGET_GLES) || defined(MAGNUM_TARGET_GLES2)

  CORRADE_INTERNAL_ASSERT_OUTPUT(link());

#ifndef MAGNUM_TARGET_GLES
  // if (!GL::Context::current().isExtensionSupported<GL::Extensions::ARB::explicit_uniform_location>(version))
#endif  // MAGNUM_TARGET_GLES
  {
    _projectionMatrixUniform = uniformLocation("projectionMatrix");
    _projectionParamsUniform = uniformLocation("projectionParams");
    _screenParamsUniform = uniformLocation("screenParams");
    _radiusUniform = uniformLocation("radius");
    _linearScaleUniform = uniformLocation("linearScale");
    _exponentialScaleUniform = uniformLocation("exponentialScale");
    _lightDirUniform = uniformLocation("lightDir");
  }

#ifndef MAGNUM_TARGET_GLES
  // if (!GL::Context::current().isExtensionSupported<GL::Extensions::ARB::shading_language_420pack>(version))
#endif  // MAGNUM_TARGET_GLES
  {
    setUniform(uniformLocation("colourTexture"), ColourUnit);
    setUniform(uniformLocation("depthTexture"), DepthUnit);
  }

  // Set default shader values.
  setRadius(1.0f);
  setLinearScale(1.0f);
  setExponentialScale(1.0f);
  setLightDirection(Magnum::Vector3(0.0f, 0.0f, 1.0f));
}


Edl::~Edl() = default;

Edl &Edl::setProjectionMatrix(const Magnum::Matrix4 &matrix)
{
  setUniform(_projectionMatrixUniform, matrix);
  return *this;
}

Edl &Edl::bindColourTexture(Magnum::GL::Texture2D &texture)
{
  texture.bind(ColourUnit);
  return *this;
}

Edl &Edl::bindDepthBuffer(Magnum::GL::Texture2D &texture)
{
  texture.bind(DepthUnit);
  return *this;
}

Edl &Edl::setClipParams(Magnum::Float near, Magnum::Float far, bool perspective, bool reverse_depth)
{
  Magnum::Vector4 params(0, 0, near, far);

  if (perspective)
  {
    if (!reverse_depth)
    {
      params.x() = far / (far - near);
      params.y() = (-far * near) / (far - near);
    }
    else
    {
      params.x() = -near / (far - near);
      params.y() = (far * near) / (far - near);
    }
  }
  else
  {
    if (!reverse_depth)
    {
      params.x() = -near / (far / near);
      params.y() = Magnum::Float{ -1.0 } / (far - near);
    }
    else
    {
      params.x() = far / (far - near);
      params.y() = Magnum::Float{ -1.0 } / (far - near);
    }
  }

  float z = params.z();
  float w = params.w();
  setUniform(_projectionParamsUniform, params);
  return *this;
}

Edl &Edl::setScreenParams(const Magnum::Vector2i &view_size)
{
  setUniform(_screenParamsUniform, Magnum::Vector2(view_size));
  return *this;
}

Edl &Edl::setLightDirection(const Magnum::Vector3 &direction)
{
  setUniform(_lightDirUniform, direction);
  return *this;
}

}  // namespace tes::shaders
