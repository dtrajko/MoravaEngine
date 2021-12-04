#pragma once

#include "H2M/Renderer/HazelShader.h"
#include "H2M/Renderer/ShaderUniform.h"

#include "Core/CommonValues.h"
#include "Shader/MoravaShader.h"

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>


class VulkanMoravaShader : public MoravaShader
{
public:
	VulkanMoravaShader();
	VulkanMoravaShader(const char* vertexLocation, const char* fragmentLocation, bool forceCompile = false);
	VulkanMoravaShader(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation, bool forceCompile = false);
	VulkanMoravaShader(const char* computeLocation, bool forceCompile = false);

	static H2M::RefH2M<VulkanMoravaShader> Create(const char* vertexLocation, const char* fragmentLocation, bool forceCompile = false);
	static H2M::RefH2M<VulkanMoravaShader> Create(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation, bool forceCompile = false);
	static H2M::RefH2M<VulkanMoravaShader> Create(const char* computeLocation, bool forceCompile = false);

	// virtual methods
	virtual void Bind() override;
	virtual void Reload(bool forceCompile = false) override;

	// HazelShader abstract methods
	virtual H2M::RendererID_H2M GetRendererID() const override;
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
	virtual void SetBool(const std::string& name, bool value) override;
	virtual void SetFloat2(const std::string& name, const glm::vec2& value) override;
	virtual void SetFloat3(const std::string& name, const glm::vec3& value) override;
	virtual void SetFloat4(const std::string& name, const glm::vec4& value) override;
	virtual void SetMat2(const std::string& name, const glm::mat2& mat) override;
	virtual void SetMat3(const std::string& name, const glm::mat3& mat) override;
	virtual void SetMat4(const std::string& name, const glm::mat4& value) override;
	virtual void SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind = true) override;
	virtual void SetIntArray(const std::string& name, int* values, uint32_t size) override;

	// generic setter methods for uniform location variables
	virtual void SetVec2(const std::string& name, const glm::vec2& value) override; // TODO: remove, use SetFloat2 instead
	virtual void SetVec2(const std::string& name, float x, float y) override;       // TODO: remove, use SetFloat2 instead

	virtual void AddShaderReloadedCallback(const ShaderReloadedCallback& callback) override;

	virtual GLint GetUniformLocation(const std::string& name) override;
	virtual void UploadUniformMat4(const std::string& name, const glm::mat4& values) override;
	virtual void UploadUniformMat4(uint32_t location, const glm::mat4& values) override;
	virtual void setLightMat4(std::vector<glm::mat4> lightMatrices) override;
	// Omni shadow maps
	virtual void SetLightMatrices(std::vector<glm::mat4> lightMatrices) override;

	virtual void Unbind() override;

	virtual const std::unordered_map<std::string, H2M::ShaderBuffer>& GetShaderBuffers() const override;
	virtual const std::unordered_map<std::string, H2M::ShaderResourceDeclaration>& GetResources() const override;

	virtual void CreateFromString(const char* vertexCode, const char* fragmentCode) override;
	virtual void CreateFromFiles(const char* vertexLocation, const char* fragmentLocation) override;
	virtual void CreateFromFiles(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation) override;

	virtual void CreateFromFileVertex(const char* vertexLocation) override;
	virtual void CreateFromFileFragment(const char* fragmentLocation) override;
	virtual void CreateFromFileGeometry(const char* geometryLocation) override;
	virtual void CreateFromFileCompute(const char* computeLocation) override;

	virtual void Validate();
	virtual void ClearShader() override;

	static std::string ReadFile(const char* fileLocation);

	~VulkanMoravaShader();

	GLuint GetProgramID();

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
