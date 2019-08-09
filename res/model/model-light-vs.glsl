#version 400
#extension GL_ARB_shading_language_include : require
#include "/model-globals.glsl"

uniform mat4 modelViewProjectionMatrix;

in vec3 position;

void main()
{
	vec4 pos = modelViewProjectionMatrix*vec4(position,1.0);
	gl_Position = pos;
	gl_PointSize = 27.0;

}