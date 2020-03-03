#version 330

in vec4 clipSpace;
in vec2 textureCoords;
in vec3 toCameraVector;

out vec4 out_Color;

uniform sampler2D reflectionTexture;
uniform sampler2D refractionTexture;
uniform sampler2D dudvMap;

uniform float waterMoveFactor;

const float waveStrength = 0.005;


void main(void)
{
	vec2 ndc = (clipSpace.xy / clipSpace.w) / 2.0 + 0.5;
	vec2 reflectTexCoords = vec2(ndc.x, -ndc.y);
	vec2 refractTexCoords = vec2(ndc.x, ndc.y);

	vec2 distorsion1 = (texture(dudvMap, vec2(textureCoords.x + waterMoveFactor, textureCoords.y)).rg * 2.0 - 1.0) * waveStrength;
	vec2 distorsion2 = (texture(dudvMap, vec2(-textureCoords.x + waterMoveFactor, textureCoords.y + waterMoveFactor)).rg * 2.0 - 1.0) * waveStrength;
	vec2 totalDistorsion = distorsion1 + distorsion2;

	reflectTexCoords += totalDistorsion;
	reflectTexCoords.x = clamp(reflectTexCoords.x, 0.001, 0.999);
	reflectTexCoords.y = clamp(reflectTexCoords.y, -0.999, -0.001);

	refractTexCoords += totalDistorsion;
	refractTexCoords = clamp(refractTexCoords, 0.001, 0.999);

	vec4 reflectColor = texture(reflectionTexture, reflectTexCoords);
	vec4 refractColor = texture(refractionTexture, refractTexCoords);

	vec3 viewVector = normalize(toCameraVector);
	float refractiveFactor = dot(viewVector, vec3(0.0, 1.0, 0.0));
	refractiveFactor = pow(refractiveFactor, 10.0);


	out_Color = mix(reflectColor, refractColor, refractiveFactor);
	out_Color = mix(out_Color, vec4(0.0, 0.3, 0.5, 0.5), 0.2);
}
