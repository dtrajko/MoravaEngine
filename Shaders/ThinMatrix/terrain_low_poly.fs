#version 330

flat in vec3 pass_color;
in vec4 shadowCoords;

out vec4 out_color;

uniform sampler2D shadowMap;
uniform float shadowMapSize;

const int pcfCount = 2;
const float totalTexels = pow(pcfCount * 2.0 + 1.0, 2);

void main(void) {

	float texelSize = 1.0 / shadowMapSize;
	float total = 0.0;

	for (int x = -pcfCount; x <= pcfCount; x++) {
		for (int y = -pcfCount; y <= pcfCount; y++) {
			float objectNearestLight = texture(shadowMap, shadowCoords.xy + vec2(x, y) * texelSize).r;
			if (shadowCoords.z > objectNearestLight) {
				total += 0.3;
			}
		}
	}

	total /= totalTexels;
	float lightFactor = 1.0 - (total * shadowCoords.w);

	out_color = vec4(pass_color, 1.0) * lightFactor;

}
