// Shadow map depth pass (Vulkan, SceneEnvMapVulkan): a static mesh seen from the sun, into one cascade of the shadow map
// (see EnvMapVulkanShadows.h). Only depth is written: the fragment stage is empty.
#type vertex
#version 450 core

// The mesh vertex (MeshH2M's Vertex): only the position is read
layout(location = 0) in vec3 a_Position;

layout (push_constant) uniform Transform
{
	mat4 u_LightViewProjection; // the cascade's world -> shadow map clip space
	mat4 u_Model;
};

void main()
{
	gl_Position = u_LightViewProjection * u_Model * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

void main()
{
}
