// -----------------------------
// -- Hazel Engine PBR shader --
// -----------------------------
// Note: this shader is still very much in progress. There are likely many bugs and future additions that will go in.
//       Currently heavily updated. 
//
// References upon which this is based:
// - Unreal Engine 4 PBR notes (https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)
// - Frostbite's SIGGRAPH 2014 paper (https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/)
// - Michał Siejak's PBR project (https://github.com/Nadrin)
// - My implementation from years ago in the Sparky engine (https://github.com/TheCherno/Sparky)
// #type vertex
#version 430 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;

uniform mat4 u_ViewProjectionMatrix;
uniform mat4 u_ViewMatrix;
uniform mat4 u_Transform;
uniform mat4 u_DirLightTransform;
uniform mat4 u_LightMatrixCascade0;
uniform mat4 u_LightMatrixCascade1;
uniform mat4 u_LightMatrixCascade2;
uniform mat4 u_LightMatrixCascade3;

out VertexOutput
{
	vec3 WorldPosition;
    vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;
	vec4 DirLightSpacePos;
	vec3 FragPos;
	mat3 TBN;
	vec4 ShadowMapCoords[4];
	vec3 ViewPosition;
} vs_Output;

void main()
{
	vs_Output.WorldPosition = vec3(u_Transform * vec4(a_Position, 1.0));
    vs_Output.Normal = mat3(u_Transform) * a_Normal;
	vs_Output.TexCoord = vec2(a_TexCoord.x, 1.0 - a_TexCoord.y);
	vs_Output.WorldNormals = mat3(u_Transform) * mat3(a_Tangent, a_Binormal, a_Normal);
	vs_Output.WorldTransform = mat3(u_Transform);
	vs_Output.Binormal = a_Binormal;
	vs_Output.DirLightSpacePos = u_DirLightTransform * u_Transform * vec4(a_Position, 1.0);
	vs_Output.FragPos = (u_Transform * vec4(a_Position, 1.0)).xyz;

	// BEGIN TBN
    mat3 modelVector = transpose(inverse(mat3(u_Transform)));
    vec3 T = normalize(modelVector * a_Tangent);
    vec3 B = normalize(modelVector * a_Binormal);
    vec3 N = normalize(modelVector * a_Normal);
    // Gram-Schmidt process
    T = normalize(T - dot(T, N) * N);
    B = cross(N, T);
    vs_Output.TBN = mat3(T, B, N);
	// END TBN

	vs_Output.ShadowMapCoords[0] = u_LightMatrixCascade0 * vec4(vs_Output.WorldPosition, 1.0);
	vs_Output.ShadowMapCoords[1] = u_LightMatrixCascade1 * vec4(vs_Output.WorldPosition, 1.0);
	vs_Output.ShadowMapCoords[2] = u_LightMatrixCascade2 * vec4(vs_Output.WorldPosition, 1.0);
	vs_Output.ShadowMapCoords[3] = u_LightMatrixCascade3 * vec4(vs_Output.WorldPosition, 1.0);
	vs_Output.ViewPosition = vec3(u_ViewMatrix * vec4(vs_Output.WorldPosition, 1.0));

	gl_Position = u_ViewProjectionMatrix * u_Transform * vec4(a_Position, 1.0);
}
