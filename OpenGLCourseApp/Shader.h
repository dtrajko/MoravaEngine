#pragma once

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "CommonValues.h"

#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"



class Shader
{
public:
	Shader();
	void CreateFromString(const char* vertexCode, const char* fragmentCode);
	void CreateFromFiles(const char* vertexLocation, const char* fragmentLocation);
	std::string ReadFile(const char* fileLocation);
	GLuint GetProgramID();
	GLint GetModelLocation();
	GLint GetViewLocation();
	GLint GetProjectionLocation();
	GLint GetUniformLocationEyePosition();
	// Directional Light
	GLint GetUniformLocationAmbientColor();
	GLint GetUniformLocationAmbientIntensity();
	GLint GetUniformLocationDiffuseIntensity();
	GLint GetUniformLocationLightDirection();

	GLint GetUniformLocationSpecularIntensity();
	GLint GetUniformLocationShininess();

	void SetDirectionalLight(DirectionalLight* directionalLight);
	void SetPointLights(PointLight* pointLights, unsigned int lightCount);
	void SetSpotLights(SpotLight* spotLights, unsigned int lightCount);

	// Directional light shadow map
	void SetTexture(GLuint textureUnit);
	void SetDirectionalShadowMap(GLuint textureUnit);
	void SetDirectionalLightTransform(glm::mat4* transform);

	void Bind();
	void Unbind();
	void ClearShader();
	~Shader();

private:
	void CompileShader(const char* vertexCode, const char* fragmentCode);
	void AddShader(GLuint programID, const char* shaderCode, GLenum shaderType);
	const char* GetShaderTypeNameFromEnum(const GLenum shaderType);

private:
	int pointLightCount;
	int spotLightCount;

	GLuint programID;
	GLint shaderID;

	// Locations of uniform variables
	GLint uniformModel;
	GLint uniformView;
	GLint uniformProjection;
	GLint uniformEyePosition;

	GLint uniformDirectionalLightTransform;
	GLint uniformDirectionalShadowMap;

	struct
	{
		GLint uniformColor;
		GLint uniformAmbientIntensity;
		GLint uniformDiffuseIntensity;
		GLint uniformDirection;
	} uniformDirectionalLight;

	GLint uniformPointLightCount;

	struct
	{
		GLint uniformColor;
		GLint uniformAmbientIntensity;
		GLint uniformDiffuseIntensity;
		GLint uniformPosition;
		GLint uniformConstant;
		GLint uniformLinear;
		GLint uniformExponent;
	} uniformPointLight[MAX_POINT_LIGHTS];

	GLint uniformSpotLightCount;

	struct
	{
		GLint uniformColor;
		GLint uniformAmbientIntensity;
		GLint uniformDiffuseIntensity;
		GLint uniformPosition;
		GLint uniformDirection;
		GLint uniformConstant;
		GLint uniformLinear;
		GLint uniformExponent;
		GLint uniformEdge;
	} uniformSpotLight[MAX_SPOT_LIGHTS];

	GLint uniformSpecularIntensity;
	GLint uniformShininess;

	GLint uniformTexture;

};
