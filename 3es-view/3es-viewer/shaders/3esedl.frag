R""(
// Verion directive added by Magnum.
uniform sampler2D colourTexture;
uniform sampler2D depthTexture;

in block
{
  vec2 uv0;
} inPs;

// Near/far clip in z/w
uniform vec4 projectionParams;
// Screen width and height.
uniform vec2 screenParams;
// Simulated light direction in camera space.
uniform vec3 lightDir;
// Distance at which to measure neighbours. Normally 1.0
uniform float radius;
// Linear scaling of the EDL darkening.
uniform float linearScale;
// Exponential scaling of the EDL darkening.
uniform float exponentialScale;

out vec3 fragColour;

float linearEyeDepth(float depth, float near, float far)
{
  return near * far / (far - depth * (far - near));
}

float obscurance(float depth)
{
  vec4 lightPlane = vec4(lightDir, -dot(lightDir, vec3(0, 0, depth)));
  vec2 uvRadius = radius / screenParams;
  float sum = 0.0;

  for (int y = -1; y <= 1; ++y)
  {
    vec2 neighbourRelativeUv = vec2(0, uvRadius.y * y);
    for (int x = -1; x <= 1; ++x)
    {
      if (x == 0 && y == 0)
      {
        continue;
      }

      neighbourRelativeUv.x = uvRadius.x * x;
      vec2 neighbourUv = inPs.uv0 + neighbourRelativeUv;
      float neighbourDepth = texture(depthTexture, neighbourUv).x;
      neighbourDepth = linearEyeDepth(neighbourDepth, projectionParams.z, projectionParams.w);
      float inPlaneNeighbourDepth = dot(vec4(neighbourRelativeUv, neighbourDepth, 1.0), lightPlane);
      sum += max(0.0, inPlaneNeighbourDepth) / linearScale;
    } 
  }

  return sum;
}

void main()
{
  vec3 colour = texture(colourTexture, inPs.uv0).xyz;
  float depth = texture(depthTexture, inPs.uv0).x;
  depth = linearEyeDepth(depth, projectionParams.z, projectionParams.w);

  float edlFactor = obscurance(depth);
  edlFactor = exp(-exponentialScale * edlFactor);

  fragColour = edlFactor * colour;
}
)""
