#version 330

in vec4 clipSpace;
in vec2 textureCoords;
in vec3 toCameraVector;
in vec3 fromLightVector;

layout (location = 0) out vec4 out_Color;
layout (location = 1) out vec4 out_BrightColor;

uniform sampler2D reflectionTexture;
uniform sampler2D refractionTexture;
uniform sampler2D normalMap;
uniform sampler2D dudvMap;
uniform sampler2D depthMap;
uniform vec3 lightColor;
uniform float moveFactor;
uniform float nearPlane;
uniform float farPlane;

uniform vec3 eyePosition; // same as cameraPosition

uniform float waterLevel;
uniform vec4 waterColor;

const float waveStrength = 0.01;
const float shineDamper = 20.0;
const float reflectivity = 0.5;
const vec3 waterColorTM = vec3(0.0, 0.3, 0.5);

void main(void)
{
	vec2 ndc = (clipSpace.xy / clipSpace.w) / 2.0 + 0.5;
	vec2 refractTexCoords = vec2(ndc.x, ndc.y);
	vec2 reflectTexCoords = vec2(ndc.x, -ndc.y);

	float floorDepth = texture(depthMap, refractTexCoords).r;
	float floorDistance = 2.0 * nearPlane * farPlane / (farPlane + nearPlane - (2.0 * floorDepth - 1.0) * (farPlane - nearPlane));

	float waterDepthAbs = gl_FragCoord.z;
	float waterDistance = 2.0 * nearPlane * farPlane / (farPlane + nearPlane - (2.0 * waterDepthAbs - 1.0) * (farPlane - nearPlane));
	float waterDepth = floorDistance - waterDistance;
	waterDepth = clamp(waterDepth / 2.0, 0.1, 1.0);
	vec2 distortedTexCoords = texture(dudvMap, vec2(textureCoords.x + moveFactor, textureCoords.y)).rg * 0.1;
	distortedTexCoords = textureCoords + vec2(distortedTexCoords.x, distortedTexCoords.y + moveFactor);
	vec2 totalDistortion = (texture(dudvMap, distortedTexCoords).rg * 2.0 - 1.0) * waveStrength * waterDepth;

	refractTexCoords += totalDistortion;
	refractTexCoords.x = clamp(refractTexCoords.x, 0.001, 0.999);
	refractTexCoords.y = clamp(refractTexCoords.y, 0.001, 0.999);

	reflectTexCoords += totalDistortion;
	reflectTexCoords.x = clamp(reflectTexCoords.x, 0.001, 0.999);
	reflectTexCoords.y = clamp(reflectTexCoords.y, -0.999, -0.001);

	vec4 reflectColor = texture(reflectionTexture, reflectTexCoords);
	vec4 refractColor = texture(refractionTexture, refractTexCoords);

	vec4 normalMapColor = texture(normalMap, distortedTexCoords);
	vec3 normal = vec3(normalMapColor.r * 2.0 - 1.0, normalMapColor.b * 3.0, normalMapColor.g * 2.0 - 1.0);
	normal = normalize(normal);

	vec3 viewVector = normalize(toCameraVector);
	float refractiveFactor = dot(viewVector, normal);
	refractiveFactor = pow(refractiveFactor, 0.5);
	refractiveFactor = clamp(refractiveFactor, 0.0, 0.8);

	vec3 reflectedLight = reflect(normalize(fromLightVector), normal);
	float specular = max(dot(reflectedLight, viewVector), 0.0);
	specular = pow(specular, shineDamper);
	vec3 specularHighlights = lightColor * specular * reflectivity * waterDepth;

	out_Color = mix(reflectColor, refractColor, refractiveFactor);
	out_Color = mix(out_Color, vec4(waterColorTM, 1.0), 0.4);
	out_Color = mix(out_Color, vec4(waterColorTM, 1.0), 0.4) + vec4(specularHighlights, 0.0);

	// Add a blue tint under the water level
    if (eyePosition.y < waterLevel)
	{
		out_Color = mix(out_Color, waterColor, 0.5);
	}

	out_Color.a = max(waterDepth, 0.5);

	out_BrightColor = vec4(0.0, 0.0, 1.0, 1.0);
}
