#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"

uniform vec3 diffuseColor;
uniform sampler2D diffuseTexture;

in vec3 vNormal;
in vec2 vTexCoord;

out vec4 fragColor;

void main()
{
	vec4 result = vec4(abs(vTexCoord).xy,1.0,1.0);
	//result.rgb *= diffuseColor;
	//result.rgb = texture(diffuseTexture,vec2(vTexCoord)).rgb;

	fragColor = result;
}