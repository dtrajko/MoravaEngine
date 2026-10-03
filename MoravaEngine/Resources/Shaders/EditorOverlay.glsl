// Editor overlays (Vulkan, SceneEnvMapVulkan): wireframe, bounding boxes and the selection mask, drawn in a single
// color into the overlay/mask framebuffers after tonemapping (see RecordEditorOverlayPasses in EnvMapVulkanRenderer.cpp).
// Static meshes and bounding box lines; EditorOverlay_Anim.glsl is the skinned version.
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
// Not used: declared so the inputs match the mesh vertex layout (ModelH2M's Vertex)
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;

layout (push_constant) uniform Transform
{
	mat4 u_MVP; // view projection * model
};

void main()
{
	gl_Position = u_MVP * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

layout (push_constant) uniform Settings
{
	layout (offset = 64) vec4 Color; // alpha 0 writes depth only
} u_Settings;

void main()
{
	o_Color = u_Settings.Color;
}
