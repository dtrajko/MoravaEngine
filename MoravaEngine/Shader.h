#pragma once

#include "CommonValues.h"

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>



class Shader
{
public:
	Shader();
	Shader(const char* vertexLocation, const char* fragmentLocation);
	Shader(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation);
	Shader(const char* computeLocation);
	void CreateFromString(const char* vertexCode, const char* fragmentCode);
	void CreateFromFiles(const char* vertexLocation, const char* fragmentLocation);
	void CreateFromFiles(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation);
	void CreateFromFileCompute(const char* computeLocation);
	static std::string ReadFile(const char* fileLocation);
	void Validate();
	void Bind();

	// generic setter methods for uniform location variables
	void setBool(const std::string& name, bool value);
	void setInt(const std::string& name, int value);
	void setFloat(const std::string& name, float value);
	void setVec2(const std::string& name, const glm::vec2& value);
	void setVec2(const std::string& name, float x, float y);
	void setVec3(const std::string& name, const glm::vec3& value);
	void setVec3(const std::string& name, float x, float y, float z);
	void setVec4(const std::string& name, const glm::vec4& value);
	void setVec4(const std::string& name, float x, float y, float z, float w);
	void setMat2(const std::string& name, const glm::mat2& mat);
	void setMat3(const std::string& name, const glm::mat3& mat);
	void setMat4(const std::string& name, const glm::mat4& mat);
	void setLightMat4(std::vector<glm::mat4> lightMatrices);
	GLint GetUniformLocation(const std::string& name);

	void Unbind();
	void ClearShader();
	~Shader();

	GLuint GetProgramID();

	// Omni shadow maps
	void SetLightMatrices(std::vector<glm::mat4> lightMatrices);

protected:
	void CompileProgram();
	virtual void GetUniformLocations();

private:
	void CompileShader(const char* vertexCode, const char* fragmentCode);
	void CompileShader(const char* vertexCode, const char* geometryCode, const char* fragmentCode);
	void CompileComputeShader(const char* computeCode);
	void AddShader(GLuint programID, const char* shaderCode, GLenum shaderType);

	const char* GetShaderTypeNameFromEnum(const GLenum shaderType);

protected:
	GLuint programID = -1;
	GLint shaderID = -1;

private:
	std::map<std::string, int> m_UniformLocations;
	bool m_Validated = false;

	// omni shadow map
	GLint uniformLightMatrices[6];

	struct
	{
		GLuint shadowMap;
		GLuint farPlane;
	} uniformOmniShadowMap[MAX_POINT_LIGHTS + MAX_SPOT_LIGHTS];
};
