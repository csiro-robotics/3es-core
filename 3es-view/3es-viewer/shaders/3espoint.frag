R""(
// Version directive gets added by Magnum.

in vec4 colour;
in vec2 uv;

out block
{
  colour;
} pixel;

void main()
{
  vec2 uvoffset = uv - vec2(0.5, 0.5);
  // float uvDistSqr = uvoffset.x * uvoffset.x + uvoffset.y * uvoffset.y;
  float uvDistSqr = dot(uvoffset, uvoffset);

  // Ignore pixels outside a radius of 0.5. We use 0.25 as the constant since uvDistSqr is a square value.
  if (uvDistSqr > 0.25)
  {
    discard;
  }

  pixel.colour = colour;
}
)""
