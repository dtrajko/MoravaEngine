#version 330

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;
out vec4 DirLightSpacePos;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 dirLightTransform;
uniform vec4 clipPlane;


void main()
{

	FragPos = (model * vec4(aPos, 1.0)).xyz;
	TexCoords = aTexCoords;

	vec4 WorldPosition = model * vec4(aPos, 1.0);
	gl_ClipDistance[0] = dot(WorldPosition, clipPlane);

	DirLightSpacePos = dirLightTransform * model * vec4(aPos, 1.0);
	
	mat3 modelVector = transpose(inverse(mat3(model)));

	vec3 T = normalize(modelVector * aTangent);
	vec3 B = normalize(modelVector * aBitangent);
	vec3 N = normalize(modelVector * aNormal);

	// Gram-Schmidt process
	T = normalize(T - dot(T, N) * N);
	B = cross(N, T);

	TBN = mat3(T, B, N);

	Normal = normalize(modelVector * aNormal);

	gl_Position = projection * view * vec4(FragPos, 1.0);
}
