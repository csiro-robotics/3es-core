R""(
// Version directive gets added by Magnum.

layout (points) in;
layout (triangle_strip, max_vertices=4) out;

uniform mat4 projectionMatrix;

// screenParams:
// - x => the width of the camera's target texture in pixels
// - y => the height of the camera's target texture in pixels
// - z = 1.0 + 1.0/width
// - w = 1.0 + 1.0/height
uniform vec4 screenParams;
// clipParams:
// - x => near clip plane
// - y => far clip plane
// - z = 1/near
// - w = 1/far
uniform vec4 clipParams;

uniform float pointSize;

in vec4 colour[1];
float depth[1];

out block
{
  vec4 colour;
  vec2 uv;
} geom;

// Convert from the vertex shader view depth to a linear depth value.
float viewDepthToLinearDepth(float)
{
  float far = clipParams.x;
  float nearInv = clipParams.z;
  float farInv = clipParams.w;
  return depth * ((1.0 - far * nearInv) * farInv) + far;
}

// Reciprical of viewDepthToLinearDepth)
float linearDepthToViewDepth(float depth)
{
  float far = clipParams.x;
  float nearInv = clipParams.z;
  float farInv = clipParams.w;
  return ((depth - far) * farInv) / (1.0 / far * nearInv);
}

void main()
{
  // Ensure we have a minimum point size greater than to cater for floating point error.
  const float kMinScale = 1.5;

  // Calculate shared values.
  vec4 pointPos = gl_in[0].gl_Position;
  float depth = pointPos.w;
  float size = max(0.5 * pointSize * depth, kMinScale * depth) * (screenParams.w - 1.0);

  // Calculate view plane vectors to expand into.
  vec4 right = vec4(size, 0, 0, 0);
  vec4 up = vec4(0, size, 0, 0);

  geom.colour = colour[0];

  // Build a triangle strip:
  // - bottom left
  // - bottom right
  // - top left
  // - top right

  // bottom left
  gl_Position = projectionMatrix * (pointPos + (-right - up));
  geom.uv = vec2(0, 0);
  EmitVertex();

  // bottom right
  gl_Position = projectionMatrix * (pointPos + (right - up));
  geom.uv = vec2(1, 0);
  EmitVertex();

  // top left
  gl_Position = projectionMatrix * (pointPos + (-right + up));
  geom.uv = vec2(0, 1);
  EmitVertex();

  // top right
  gl_Position = projectionMatrix * (pointPos + (right + up));
  geom.uv = vec2(1, 1);
  EmitVertex();

  EndPrimitive();
}
)""
