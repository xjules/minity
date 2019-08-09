#version 400
#extension GL_ARB_shading_language_include : require
#include "/model-globals.glsl"

out vec4 fragColor;

void main()
{
	float intensity = 1.0-length(2.0*gl_PointCoord-vec2(1.0));
	fragColor = vec4(1.0,1.0,1.0,intensity);
}