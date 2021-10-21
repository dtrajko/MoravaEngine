#pragma once

#include "Hazel/Renderer/HazelShader.h"
#include "Hazel/Renderer/ShaderUniform.h"

#include "Core/CommonValues.h"

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>


struct MoravaShaderSpecification
{
	enum class ShaderType
	{
		None = 0,
		MoravaShader,
		HazelShader,
		DX11Shader,
	};
	ShaderType ShaderType = ShaderType::None;

	// MoravaShader
	std::string VertexShaderPath   = "";
	std::string FragmentShaderPath = "";
	std::string GeometryShaderPath = "";
	std::string ComputeShaderPath  = "";
	// HazelShader
	std::string HazelShaderPath    = "";
	// DX11Shader
	std::string PixelShaderPath    = "";

	bool ForceCompile = false;
};


class MoravaShader : public Hazel::HazelShader
{
public:
	// the ultimate Create method that can create both MoravaShader and HazelShader shader types
	static Hazel::Ref<MoravaShader> Create(MoravaShaderSpecification moravaShaderSpecification);
	static Hazel::Ref<MoravaShader> Create(const char* vertexLocation, const char* fragmentLocation, bool forceCompile = false);
	static Hazel::Ref<MoravaShader> Create(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation, bool forceCompile = false);
	static Hazel::Ref<MoravaShader> Create(const char* computeLocation, bool forceCompile = false);

	MoravaShader();
	MoravaShader(const char* vertexLocation, const char* fragmentLocation, bool forceCompile = false);
	MoravaShader(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation, bool forceCompile = false);
	MoravaShader(const char* computeLocation, bool forceCompile = false);
	~MoravaShader();

	// HazelShader abstract methods
	virtual size_t GetHash() const override;

	// virtual methods
	virtual void Reload(bool forceCompile = false) override;

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
	virtual void SetBool(const std::string& name, bool value);
	virtual void SetFloat2(const std::string& name, const glm::vec2& value) override;
	virtual void SetFloat3(const std::string& name, const glm::vec3& value) override;
	virtual void SetFloat4(const std::string& name, const glm::vec4& value) override;
	virtual void SetMat2(const std::string& name, const glm::mat2& mat);
	virtual void SetMat3(const std::string& name, const glm::mat3& mat);
	virtual void SetMat4(const std::string& name, const glm::mat4& value) override;
	virtual void SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind = true) override;
	virtual void SetIntArray(const std::string& name, int* values, uint32_t size) override;

	// generic setter methods for uniform location variables
	virtual void SetVec2(const std::string& name, const glm::vec2& value); // TODO: remove, use SetFloat2 instead
	virtual void SetVec2(const std::string& name, float x, float y);       // TODO: remove, use SetFloat2 instead

	virtual void AddShaderReloadedCallback(const ShaderReloadedCallback& callback) override;

	virtual GLint GetUniformLocation(const std::string& name);
	virtual void UploadUniformMat4(const std::string& name, const glm::mat4& values);
	virtual void UploadUniformMat4(uint32_t location, const glm::mat4& values);
	virtual void setLightMat4(std::vector<glm::mat4> lightMatrices);
	// Omni shadow maps
	virtual void SetLightMatrices(std::vector<glm::mat4> lightMatrices);

	virtual void Unbind();

	virtual const std::string& GetName() const override { return m_Name; }
	virtual const std::unordered_map<std::string, Hazel::ShaderBuffer>& GetShaderBuffers() const override;
	virtual const std::unordered_map<std::string, Hazel::ShaderResourceDeclaration>& GetResources() const override;

	virtual void CreateFromString(const char* vertexCode, const char* fragmentCode);
	virtual void CreateFromFiles(const char* vertexLocation, const char* fragmentLocation);
	virtual void CreateFromFiles(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation);

	virtual void CreateFromFileVertex(const char* vertexLocation);
	virtual void CreateFromFileFragment(const char* fragmentLocation);
	virtual void CreateFromFileGeometry(const char* geometryLocation);
	virtual void CreateFromFileCompute(const char* computeLocation);

	virtual void Validate();
	virtual void ClearShader();

	static std::string ReadFile(const char* fileLocation);

	GLuint GetProgramID();

	Hazel::RendererID GetRendererID() const;
	void Bind();

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

	// Vulkan Week Day 1 (removed later)
	// virtual const Hazel::ShaderUniformBufferList& GetVSRendererUniforms() const override { return m_VSRendererUniformBuffers; }
	// virtual const Hazel::ShaderUniformBufferList& GetPSRendererUniforms() const override { return m_PSRendererUniformBuffers; }
	// virtual bool HasVSMaterialUniformBuffer() const override { return (bool)m_VSMaterialUniformBuffer; }
	// virtual bool HasPSMaterialUniformBuffer() const override { return (bool)m_PSMaterialUniformBuffer; }
	// virtual const Hazel::ShaderUniformBufferDeclaration& GetVSMaterialUniformBuffer() const override { return *m_VSMaterialUniformBuffer; }
	// virtual const Hazel::ShaderUniformBufferDeclaration& GetPSMaterialUniformBuffer() const override { return *m_PSMaterialUniformBuffer; }
	// virtual const ShaderResourceList& GetResources() const override { return m_Resources; }

protected:
	GLuint programID = -1;
	GLint shaderID = -1;

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

	// Temporary, before we have an asset manager
	static std::vector<Hazel::Ref<MoravaShader>> s_AllShaders;

	// Vulkan Week Day 1
	Hazel::ShaderUniformBufferList m_VSRendererUniformBuffers;
	Hazel::ShaderUniformBufferList m_PSRendererUniformBuffers;
	Ref<Hazel::ShaderUniformBufferDeclaration> m_VSMaterialUniformBuffer;
	Ref<Hazel::ShaderUniformBufferDeclaration> m_PSMaterialUniformBuffer;

	static MoravaShaderSpecification s_Specification;

};
