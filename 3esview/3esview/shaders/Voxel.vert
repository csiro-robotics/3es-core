R""(
// Version directive gets added by Magnum.
uniform mat4 modelMatrix;

uniform vec4 tint;

in vec4 position;
in lowp vec4 colour;

out Vertex
{
  vec4 colour;
} vert;

void main()
{
  gl_Position = modelMatrix * position;
  vert.colour = colour * tint;
}
)""
