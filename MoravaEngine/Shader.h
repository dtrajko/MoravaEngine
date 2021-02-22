#pragma once

#include "CommonValues.h"

#include "Hazel/Renderer/HazelShader.h"

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>


class Shader : public Hazel::HazelShader
{
public:
	Shader();
	Shader(const char* vertexLocation, const char* fragmentLocation);
	Shader(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation);
	Shader(const char* computeLocation);

	// virtual methods
	virtual void Bind() override;
	virtual void Reload() override;
	virtual void SetIntArray(const std::string& name, int* values, uint32_t size) override;

	// HazelShader abstract methods
	virtual uint32_t GetRendererID() const override;
	virtual void SetUniformBuffer(const std::string& name, const void* data, uint32_t size) override;
	virtual void SetUniform(const std::string& fullname, float value) override;
	virtual void SetUniform(const std::string& fullname, int value) override;
	virtual void SetUniform(const std::string& fullname, const glm::vec2& value) override;
	virtual void SetUniform(const std::string& fullname, const glm::vec3& value) override;
	virtual void SetUniform(const std::string& fullname, const glm::vec4& value) override;
	virtual void SetUniform(const std::string& fullname, const glm::mat3& value) override;
	virtual void SetUniform(const std::string& fullname, const glm::mat4& value) override;
	virtual void SetFloat(const std::string& name, float value) override;
	virtual void SetInt(const std::string& name, int value) override;
	virtual void SetBool(const std::string& name, bool value) override;
	virtual void SetFloat3(const std::string& name, const glm::vec3& value) override;
	virtual void SetMat4(const std::string& name, const glm::mat4& value) override;
	virtual void SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind = true) override;
	virtual const std::string& GetName() const override;
	virtual const std::unordered_map<std::string, Hazel::ShaderBuffer>& GetShaderBuffers() const override;
	virtual const std::unordered_map<std::string, Hazel::ShaderResourceDeclaration>& GetResources() const override;
	virtual void AddShaderReloadedCallback(const ShaderReloadedCallback& callback) override;

	void UploadUniformMat4(const std::string& name, const glm::mat4& values);
	void UploadUniformMat4(uint32_t location, const glm::mat4& values);

	void CreateFromString(const char* vertexCode, const char* fragmentCode);
	void CreateFromFiles(const char* vertexLocation, const char* fragmentLocation);
	void CreateFromFiles(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation);

	void CreateFromFileVertex(const char* vertexLocation);
	void CreateFromFileFragment(const char* fragmentLocation);
	void CreateFromFileGeometry(const char* geometryLocation);
	void CreateFromFileCompute(const char* computeLocation);

	static std::string ReadFile(const char* fileLocation);
	void Validate();

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

	inline std::string GetName() { return m_Name; }

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

	void AddShaderVertex(const char* vertexCode);
	void AddShaderFragment(const char* fragmentCode);
	void AddShaderGeometry(const char* geometryCode);
	void AddShaderCompute(const char* computeCode);

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

	std::string m_Name;

	std::string m_ShaderFilepath_Vertex;
	std::string m_ShaderFilepath_Fragment;
	std::string m_ShaderFilepath_Compute;
	std::string m_ShaderFilepath_Geometry;
	std::string m_ShaderFilepath_TessControl;
	std::string m_ShaderFilepath_TessEvaluation;

};
