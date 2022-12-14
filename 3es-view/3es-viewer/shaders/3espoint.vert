R""(
// Version directive gets added by Magnum.
in vec4 vertex;
in vec4 colour;

uniform mat4 viewModelMatrix;

uniform vec4 tint;

out gl_PerVertex
{
  vec4 gl_Position;
};

out block
{
  vec4 colour;
} vert;

void main()
{
  gl_Position = viewModelMatrix * vertex;
  vert.colour = colour * tint;
}
)""
