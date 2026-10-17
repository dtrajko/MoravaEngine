#version 330 core

out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];

uniform float screenWidth;
uniform float screenHeight;

uniform mat4 projection;

uniform int kernelSize;
uniform float radius;
uniform float bias;

// parameters (you'd probably want to use them as uniforms to more easily tweak the effects)
// int kernelSize = 64;
// float radius = 0.5;
// float bias = 0.025;

void main()
{
	// tile noise texture over screen based on screen dimensions divided by noise size
	vec2 noiseScale = vec2(screenWidth / 4.0, screenHeight / 4.0);

	// get input for SSAO algorithm (fragPos and normal are in view space!)
	vec3 fragPos = texture(gPosition, TexCoords).xyz;
	vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
	vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
	// create TBN change-of-basis matrix: from tangent-space to view-space
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal)); // find vector orthogonal to normal that
	                                                                       // is on plane of normal and randomVec
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	// iterate over the sample kernel and calculate occlusion factor
	float occlusion = 0.0;
	for (int i = 0; i < kernelSize; ++i)
	{
		// get sample position
		vec3 sample = TBN * samples[i]; // from tangent to view-space
		sample = fragPos + sample * radius;

		// get sample position in screen space
		vec4 offset = vec4(sample, 1.0);
		offset = projection * offset; // from view to clip-space
		offset.xyz /= offset.w; // perspective divide
		offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

		// get (potential) occluder corresponding to sample pos in screen space
		vec3 occluderPos = texture(gPosition, offset.xy).rgb;

		// occluding geometrz shouldn't count if it's outside the dome radius (but we want smooth transition rather than abrubt cutoff)
		float rangeCheck = smoothstep(0.0, 1.0, radius / length(fragPos - occluderPos)); // smoothstep clamps but also smooths

		// in view space, greater z values are closer to camera
		occlusion += (occluderPos.z >= sample.z + bias ? 1.0 : 0.0) * rangeCheck;
	}
	FragColor = 1.0 - (occlusion / kernelSize);
}
