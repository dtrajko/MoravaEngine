#version 330

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_normal;
layout(location = 2) in vec4 in_color;

flat out vec3 pass_color; //The "flat" qualifier stops the color from being interpolated over the triangles.
out vec4 shadowCoords;

uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec2 lightBias;
uniform mat4 transformationMatrix;
uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform vec4 clipPlane;
uniform mat4 toShadowMapSpace;
uniform float shadowDistance;
uniform float transitionDistance;

//simple diffuse lighting
vec3 calculateLighting(){
	vec3 normal = in_normal.xyz * 2.0 - 1.0;//required just because of the format the normals were stored in (0 - 1)
	float brightness = max(dot(-lightDirection, normal), 0.0);
	return (lightColor * lightBias.x) + (brightness * lightColor * lightBias.y);
}

void main(void){

	vec4 worldPosition = transformationMatrix * vec4(in_position, 1.0);
	gl_ClipDistance[0] = dot(worldPosition, clipPlane);
	vec4 positionRelativeToCamera = viewMatrix * worldPosition;
	gl_Position = projectionMatrix * positionRelativeToCamera;
	vec3 lighting = calculateLighting();
	pass_color = in_color.rgb * lighting;

	shadowCoords = toShadowMapSpace * worldPosition;
	float distance = length(positionRelativeToCamera.xyz);
	distance = distance - (shadowDistance - transitionDistance);
	distance = distance / transitionDistance;
	shadowCoords.w = clamp(1.0 - distance, 0.0, 1.0);
}
