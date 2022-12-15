R""(
// Version directive gets added by Magnum.

layout(points) in;
layout(line_strip, max_vertices = 16) out;

uniform mat4 pvTransform;

// screenParams:
// - x => the width of the camera's target texture in pixels
// - y => the height of the camera's target texture in pixels
// - z = 1.0 + 1.0/width
// - w = 1.0 + 1.0/height
uniform vec4 screenParams;

uniform vec3 scale;

in Vertex
{
  vec4 colour;
}
vert[1];

out Geom
{
  vec4 colour;
}
geom;

void main()
{
  // Incoming points are in the *world frame*.
  vec4 centre = gl_in[0].gl_Position;

  // Calculate view plane vectors to expand into.
  vec4 right = vec4(scale.x, 0, 0, 0);
  vec4 fwd = vec4(0, scale.y, 0, 0);
  vec4 up = vec4(0, 0, scale.z, 0);

  geom.colour = vert[0].colour;

  // Inefficient line strip render of the voxel box. I trace 3 of the vertical lines twice.
  // Initial vertical
  // left, back, bottom
  gl_Position = pvTransform * (centre + (-right - fwd - up));
  EmitVertex();
  // right, back, bottom
  gl_Position = pvTransform * (centre + (right - fwd - up));
  EmitVertex();

  // right, back, top
  gl_Position = pvTransform * (centre + (right - fwd + up));
  EmitVertex();
  gl_Position = pvTransform * (centre + (right - fwd - up));
  EmitVertex();

  // right, forward, bottom
  gl_Position = pvTransform * (centre + (right + fwd - up));
  EmitVertex();

  // right, forward, top
  gl_Position = pvTransform * (centre + (right + fwd + up));
  EmitVertex();
  gl_Position = pvTransform * (centre + (right + fwd - up));
  EmitVertex();

  // left, forward, bottom
  gl_Position = pvTransform * (centre + (-right + fwd - up));
  EmitVertex();

  // left, forward, top
  gl_Position = pvTransform * (centre + (-right + fwd + up));
  EmitVertex();
  gl_Position = pvTransform * (centre + (-right + fwd - up));
  EmitVertex();

  // Close the bottom loop.
  // left, back, bottom
  gl_Position = pvTransform * (centre + (-right - fwd - up));
  EmitVertex();

  // Top loop
  // left, back, top
  gl_Position = pvTransform * (centre + (-right - fwd + up));
  EmitVertex();
  // right, back, top
  gl_Position = pvTransform * (centre + (right - fwd + up));
  EmitVertex();
  // right, forward, top
  gl_Position = pvTransform * (centre + (right + fwd + up));
  EmitVertex();
  // left, forward, top
  gl_Position = pvTransform * (centre + (-right + fwd + up));
  EmitVertex();
  // left, back, top
  gl_Position = pvTransform * (centre + (-right - fwd + up));
  EmitVertex();

  EndPrimitive();
}
)""
