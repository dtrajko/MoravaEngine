#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vPosition;
out vec3 vFragPos;
out vec4 vDirLightSpacePos;
out mat3 vTBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 dirLightTransform;
uniform vec4 clipPlane;


void main()
{
    vPosition = vec3(model * vec4(aPosition, 1.0));
    vTexCoord = aTexCoord;
    vFragPos = (model * vec4(aPosition, 1.0)).xyz;

    vec4 WorldPosition = model * vec4(aPosition, 1.0);
	gl_ClipDistance[0] = dot(WorldPosition, clipPlane);

	vDirLightSpacePos = dirLightTransform * model * vec4(aPosition, 1.0);

    mat3 modelVector = transpose(inverse(mat3(model)));

    vec3 T = normalize(modelVector * aTangent);
    vec3 B = normalize(modelVector * aBitangent);
    vec3 N = normalize(modelVector * aNormal);

    // Gram-Schmidt process
    T = normalize(T - dot(T, N) * N);
    B = cross(N, T);

    vTBN = mat3(T, B, N);

    vNormal = normalize(modelVector * aNormal);
    gl_Position = projection * view * model * vec4(aPosition, 1.0);
}
