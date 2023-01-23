R""(
// Version directive gets added by Magnum.

in Geom
{
  vec4 colour;
} geom;

out vec4 fragColour;

void main()
{
  fragColour = geom.colour;
}
)""
