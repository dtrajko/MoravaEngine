// Editor overlay (Vulkan, SceneEnvMapVulkan): normal, tangent or bitangent lines of a mesh, one line per vertex.
// Drawn instanced, with the mesh's own vertex buffer as per-instance data: each instance is one mesh vertex, and the two
// vertices of the line are its position (gl_VertexIndex 0) and the position moved along the chosen vector (gl_VertexIndex 1).
#type vertex
#version 450 core

// The mesh vertex (ModelH2M's Vertex), per instance
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;

layout (push_constant) uniform Settings
{
	mat4 u_ViewProjection;
	vec4 u_Model0; // xyz: model matrix column 0; w: line length (world units)
	vec4 u_Model1; // xyz: column 1;               w: vector (0 normal, 1 tangent, 2 bitangent)
	vec4 u_Model2; // xyz: column 2;               w: color mode (0 by vector: tangent red, bitangent green, normal blue; 1 by direction)
	vec4 u_Model3; // xyz: column 3 (translation)
};

layout(location = 0) out vec3 v_Color;

void main()
{
	mat4 model = mat4(vec4(u_Model0.xyz, 0.0), vec4(u_Model1.xyz, 0.0), vec4(u_Model2.xyz, 0.0), vec4(u_Model3.xyz, 1.0));
	int vectorIndex = int(u_Model1.w + 0.5);

	// Normals transform with the inverse transpose (correct under non-uniform scale); tangents and bitangents lie in the
	// surface and transform like positions
	mat3 model3 = mat3(model);
	vec3 direction = vectorIndex == 0 ? transpose(inverse(model3)) * a_Normal : model3 * (vectorIndex == 1 ? a_Tangent : a_Binormal);
	direction = dot(direction, direction) > 1e-12 ? normalize(direction) : vec3(0.0);

	vec3 worldPosition = (model * vec4(a_Position, 1.0)).xyz;
	if (gl_VertexIndex == 1)
	{
		worldPosition += direction * u_Model0.w;
	}
	gl_Position = u_ViewProjection * vec4(worldPosition, 1.0);

	const vec3 vectorColors[3] = vec3[](vec3(0.25, 0.45, 1.0), vec3(1.0, 0.3, 0.3), vec3(0.35, 1.0, 0.35));
	v_Color = u_Model2.w < 0.5 ? vectorColors[vectorIndex] : direction * 0.5 + 0.5;
	v_Color *= gl_VertexIndex == 0 ? 0.55 : 1.0; // darker at the base: shows which way the line points
}

#type fragment
#version 450 core

layout(location = 0) in vec3 v_Color;
layout(location = 0) out vec4 o_Color;

void main()
{
	o_Color = vec4(v_Color, 1.0);
}
