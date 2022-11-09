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
uniform vec2 screenParams;
uniform float radius;
uniform float linearScale;
uniform float exponentialScale;

out vec3 fragColour;

float linearEyeDepth(float depth)
{
  return projectionParams.z * projectionParams.w / (projectionParams.w + depth * (projectionParams.z - projectionParams.w));
}

float obscurance(float depth)
{
  // Simulated light direction. I noted that I got much better edge outlining by
  // making it (0, 0, -1). Using (0, 0, 1) tends to eat away the colour of lines and
  // points, making them black. Using (0, 0, -1) tends to outline lines and points.
  // I should really come back to investigate more and see if firstly I can understand why,
  // then see if I can remove the outline altogether where we have such edge transition.
  vec3 lightDir = vec3(0, 0, 1);
  vec4 lightPlane = vec4(lightDir, -dot(lightDir, vec3(0, 0, depth)));
  vec2 uvRadius = radius / screenParams;
  vec2 neighbourRelativeUv, neighbourUv;
  float neighbourDepth, inPlaneNeighbourDepth;
  float sum = 0.0;

  for (int y = -1; y <= 1; ++y)
  {
    neighbourRelativeUv.y = uvRadius.y * y;
    for (int x = -1; x <= 1; ++x)
    {
      if (x == 0 && y == 0)
      {
        continue;
      }

      neighbourRelativeUv.x = uvRadius.x * x;
      neighbourUv = inPs.uv0 + neighbourRelativeUv;
      float neighbourDepth = texture(depthTexture, neighbourUv).x;
      neighbourDepth = linearEyeDepth(neighbourDepth);
      // sum += (neighbourDepth > depth) ? 0.1 : 0.0;

      // Try not to darken around transitions from surface to background.
      // neighbourDepth = (neighbourDepth > 0.01) ? neighbourDepth : depth;
      inPlaneNeighbourDepth = dot(vec4(neighbourRelativeUv, neighbourDepth, 1.0), lightPlane);
      sum += max(0.0, inPlaneNeighbourDepth) / linearScale;
    }
  }

  return sum;
}

void main()
{
  vec3 colour = texture(colourTexture, inPs.uv0).xyz;
  float depth = texture(depthTexture, inPs.uv0).x;
  depth = linearEyeDepth(depth);

  float edlFactor = obscurance(depth);
  edlFactor = exp(-exponentialScale * edlFactor);

  fragColour = edlFactor * colour;
}
)""
