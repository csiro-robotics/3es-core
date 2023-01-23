R""(
// Version directive gets added by Magnum.

in Geom
{
  vec4 colour;
  vec2 uv;
} geom;

out vec4 fragColour;

void main()
{
  vec2 uvoffset = geom.uv - vec2(0.5, 0.5);
  // float uvDistSqr = uvoffset.x * uvoffset.x + uvoffset.y * uvoffset.y;
  float uvDistSqr = dot(uvoffset, uvoffset);

  // Ignore pixels outside a radius of 0.5. We use 0.25 as the constant since uvDistSqr is a square value.
  if (uvDistSqr > 0.25)
  {
    discard;
  }

  fragColour = geom.colour;
}
)""
