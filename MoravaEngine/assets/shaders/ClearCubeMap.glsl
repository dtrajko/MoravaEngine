#type compute
#version 450 core

// Clears the content of a cube map

const float PI = 3.141592;

layout(binding = 0, rgba16f) restrict writeonly uniform imageCube o_CubeMap;
layout(binding = 1) uniform sampler2D u_EquirectangularTex;

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

void main()
{
	vec2 uv = vec2(0.0, 0.0);
	vec4 texColor = texture(u_EquirectangularTex, uv);
	vec4 color = vec4(1.0, 0.0, 1.0, 1.0);
	color *= texColor;
	imageStore(o_CubeMap, ivec3(gl_GlobalInvocationID), color);
}
