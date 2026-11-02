// Probe update, step 1 (SceneEnvMapVulkan, see EnvMapVulkanProbes): the static meshes drawn into a small cube map
// around a probe, one cube face per render pass. The fragment stage (Include/ProbeCapture_Fragment.glslh) writes the
// light each surface sends to the probe and its distance; ProbeResample.glsl then reads the cube in the directions of
// the probe's rays.
// Set 0 is the per-frame set, declared as in the PBR shaders (Include/FrameCamera.glslh, Include/FrameSet.glslh); the
// view is the cube face's, from the push constants (Include/ProbeCapture.glslh), not the camera's.
#type vertex
#version 450 core

// The mesh vertex (ModelH2M's VertexH2M): only what the capture reads
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;

#include "Include/FrameCamera.glslh"
#include "Include/ProbeCapture.glslh"

layout (location = 0) out vec3 v_WorldPosition;
layout (location = 1) out vec3 v_Normal;

void main()
{
	mat4 transform = CaptureTransform();
	vec4 worldPosition = transform * vec4(a_Position, 1.0);
	v_WorldPosition = worldPosition.xyz;
	v_Normal = mat3(transform) * a_Normal;
	gl_Position = u_Capture.FaceViewProjection * worldPosition;
}

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

#include "Include/ProbeCapture_Fragment.glslh"
