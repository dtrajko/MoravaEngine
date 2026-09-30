// Skybox shader

#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout (std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjectionMatrix;
	mat4 u_InverseViewProjection;
};

layout (location = 0) out vec3 v_Position;

void main()
{
	vec4 position = vec4(a_Position.xy, 1.0, 1.0);
	gl_Position = position;

	v_Position = (u_InverseViewProjection * position).xyz;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 finalColor;
// layout(location = 1) out vec4 o_Bloom;

layout (binding = 1) uniform samplerCube u_Texture;

layout (push_constant) uniform Uniforms
{
	float TextureLod;
	float Rotation; // degrees around the vertical (Y) axis, same as EnvMapRotation in the PBR shaders
} u_Uniforms;

layout (location = 0) in vec3 v_Position;

const float PI = 3.14159265;
const int BlurSampleCount = 16;
const float GoldenAngle = 2.39996323; // radians

// A low mip level magnified over the screen shows its texel grid: bilinear filtering is only piecewise linear,
// so the gradient changes direction at every texel center. Averaging Gaussian-weighted samples in a small cone
// around the view direction (about 1.5 texels of the sampled mip level wide) hides that grid.
vec4 SampleBlurred(vec3 direction, float lod)
{
	// Angle covered by one texel of this mip level: a cube face spans 90 degrees
	float faceTexels = max(float(textureSize(u_Texture, 0).x) / exp2(lod), 1.0);
	float coneRadius = 1.5 * (0.5 * PI) / faceTexels;

	// Tangent frame around the view direction
	vec3 up = abs(direction.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, direction));
	vec3 bitangent = cross(direction, tangent);

	// Evenly spread samples (golden angle spiral) with Gaussian weights
	vec4 sum = vec4(0.0);
	float weightSum = 0.0;
	for (int i = 0; i < BlurSampleCount; i++)
	{
		float r = sqrt((float(i) + 0.5) / float(BlurSampleCount)); // 0..1, uniform over the disc
		float angle = float(i) * GoldenAngle;
		vec2 offset = r * vec2(cos(angle), sin(angle)) * coneRadius;
		float weight = exp(-2.0 * r * r);

		vec3 sampleDirection = normalize(direction + offset.x * tangent + offset.y * bitangent);
		sum += textureLod(u_Texture, sampleDirection, lod) * weight;
		weightSum += weight;
	}
	return sum / weightSum;
}

vec3 RotateVectorAboutY(float angle, vec3 vec)
{
	angle = radians(angle);
	mat3x3 rotationMatrix = { vec3(cos(angle), 0.0, sin(angle)),
	                          vec3(0.0, 1.0, 0.0),
	                          vec3(-sin(angle), 0.0, cos(angle)) };
	return rotationMatrix * vec;
}

void main()
{
	vec3 direction = RotateVectorAboutY(u_Uniforms.Rotation, normalize(v_Position));
	float lod = u_Uniforms.TextureLod;

	// LOD 0 is the sharp environment: a single sample (the cone would only be a fraction of a pixel wide)
	finalColor = lod > 0.0 ? SampleBlurred(direction, lod) : textureLod(u_Texture, direction, 0.0);
	// o_Bloom = vec4(0.0);
}
