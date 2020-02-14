#pragma once

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

#include <GL/glew.h>


class Shader
{
public:
	Shader();
	void CreateFromString(const char* vertexCode, const char* fragmentCode);
	void CreateFromFiles(const char* vertexLocation, const char* fragmentLocation);
	std::string ReadFile(const char* fileLocation);
	GLuint GetProgramID();
	GLuint GetProjectionLocation();
	GLuint GetModelLocation();
	void Bind();
	void Unbind();
	void ClearShader();
	~Shader();

private:
	void CompileShader(const char* vertexCode, const char* fragmentCode);
	void AddShader(GLuint programID, const char* shaderCode, GLenum shaderType);
	const char* GetShaderTypeNameFromEnum(const GLenum shaderType);

private:
	GLuint programID;
	GLuint shaderID;
	GLuint uniformProjection;
	GLuint uniformModel;

};
