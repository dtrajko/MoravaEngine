#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"

#include "Shader/MoravaShader.h"
#include "Hazel/Renderer/HazelTexture.h"


class DX11Shader : public MoravaShader
{
public:
	struct UniformBuffer
	{
		uint32_t Size = 0;
		uint32_t BindingPoint = 0;
		std::string Name;
	};

	struct ImageSampler
	{
		uint32_t BindingPoint = 0;
		std::string Name;
	};

	struct PushConstantRange
	{
		uint32_t Offset = 0;
		uint32_t Size = 0;
	};

	enum class Type
	{
		None = 0,
		Vertex,
		Pixel,
	};

public:
	DX11Shader(const std::string& path, bool forceCompile);
	// DirectX 11
	DX11Shader(const wchar_t* vertexShaderPath, const wchar_t* pixelShaderPath);
	virtual ~DX11Shader();

	virtual void Bind() override;
	virtual void Reload(bool forceCompile = false) override;

	virtual size_t GetHash() const override;
	virtual Hazel::RendererID GetRendererID() const override;

	virtual void SetUniformBuffer(const std::string& name, const void* data, uint32_t size) override;

	virtual void SetUniform(const std::string& fullname, uint32_t value) override;
	virtual void SetUniform(const std::string& fullname, float value) override;
	virtual void SetUniform(const std::string& fullname, int value) override;
	virtual void SetUniform(const std::string& fullname, const glm::vec2& value) override;
	virtual void SetUniform(const std::string& fullname, const glm::vec3& value) override;
	virtual void SetUniform(const std::string& fullname, const glm::vec4& value) override;
	virtual void SetUniform(const std::string& fullname, const glm::mat3& value) override;
	virtual void SetUniform(const std::string& fullname, const glm::mat4& value) override;

	virtual void SetInt(const std::string& name, int value) override;
	virtual void SetUInt(const std::string& name, uint32_t value) override;
	virtual void SetFloat(const std::string& name, float value) override;
	virtual void SetFloat2(const std::string& name, const glm::vec2& value) override;
	virtual void SetFloat3(const std::string& name, const glm::vec3& value) override;
	virtual void SetFloat4(const std::string& name, const glm::vec4& value) override;
	virtual void SetMat4(const std::string& name, const glm::mat4& value) override;
	virtual void SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind = true) override;
	virtual void SetIntArray(const std::string& name, int* values, uint32_t size) override;
	virtual const std::string& GetName() const override { return m_Name; }
	virtual const std::unordered_map<std::string, Hazel::ShaderBuffer>& GetShaderBuffers() const override;
	virtual const std::unordered_map<std::string, Hazel::ShaderResourceDeclaration>& GetResources() const override;
	virtual void AddShaderReloadedCallback(const ShaderReloadedCallback& callback) override;

	void* MapUniformBuffer(uint32_t bindingPoint);
	void UnmapUniformBuffer(uint32_t bindingPoint);

	UniformBuffer& GetUniformBuffer() { return m_UniformBuffers[0]; }
	const std::vector<PushConstantRange>& GetPushConstantRanges() const { return m_PushConstantRanges; }

	uint32_t GetUniformBufferCount(uint32_t set = 0)
	{
		if (m_ShaderDescriptorSets.find(set) == m_ShaderDescriptorSets.end())
			return 0;

		return (uint32_t)m_ShaderDescriptorSets.at(set).UniformBuffers.size();
	}

	std::unordered_map<uint32_t, std::string> PreProcess(const std::string& source);

	struct ShaderDescriptorSet
	{
		std::unordered_map<uint32_t, UniformBuffer*> UniformBuffers;
		std::unordered_map<uint32_t, ImageSampler> ImageSamplers;
		std::unordered_map<uint32_t, ImageSampler> StorageImages;
	};
	const std::unordered_map<uint32_t, ShaderDescriptorSet>& GetShaderDescriptorSets() const { return m_ShaderDescriptorSets; }
		
	struct ShaderMaterialDescriptorSet
	{
	};

	ShaderMaterialDescriptorSet CreateDescriptorSets(uint32_t set = 0);
	ShaderMaterialDescriptorSet CreateDescriptorSets(uint32_t set, uint32_t numberOfSets);

	static void ClearUniformBuffers();

	// DirectX 11 public methods
	ID3D11VertexShader* GetVertexShaderDX11() { return m_VertexShaderDX11; }
	ID3D11PixelShader* GetPixelShaderDX11() { return m_PixelShaderDX11; }

	void SetVertexShaderDX11(ID3D11VertexShader* vertexShader) { m_VertexShaderDX11 = vertexShader; }
	void SetPixelShaderDX11(ID3D11PixelShader* pixelShader) { m_PixelShaderDX11 = pixelShader; }

	// DX11-specific GET methods
	const void* GetBytecodeWithInputSignature();
	size_t GetBytecodeLength();

private:
	void CompileOrGetDX11Binary(std::array<std::vector<uint32_t>, 2>& outputBinary, bool forceCompile);
	void LoadAndCreateVertexShader(const std::vector<uint32_t>& shaderData);
	void LoadAndCreatePixelShader(const std::vector<uint32_t>& shaderData);

	void Reflect(const std::string& shaderStage, const std::vector<uint32_t>& shaderData);
	void CreateDescriptors();

	void AllocateUniformBuffer(UniformBuffer& dst);

	// DirectX 11 private methods
	bool CompileDX11Shader(const wchar_t* fileName, Type shaderType, const char* entryPointName, void** shaderByteCodeOut, size_t* byteCodeSizeOut);
	void ReleaseCompiledDX11Shader();

	virtual void ClearShader() override;

private:
	std::string m_AssetPath;
	std::string m_Name;

	std::unordered_map<uint32_t, ShaderDescriptorSet> m_ShaderDescriptorSets;

	std::unordered_map<uint32_t, UniformBuffer> m_UniformBuffers;
	std::unordered_map<uint32_t, ImageSampler> m_ImageSamplers;
	std::vector<PushConstantRange> m_PushConstantRanges;

	std::unordered_map<std::string, Hazel::ShaderBuffer> m_Buffers;

	// DirectX 11 specific
	ID3D11VertexShader* m_VertexShaderDX11;
	ID3D11PixelShader* m_PixelShaderDX11;

	ID3DBlob* m_Blob = nullptr;

	friend class DX11Material;

};
