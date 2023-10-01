#version 400
#extension GL_ARB_shading_language_include : require
#include "/model-globals.glsl"

uniform vec3 worldCameraPosition;
uniform vec3 worldLightPosition;
uniform vec3 diffuseColor;
uniform sampler2D diffuseTexture;
uniform bool wireframeEnabled;
uniform vec4 wireframeLineColor;

in fragmentData
{
	vec3 position;
	vec3 normal;
	vec2 texCoord;
	noperspective vec3 edgeDistance;
} fragment;

out vec4 fragColor;

void main()
{
	vec4 result = vec4(0.5,0.5,0.5,1.0);

	if (wireframeEnabled)
	{
		float smallestDistance = min(min(fragment.edgeDistance[0],fragment.edgeDistance[1]),fragment.edgeDistance[2]);
		float edgeIntensity = exp2(-1.0*smallestDistance*smallestDistance);
		result.rgb = mix(result.rgb,wireframeLineColor.rgb,edgeIntensity*wireframeLineColor.a);
	} else {
		// Normalize vectors
		vec3 N = normalize(fragment.normal);
		vec3 L = normalize(worldCameraPosition - fragment.position);

		// Compute ambient
		vec3 lightColor = vec3(1.0,1.0,1.0);
		// vec3 ambient = materialAmbient * lightColor;
		vec3 ambient = vec3(0.0,0.0,0.0);

		// Compute diffuse
		float diff = max(dot(N, L), 0.0);
		vec3 diffuse = diffuseColor * (diff * lightColor);

		// Compute specular
		float materialShininess = 128.0;
		float materialSpecular = 1.0f;
		vec3 V = normalize(-fragment.position);
		vec3 R = reflect(-L, N);  
		float spec = pow(max(dot(V, R), 0.0), materialShininess);
		vec3 specular = materialSpecular * (spec * lightColor); 

		// Combine all components
		result.rgb = ambient + diffuse + specular;
	}

	fragColor = result;
}