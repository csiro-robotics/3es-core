R""(
// Version directive gets added by Magnum.
uniform mat4 viewModelMatrix;

uniform vec4 tint;

in vec4 position;
in lowp vec4 colour;

out Vertex
{
  vec4 colour;
} vert;

void main()
{
  gl_Position = viewModelMatrix * position;
  vert.colour = colour * tint;
}
)""
