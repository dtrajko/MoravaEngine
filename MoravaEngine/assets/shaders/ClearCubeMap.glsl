#type compute
#version 450 core

// Clears the content of a cube map

const float PI = 3.141592;

layout(binding = 0, rgba16f) restrict writeonly uniform imageCube o_CubeMap;

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

void main()
{
	vec4 color = vec4(0.0, 1.0, 1.0, 1.0);

	if (gl_GlobalInvocationID.z == 0)
		color.rgb = vec3(1.0, 0.0, 0.0);
	else if (gl_GlobalInvocationID.z == 1)
		color.rgb = vec3(1.0, 0.0, 0.0);
	else if (gl_GlobalInvocationID.z == 2)
		color.rgb = vec3(0.0, 1.0, 0.0);
	else if (gl_GlobalInvocationID.z == 3)
		color.rgb = vec3(0.0, 1.0, 0.0);
	else if (gl_GlobalInvocationID.z == 4)
		color.rgb = vec3(0.0, 0.0, 1.0);
	else if (gl_GlobalInvocationID.z == 5)
		color.rgb = vec3(0.0, 0.0, 1.0);

	imageStore(o_CubeMap, ivec3(gl_GlobalInvocationID), color);
}
