#version 330

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

out vec4 vCol;
out vec2 TexCoord;
out vec3 vNormal;
out vec3 FragPos;
out vec4 DirectionalLightSpacePos;
out mat3 tbnMatrix;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 directionalLightTransform;

void main()
{
	gl_Position = projection * view * model * vec4(pos, 1.0);
	
	DirectionalLightSpacePos = directionalLightTransform * model * vec4(pos, 1.0);
	
	vCol = vec4(clamp(pos, 0.0f, 1.0f), 1.0f);
	TexCoord = tex;
	vNormal = mat3(transpose(inverse(model))) * normal; // mat3 - normal depends on model's rotation and scale
	FragPos = (model * vec4(pos, 1.0)).xyz;

	mat3 modelView3x3 = mat3(transpose(view * model));
	vec3 tbnNormal    = modelView3x3 * normalize(normal);
    vec3 tbnTangent   = modelView3x3 * normalize(tangent);
    vec3 tbnBitangent = modelView3x3 * normalize(bitangent);

	tbnMatrix = transpose(mat3(tbnTangent, tbnBitangent, tbnNormal));
}
