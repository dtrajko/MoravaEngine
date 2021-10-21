#include "DX11Shader.h"

#include "Hazel/Core/Assert.h"
#include "Hazel/Renderer/HazelRenderer.h"

#include "Core/Log.h"
#include "DX11Context.h"
#include "DX11Texture2D.h"

#include <shaderc/shaderc.hpp>
#include <spirv_glsl.hpp>

#include <filesystem>


static std::unordered_map<uint32_t, std::unordered_map<uint32_t, DX11Shader::UniformBuffer*>> s_UniformBuffers; // set -> binding point -> buffer

DX11Shader::DX11Shader()
{
}

DX11Shader::DX11Shader(const std::string& path, bool forceCompile)
	: m_AssetPath(path)
{
	// TODO: This should be more "general"
	size_t found = path.find_last_of("/\\");
	m_Name = found != std::string::npos ? path.substr(found + 1) : path;
	found = m_Name.find_last_of(".");
	m_Name = found != std::string::npos ? m_Name.substr(0, found) : m_Name;

	Reload();
}

DX11Shader::DX11Shader(const wchar_t* vertexShaderPath, const wchar_t* pixelShaderPath)
{
	s_Specification.ShaderType = MoravaShaderSpecification::ShaderType::DX11Shader;
	s_Specification.VertexShaderPath = Util::to_str(vertexShaderPath);
	s_Specification.PixelShaderPath = Util::to_str(pixelShaderPath);

	m_VertexShader = Hazel::Ref<DX11VertexShader>::Create(vertexShaderPath);
	m_PixelShader = Hazel::Ref<DX11PixelShader>::Create(pixelShaderPath);
}

DX11Shader::~DX11Shader()
{
	ClearShader();
}

Hazel::Ref<DX11Shader> DX11Shader::CreateFromString(const std::string& source)
{
	Log::GetLogger()->error("DX11Shader::CreateFromString - method not implemented!");

	Hazel::Ref<DX11Shader> shader = Hazel::Ref<DX11Shader>::Create();
	// shader->Load(source, true);
	return shader;
}

void DX11Shader::Validate()
{
}

void DX11Shader::ClearShader()
{
}

static std::string ReadShaderFromFile(const std::string& filepath)
{
	std::string result;
	std::ifstream in(filepath, std::ios::in | std::ios::binary);
	if (in)
	{
		in.seekg(0, std::ios::end);
		result.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&result[0], result.size());
	}
	else
	{
		HZ_CORE_ASSERT(false, "Could not load shader!");
	}
	in.close();
	return result;
}

void DX11Shader::Reload(bool forceCompile)
{
	// Ref<DX11Shader> instance = this;
	// HazelRenderer::Submit([instance]() mutable
	// {
	// });
	{
		// Vertex and Fragment for now
		std::string source = ReadShaderFromFile(m_AssetPath);
		std::array<std::vector<uint32_t>, 2> shaderData;
		CompileOrGetDX11Binary(shaderData, false);
		LoadAndCreateVertexShader(shaderData[0]);
		LoadAndCreatePixelShader(shaderData[1]);
		Reflect("vertex", shaderData[0]);
		Reflect("pixel", shaderData[1]);
		CreateDescriptors();
	}
}

void DX11Shader::LoadAndCreateVertexShader(const std::vector<uint32_t>& shaderData)
{
	// DX11Device device = DX11Context::GetCurrentDevice()->GetDX11Device();

	HZ_CORE_ASSERT(shaderData.size());
	// Create a new shader module that will be used for pipeline creation

	// TODO: CreateShaderModule
}

void DX11Shader::LoadAndCreatePixelShader(const std::vector<uint32_t>& shaderData)
{
	// DX11Device device = DX11Context::GetCurrentDevice()->GetDX11Device();

	HZ_CORE_ASSERT(shaderData.size());
	// Create a new shader module that will be used for pipeline creation
	
	// TODO: CreateShaderModule
}

void DX11Shader::Reflect(const std::string& shaderStage, const std::vector<uint32_t>& shaderData)
{
	// DX11Device device = DX11Context::GetCurrentDevice()->GetDX11Device();

	MORAVA_CORE_TRACE("==========================");
	MORAVA_CORE_TRACE(" DX11 Shader Reflection");
	MORAVA_CORE_TRACE(" {0}", m_AssetPath);
	MORAVA_CORE_TRACE("==========================");

	// Vertex Shader
	spirv_cross::Compiler compiler(shaderData);
	spirv_cross::ShaderResources resources = compiler.get_shader_resources();

	MORAVA_CORE_TRACE("Uniform Buffers:");
	for (const spirv_cross::Resource& resource : resources.uniform_buffers)
	{
		const auto& name = resource.name;
		auto& bufferType = compiler.get_type(resource.base_type_id);
		int memberCount = static_cast<uint32_t>(bufferType.member_types.size());
		uint32_t bindingPoint = compiler.get_decoration(resource.id, spv::DecorationBinding);
		uint32_t size = static_cast<uint32_t>(compiler.get_declared_struct_size(bufferType));

		HZ_CORE_ASSERT(m_UniformBuffers.find(bindingPoint) == m_UniformBuffers.end());

		UniformBuffer& buffer = m_UniformBuffers[bindingPoint];
		// UniformBuffer buffer;
		buffer.BindingPoint = bindingPoint;
		buffer.Size = size;
		// AllocateUniformBuffer(buffer);
		buffer.Name = name;
		// m_UniformBuffers.insert(std::pair(bindingPoint, buffer));

		MORAVA_CORE_TRACE("  Name: {0}", name);
		MORAVA_CORE_TRACE("  Member Count: {0}", memberCount);
		MORAVA_CORE_TRACE("  Binding Point: {0}", bindingPoint);
		MORAVA_CORE_TRACE("  Size: {0}", size);
		MORAVA_CORE_TRACE("--------------------------");
	}

	MORAVA_CORE_TRACE("Push Constant Buffers:");
	for (const auto& resource : resources.push_constant_buffers)
	{
		const auto& bufferName = resource.name;
		auto& bufferType = compiler.get_type(resource.base_type_id);
		auto bufferSize = compiler.get_declared_struct_size(bufferType);
		int memberCount = static_cast<int>(bufferType.member_types.size());
		uint32_t size = static_cast<uint32_t>(compiler.get_declared_struct_size(bufferType));

		uint32_t offset = 0;
		if (m_PushConstantRanges.size())
		{
			offset = m_PushConstantRanges.back().Offset + m_PushConstantRanges.back().Size;
		}

		auto& pushConstantRange = m_PushConstantRanges.emplace_back();
		pushConstantRange.Size = static_cast<uint32_t>(bufferSize);
		pushConstantRange.Offset = offset;

		// Skip empty push constant buffers - these are for the renderer only
		if (bufferName.empty())
			continue;

		Hazel::ShaderBuffer& buffer = m_Buffers[bufferName];
		buffer.Name = bufferName;
		buffer.Size = static_cast<uint32_t>(bufferSize);

		MORAVA_CORE_TRACE("  Name: {0}", bufferName);
		MORAVA_CORE_TRACE("  Member Count: {0}", memberCount);
		// MORAVA_CORE_TRACE("  Binding Point: {0}", bindingPoint);
		MORAVA_CORE_TRACE("  Size: {0}", size);
		MORAVA_CORE_TRACE("--------------------------");

		for (int i = 0; i < memberCount; i++)
		{
			// auto type = compiler.get_type(bufferType.member_types[i]);
			Hazel::ShaderUniformType type{};
			const auto& memberName = compiler.get_member_name(bufferType.self, i);
			auto size = compiler.get_declared_struct_member_size(bufferType, i);
			auto offset = compiler.type_struct_member_offset(bufferType, i);

			std::string uniformName = bufferName + "." + memberName;
			buffer.Uniforms[uniformName] = Hazel::ShaderUniform(uniformName, type, static_cast<uint32_t>(size), offset);
		}
	}

	MORAVA_CORE_TRACE("Sampled Images:");
	for (const auto& resource : resources.sampled_images)
	{
		const auto& name = resource.name;
		auto& type = compiler.get_type(resource.base_type_id);
		uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
		uint32_t dimension = type.image.dim;

		// HZ_CORE_ASSERT(m_ImageSamplers.find(binding) == m_ImageSamplers.end());

		auto& imageSampler = m_ImageSamplers[binding];
		// ImageSampler imageSampler;
		imageSampler.BindingPoint = binding;
		imageSampler.Name = name;
		// m_ImageSamplers.insert(std::pair(bindingPoint, imageSampler));

		MORAVA_CORE_TRACE("  Name: {0}", name);
		// MORAVA_CORE_TRACE("  Member Count: {0}", memberCount);
		MORAVA_CORE_TRACE("  Binding Point: {0}", binding);
		// MORAVA_CORE_TRACE("  Size: {0}", size);
		MORAVA_CORE_TRACE("--------------------------");
	}

	MORAVA_CORE_TRACE("==========================");
}

void DX11Shader::CreateDescriptors()
{
	// DX11Device device = DX11Context::GetCurrentDevice()->GetDX11Device();

	//////////////////////////////////////////////////////////////////////
	// Descriptor Pool
	//////////////////////////////////////////////////////////////////////

	// We need to tell the API the number of max. requested descriptors per type

	if (m_UniformBuffers.size())
	{
	}

	if (m_ImageSamplers.size())
	{
	}

	// TODO: Move this to the centralized renderer
	// Create the global descriptor pool
	// All descriptors used in this example are allocated from this pool

	// TODO: CreateDescriptorPool

	//////////////////////////////////////////////////////////////////////
	// Descriptor Set Layout
	//////////////////////////////////////////////////////////////////////
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
	for (auto& [binding, uniformBuffer] : m_UniformBuffers)
	{
		AllocateUniformBuffer(uniformBuffer);
	}

	for (auto& [binding, imageSampler] : m_ImageSamplers)
	{
	}

	// TODO: CreateDescriptorSetLayout
}

void DX11Shader::AllocateUniformBuffer(UniformBuffer& dst)
{
	// DX11Device device = DX11Context::GetCurrentDevice()->GetDX11Device();

	UniformBuffer& uniformBuffer = dst;

	// Prepare and initialize an uniform buffer block containing shader uniforms
	// Single uniforms like in OpenGL are no longer present in DX11. All Shader uniforms are passed via uniform buffer blocks

	// Vertex shader uniform buffer block
	// Create a new buffer

	// TODO: CreateBuffer
	// TODO: BindBufferMemory
}

// TODO: does not exist in DX11 Week version, added later
DX11Shader::ShaderMaterialDescriptorSet DX11Shader::CreateDescriptorSets(uint32_t set)
{
	Log::GetLogger()->warn("DX11Shader::CreateDescriptorSets(uint32_t set): Method not yet implemented!");

	return DX11Shader::ShaderMaterialDescriptorSet();
}

// TODO: does not exist in DX11 Week version, added later
DX11Shader::ShaderMaterialDescriptorSet DX11Shader::CreateDescriptorSets(uint32_t set, uint32_t numberOfSets)
{
	Log::GetLogger()->warn("DX11Shader::CreateDescriptorSets(uint32_t set, uint32_t numberOfSets): Method not yet implemented!");

	return DX11Shader::ShaderMaterialDescriptorSet();
}

void DX11Shader::CompileOrGetDX11Binary(std::array<std::vector<uint32_t>, 2>& outputBinary, bool forceCompile)
{
	// Vertex Shader
	{
		std::filesystem::path p = m_AssetPath;
		auto path = p.parent_path() / "cached" / (p.filename().string() + ".cached_DX11.vert");
		std::string cachedFilePath = path.string();

		FILE* f = fopen(cachedFilePath.c_str(), "rb");
		if (f)
		{
			fseek(f, 0, SEEK_END);
			uint64_t size = ftell(f);
			fseek(f, 0, SEEK_SET);
			outputBinary[0] = std::vector<uint32_t>(size / sizeof(uint32_t));
			fread(outputBinary[0].data(), sizeof(uint32_t), outputBinary[0].size(), f);
			fclose(f);
		}
	}

	// Fragment Shader
	{
		std::filesystem::path p = m_AssetPath;
		auto path = p.parent_path() / "cached" / (p.filename().string() + ".cached_DX11.frag");
		std::string cachedFilePath = path.string();

		FILE* f = fopen(cachedFilePath.c_str(), "rb");
		if (f)
		{
			fseek(f, 0, SEEK_END);
			uint64_t size = ftell(f);
			fseek(f, 0, SEEK_SET);
			outputBinary[1] = std::vector<uint32_t>(size / sizeof(uint32_t));
			fread(outputBinary[1].data(), sizeof(uint32_t), outputBinary[1].size(), f);
			fclose(f);
		}
	}

	if (outputBinary[0].size() == 0)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		const bool optimize = false;
		if (optimize) {
			options.SetOptimizationLevel(shaderc_optimization_level_performance);
		}

		// Vertex Shader
		{
		}

		{
			std::filesystem::path p = m_AssetPath;
			auto path = p.parent_path() / "cached" / (p.filename().string() + ".cached_DX11.vert");
			std::string cachedFilePath = path.string();

			FILE* f = fopen(cachedFilePath.c_str(), "wb");
			fwrite(outputBinary[0].data(), sizeof(uint32_t), outputBinary[0].size(), f);
			fclose(f);
		}
	}

	if (outputBinary[1].size() == 0)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		const bool optimize = false;
		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);

		// Pixel Shader
		{
		}

		{
			std::filesystem::path p = m_AssetPath;
			auto path = p.parent_path() / "cached" / (p.filename().string() + ".cached_DX11.frag");
			std::string cachedFilePath = path.string();

			FILE* f = fopen(cachedFilePath.c_str(), "wb");
			fwrite(outputBinary[1].data(), sizeof(uint32_t), outputBinary[1].size(), f);
			fclose(f);
		}
	}
}

static uint32_t ShaderTypeFromString(const std::string& type)
{
	if (type == "vertex")                       return 1;
	if (type == "fragment" || type == "pixel")  return 2;
	if (type == "compute")
	{
		HZ_CORE_ASSERT(false);
	}

	return 0;
}

std::unordered_map<uint32_t, std::string> DX11Shader::PreProcess(const std::string& source)
{
	std::unordered_map<uint32_t, std::string> shaderSources;

	const char* typeToken = "#type";
	size_t typeTokenLength = strlen(typeToken);
	size_t pos = source.find(typeToken, 0);
	while (pos != std::string::npos)
	{
		size_t eol = source.find_first_of("\r\n", pos);
		HZ_CORE_ASSERT(eol != std::string::npos, "Syntax error");
		size_t begin = pos + typeTokenLength + 1;
		std::string type = source.substr(begin, eol - begin);
		HZ_CORE_ASSERT(type == "vertex" || type == "fragment" || type == "pixel" || type == "compute", "Invalid shader type specified");

		size_t nextLinePos = source.find_first_not_of("\r\n", eol);
		pos = source.find(typeToken, nextLinePos);
		auto shaderType = ShaderTypeFromString(type);
		shaderSources[shaderType] = source.substr(nextLinePos, pos - (nextLinePos == std::string::npos ? source.size() - 1 : nextLinePos));
	}

	return shaderSources;
}

void DX11Shader::ClearUniformBuffers()
{
	s_UniformBuffers.clear();
}

size_t DX11Shader::GetHash() const
{
	return std::hash<std::string>{}(m_AssetPath);
}

void DX11Shader::SetUniformBuffer(const std::string& name, const void* data, uint32_t size) {}

void DX11Shader::SetUniform(const std::string& fullname, float value) {}

void DX11Shader::SetUniform(const std::string& fullname, int value) {}

void DX11Shader::SetUniform(const std::string& fullname, const glm::vec2& value) {}

void DX11Shader::SetUniform(const std::string& fullname, const glm::vec3& value) {}

void DX11Shader::SetUniform(const std::string& fullname, const glm::vec4& value) {}

void DX11Shader::SetUniform(const std::string& fullname, const glm::mat3& value) {}

void DX11Shader::SetUniform(const std::string& fullname, const glm::mat4& value) {}

void DX11Shader::SetUniform(const std::string& fullname, uint32_t value) {}

void DX11Shader::SetInt(const std::string& name, int value) {}

void DX11Shader::SetUInt(const std::string& name, uint32_t value) {}

void DX11Shader::SetFloat(const std::string& name, float value) {}

void DX11Shader::SetBool(const std::string& name, bool value) {}

void DX11Shader::SetFloat2(const std::string& name, const glm::vec2& value) {}

void DX11Shader::SetFloat3(const std::string& name, const glm::vec3& value) {}

void DX11Shader::SetFloat4(const std::string& name, const glm::vec4& value) {}

void DX11Shader::SetMat4(const std::string& name, const glm::mat4& value) {}

void DX11Shader::SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind /*= true*/) {}

void DX11Shader::SetIntArray(const std::string& name, int* values, uint32_t size) {}

const std::unordered_map<std::string, Hazel::ShaderBuffer>& DX11Shader::GetShaderBuffers() const { return {}; }

const std::unordered_map<std::string, Hazel::ShaderResourceDeclaration>& DX11Shader::GetResources() const { return {}; }

void DX11Shader::AddShaderReloadedCallback(const ShaderReloadedCallback& callback) {}

GLint DX11Shader::GetUniformLocation(const std::string& name)
{
	Log::GetLogger()->error("DX11Shader::GetUniformLocation() not implemented yet!");
	return GLint();
}

void DX11Shader::UploadUniformMat4(const std::string& name, const glm::mat4& values)
{
	Log::GetLogger()->error("DX11Shader::UploadUniformMat4() not implemented yet!");
}

void DX11Shader::UploadUniformMat4(uint32_t location, const glm::mat4& values)
{
	Log::GetLogger()->error("DX11Shader::UploadUniformMat4() not implemented yet!");
}

void DX11Shader::setLightMat4(std::vector<glm::mat4> lightMatrices)
{
	Log::GetLogger()->error("DX11Shader::setLightMat4() not implemented yet!");
}

void DX11Shader::SetLightMatrices(std::vector<glm::mat4> lightMatrices)
{
	Log::GetLogger()->error("DX11Shader::SetLightMatrices() not implemented yet!");
}

void DX11Shader::Unbind()
{
	Log::GetLogger()->error("DX11Shader::Unbind() not implemented yet!");
}

void DX11Shader::CreateFromString(const char* vertexCode, const char* fragmentCode)
{
	Log::GetLogger()->error("DX11Shader::CreateFromString() not implemented yet!");

}

void DX11Shader::CreateFromFiles(const char* vertexLocation, const char* fragmentLocation)
{
	Log::GetLogger()->error("DX11Shader::CreateFromFiles() not implemented yet!");
}

void DX11Shader::CreateFromFiles(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation)
{
	Log::GetLogger()->error("DX11Shader::CreateFromFiles() not implemented yet!");
}

void DX11Shader::CreateFromFileVertex(const char* vertexLocation)
{
	Log::GetLogger()->error("DX11Shader::CreateFromFileVertex() not implemented yet!");
}

void DX11Shader::CreateFromFileFragment(const char* fragmentLocation)
{
	Log::GetLogger()->error("DX11Shader::CreateFromFileFragment() not implemented yet!");
}

void DX11Shader::CreateFromFileGeometry(const char* geometryLocation)
{
	Log::GetLogger()->error("DX11Shader::CreateFromFileGeometry() not implemented yet!");
}

void DX11Shader::CreateFromFileCompute(const char* computeLocation)
{
	Log::GetLogger()->error("DX11Shader::CreateFromFileCompute() not implemented yet!");
}

void DX11Shader::SetVec2(const std::string& name, const glm::vec2& value)
{
	Log::GetLogger()->error("DX11Shader::SetVec2() not implemented yet!");
}

void DX11Shader::SetVec2(const std::string& name, float x, float y)
{
	Log::GetLogger()->error("DX11Shader::SetVec2() not implemented yet!");
}

void DX11Shader::SetMat2(const std::string& name, const glm::mat2& mat)
{
	Log::GetLogger()->error("DX11Shader::SetMat2() not implemented yet!");
}

void DX11Shader::SetMat3(const std::string& name, const glm::mat3& mat)
{
	Log::GetLogger()->error("DX11Shader::SetMat3() not implemented yet!");
}

std::string DX11Shader::ReadFile(const char* fileLocation)
{
	return std::string();
}

void* DX11Shader::MapUniformBuffer(uint32_t bindingPoint)
{
	// DX11Device device = DX11Context::GetCurrentDevice()->GetDX11Device();

	return nullptr;
}

void DX11Shader::UnmapUniformBuffer(uint32_t bindingPoint)
{
	// DX11Device device = DX11Context::GetCurrentDevice()->GetDX11Device();
}
