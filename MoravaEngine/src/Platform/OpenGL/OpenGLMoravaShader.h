#pragma once

#include "Hazel/Renderer/HazelShader.h"
#include "Hazel/Renderer/ShaderUniform.h"

#include "Core/CommonValues.h"
#include "Shader/MoravaShader.h"

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>


class OpenGLMoravaShader : public MoravaShader
{
public:
	OpenGLMoravaShader();
	OpenGLMoravaShader(const char* vertexLocation, const char* fragmentLocation, bool forceCompile = false);
	OpenGLMoravaShader(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation, bool forceCompile = false);
	OpenGLMoravaShader(const char* computeLocation, bool forceCompile = false);

	static Hazel::Ref<OpenGLMoravaShader> Create(const char* vertexLocation, const char* fragmentLocation, bool forceCompile = false);
	static Hazel::Ref<OpenGLMoravaShader> Create(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation, bool forceCompile = false);
	static Hazel::Ref<OpenGLMoravaShader> Create(const char* computeLocation, bool forceCompile = false);

	// virtual methods
	virtual void Bind() override;
	virtual void Reload(bool forceCompile = false) override;
	virtual void SetIntArray(const std::string& name, int* values, uint32_t size) override;

	// HazelShader abstract methods
	virtual Hazel::RendererID GetRendererID() const override;
	virtual size_t GetHash() const override;
	virtual void SetUniformBuffer(const std::string& name, const void* data, uint32_t size) override;

	virtual void SetUniform(const std::string& fullname, float value) override;
	virtual void SetUniform(const std::string& fullname, uint32_t value) override;
	virtual void SetUniform(const std::string& fullname, int value) override;
	virtual void SetUniform(const std::string& fullname, const glm::vec2& value) override;
	virtual void SetUniform(const std::string& fullname, const glm::vec3& value) override;
	virtual void SetUniform(const std::string& fullname, const glm::vec4& value) override;
	virtual void SetUniform(const std::string& fullname, const glm::mat3& value) override;
	virtual void SetUniform(const std::string& fullname, const glm::mat4& value) override;

	virtual void SetFloat(const std::string& name, float value) override;
	virtual void SetUInt(const std::string& name, uint32_t value) override;
	virtual void SetInt(const std::string& name, int value) override;
	void SetBool(const std::string& name, bool value);
	virtual void SetFloat2(const std::string& name, const glm::vec2& value) override;
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
	virtual void setInt(const std::string& name, int value) override;
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
	virtual GLint GetUniformLocation(const std::string& name) override;

	inline std::string GetName() { return m_Name; }

	void Unbind();
	void ClearShader();
	~OpenGLMoravaShader();

	GLuint GetProgramID();

	// Omni shadow maps
	void SetLightMatrices(std::vector<glm::mat4> lightMatrices);

protected:
	void CompileProgram();
	virtual void GetUniformLocations();

	void CompileShader(const char* vertexCode, const char* fragmentCode);
	void CompileShader(const char* vertexCode, const char* geometryCode, const char* fragmentCode);

	void AddShaderVertex(const char* vertexCode);
	void AddShaderFragment(const char* fragmentCode);
	void AddShaderGeometry(const char* geometryCode);
	void AddShaderCompute(const char* computeCode);

	void AddShader(GLuint programID, const char* shaderCode, GLenum shaderType);

	const char* GetShaderTypeNameFromEnum(const GLenum shaderType);

};
