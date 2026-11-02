#version 330

const float PI = 3.1415926535897932384626433832795;

const float specularReflectivity = 0.2;
const float shineDamper = 4.0;

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec4 in_indicators;

out vec4 pass_clipSpaceGrid;
out vec4 pass_clipSpaceReal;
out vec3 pass_normal;
out vec3 pass_toCameraVector;
out vec3 pass_specular;
out vec3 pass_diffuse;

uniform float height;
uniform vec3 cameraPos;
uniform float waveTime;
uniform float waveAmplitude;
uniform float waveLength;

uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec2 lightBias;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 transformationMatrix;

vec3 calcSpecularLighting(vec3 toCamVector, vec3 toLightVector, vec3 normal){
	vec3 reflectedLightDirection = reflect(-toLightVector, normal);
	float specularFactor = dot(reflectedLightDirection , toCamVector);
	specularFactor = max(specularFactor,0.0);
	specularFactor = pow(specularFactor, shineDamper);
	return specularFactor * specularReflectivity * lightColor;
}

vec3 calculateDiffuseLighting(vec3 toLightVector, vec3 normal) {
	float brightness = max(dot(toLightVector, normal), 0.0);
	return (lightColor * lightBias.x) + (brightness * lightColor * lightBias.y);
}

vec3 calcNormal(vec3 vertex0, vec3 vertex1, vec3 vertex2) {
	vec3 tangent = vertex1 - vertex0;
	vec3 bitangent = vertex2 - vertex0;
	return normalize(cross(tangent, bitangent));
}

float generateOffset(float x, float z, float val1, float val2) {
	float radiansX = ((mod(x+z*x*val1, waveLength)/waveLength) + waveTime * mod(x * 0.8 + z, 1.5)) * 2.0 * PI;
	float radiansZ = ((mod(val2 * (z*x +x*z), waveLength)/waveLength) + waveTime * 2.0 * mod(x , 2.0) ) * 2.0 * PI;
	return waveAmplitude * 0.5 * (sin(radiansZ) + cos(radiansX));
}

vec3 applyDistortion(vec3 vertex) {
	float xDistortion = generateOffset(vertex.x, vertex.z, 0.2, 0.1);
	float yDistortion = generateOffset(vertex.x, vertex.z, 0.1, 0.3);
	float zDistortion = generateOffset(vertex.x, vertex.z, 0.15, 0.2);
	return vertex + vec3(xDistortion, yDistortion, zDistortion);
}

void main(void){
	
	//get the grid position of all 3 vertices in the triangle
	vec3 currentVertex = vec3(in_position.x, height, in_position.y);
	vec3 vertex1 = currentVertex + vec3(in_indicators.x, 0.0, in_indicators.y);
	vec3 vertex2 = currentVertex + vec3(in_indicators.z, 0.0, in_indicators.w);
	
	pass_clipSpaceGrid = projectionMatrix * viewMatrix * transformationMatrix * vec4(currentVertex, 1.0);
	
	//apply distortion to all 3 vertices
	currentVertex = applyDistortion(currentVertex);
	vertex1 = applyDistortion(vertex1);
	vertex2 = applyDistortion(vertex2);
	
	pass_normal = calcNormal(currentVertex, vertex1, vertex2);
	
	pass_clipSpaceReal = projectionMatrix * viewMatrix * transformationMatrix * vec4(currentVertex, 1.0);
	gl_Position = pass_clipSpaceReal;
	
	pass_toCameraVector = normalize(cameraPos - currentVertex);
	
	//calculate lighting
	vec3 toLightVector = -normalize(lightDirection);
	pass_specular = calcSpecularLighting(pass_toCameraVector, toLightVector, pass_normal);
	pass_diffuse = calculateDiffuseLighting(toLightVector, pass_normal);
	
}
