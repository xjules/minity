#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"

uniform mat4 modelViewProjectionMatrix;

in vec3 position;
in vec3 normal;
in vec2 texCoord;

out vec3 vNormal;
out vec2 vTexCoord;

void main()
{
	vec4 pos = modelViewProjectionMatrix*vec4(position,1.0);

	vNormal = normal;
	vTexCoord = texCoord;	
	
	gl_Position = pos;
}