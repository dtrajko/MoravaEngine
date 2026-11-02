#version 330

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec3  vReflection;
out vec3  vRefraction;
out float vFresnel;
out vec2  vTexCoord;
out vec3  vWorldPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 cameraPosition;

// Indices of refraction
const float Air = 1.0;
const float Glass = 1.51714;

// Air to glass ratio of the indices of refraction (Eta)
const float Eta = Air / Glass;
 
// see http://en.wikipedia.org/wiki/Refractive_index Reflectivity
const float R0 = ((Air - Glass) * (Air - Glass)) / ((Air + Glass) * (Air + Glass));

void main(void)
{
	// We calculate in world space.

	vec4 WorldPos = model * vec4(aPosition, 1.0);

	vec3 Incident = normalize(vec3(WorldPos.xyz - cameraPosition));

	vec3 Normal = mat3(transpose(inverse(model))) * aNormal;

	vRefraction = refract(Incident, Normal, Eta);
	vReflection = reflect(Incident, Normal);
			
	// see http://en.wikipedia.org/wiki/Schlick%27s_approximation
	vFresnel = R0 + (1.0 - R0) * pow((1.0 - dot(-Incident, aNormal)), 5.0);

	vTexCoord = aTexCoord;

	vWorldPos = WorldPos.xyz;

	gl_Position = projection * view * model * vec4(aPosition, 1.0);
}
