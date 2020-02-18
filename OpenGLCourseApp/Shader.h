#pragma once

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

#include <GL/glew.h>

#include "DirectionalLight.h"
#include "PointLight.h"


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

	GLuint programID;
	GLint shaderID;

	// Locations of uniform variables
	GLint uniformModel;
	GLint uniformView;
	GLint uniformProjection;
	GLint uniformEyePosition;

	struct
	{
		GLint uniformColor;
		GLint uniformAmbientIntensity;
		GLint uniformDiffuseIntensity;
		GLint uniformDirection;
	} uniformDirectionalLight;

	GLint uniformSpecularIntensity;
	GLint uniformShininess;

};
