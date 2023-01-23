R""(
// Version directive gets added by Magnum.
in vec4 vertex;
in vec2 uv0;

uniform mat4 projectionMatrix;

out gl_PerVertex
{
  vec4 gl_Position;
};

out block
{
  vec2 uv0;
} outVs;

void main()
{
  gl_Position = (projectionMatrix * vec4(vertex.xyz, 1.0f)).xyzw;
  outVs.uv0.xy = uv0.xy;
}
)""
