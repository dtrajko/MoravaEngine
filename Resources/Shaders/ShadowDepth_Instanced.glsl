// Shadow map depth pass for the foliage's copies (EnvMapVulkanFoliage): ShadowDepth.glsl with each copy's transform from the
// instance buffer (binding 1, locations 7 to 10: Include/MeshInstance.glslh). The push constants are ShadowDepth.glsl's;
// u_Model is unused (the copies' transforms hold all of it).
#type vertex
#version 450 core

// The mesh vertex (ModelH2M's Vertex): only the position is read
layout(location = 0) in vec3 a_Position;
layout(location = 7) in mat4 a_InstanceTransform;

layout (push_constant) uniform Transform
{
	mat4 u_LightViewProjection; // the cascade's world -> shadow map clip space
	mat4 u_Model;
};

void main()
{
	gl_Position = u_LightViewProjection * a_InstanceTransform * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

void main()
{
}
