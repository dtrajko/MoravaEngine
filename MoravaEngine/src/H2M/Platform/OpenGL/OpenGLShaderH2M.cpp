/**
 *
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "OpenGLShaderH2M.h"

#include "H2M/Renderer/RendererH2M.h"
#include "H2M/Utilities/FileSystemH2M.h"

#include <string>
#include <sstream>
#include <limits>

#include <glm/gtc/type_ptr.hpp>

#include <shaderc/shaderc.hpp>
#include <filesystem>



namespace H2M
{

#define UNIFORM_LOGGING 0
#if UNIFORM_LOGGING
#define HZ_LOG_UNIFORM(...) H2M_CORE_WARN(__VA_ARGS__)
#else
#define HZ_LOG_UNIFORM
#endif

#define PRINT_SHADERS 1

	namespace Utils {

		static const char* GetCacheDirectory()
		{
			// TODO: make sure the assets directory is valid
			return "assets/cache/shader/opengl";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			std::string cacheDirectory = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDirectory))
				std::filesystem::create_directories(cacheDirectory);
		}

	}

	OpenGLShaderH2M::OpenGLShaderH2M(const std::string& filepath, bool forceRecompile)
		: m_AssetPath(filepath)
	{
		size_t found = filepath.find_last_of("/\\");
		m_Name = found != std::string::npos ? filepath.substr(found + 1) : filepath;
		found = m_Name.find_last_of(".");
		m_Name = found != std::string::npos ? m_Name.substr(0, found) : m_Name;

		Reload(forceRecompile);
	}

	RefH2M<OpenGLShaderH2M> OpenGLShaderH2M::CreateFromString(const std::string& source)
	{
		RefH2M<OpenGLShaderH2M> shader = RefH2M<OpenGLShaderH2M>::Create();
		shader->Load(source, true);
		return shader;
	}

	void OpenGLShaderH2M::Reload(bool forceCompile)
	{
		std::string source = ReadShaderFromFile(m_AssetPath);
		Load(source, forceCompile);
	}

	void OpenGLShaderH2M::Load(const std::string& source, bool forceCompile)
	{
		m_ShaderSource = PreProcess(source);
		Utils::CreateCacheDirectoryIfNeeded();
		RefH2M<OpenGLShaderH2M> instance = this;
		RendererH2M::Submit([instance, forceCompile]() mutable
		{
			std::array<std::vector<uint32_t>, 2> vulkanBinaries;
			std::unordered_map<uint32_t, std::vector<uint32_t>> shaderData;
			instance->CompileOrGetVulkanBinary(shaderData, forceCompile);
			instance->CompileOrGetOpenGLBinary(shaderData, forceCompile);
		});
	}

	void OpenGLShaderH2M::ClearUniformBuffers()
	{
		s_UniformBuffers.clear();
	}

	static const char* GLShaderStageCachedVulkanFileExtension(uint32_t stage)
	{
		switch (stage)
		{
		case GL_VERTEX_SHADER:    return ".cached_vulkan.vert";
		case GL_FRAGMENT_SHADER:  return ".cached_vulkan.frag";
		case GL_COMPUTE_SHADER:   return ".cached_vulkan.comp";
		}
		H2M_CORE_ASSERT(false);
		return "";
	}

	static const char* GLShaderStageCachedOpenGLFileExtension(uint32_t stage)
	{
		switch (stage)
		{
		case GL_VERTEX_SHADER:    return ".cached_opengl.vert";
		case GL_FRAGMENT_SHADER:  return ".cached_opengl.frag";
		case GL_COMPUTE_SHADER:   return ".cached_opengl.comp";
		}
		H2M_CORE_ASSERT(false);
		return "";
	}

	static shaderc_shader_kind GLShaderStageToShaderC(uint32_t stage)
	{
		switch (stage)
		{
		case GL_VERTEX_SHADER:    return shaderc_vertex_shader;
		case GL_FRAGMENT_SHADER:  return shaderc_fragment_shader;
		case GL_COMPUTE_SHADER:   return shaderc_compute_shader;
		}
		H2M_CORE_ASSERT(false);
		return (shaderc_shader_kind)0;
	}

	static const char* GLShaderTypeToString(uint32_t stage)
	{
		switch (stage)
		{
		case GL_VERTEX_SHADER:    return "Vertex";
		case GL_FRAGMENT_SHADER:  return "Fragment";
		case GL_COMPUTE_SHADER:   return "Compute";
		}
		H2M_CORE_ASSERT(false);
		return "";
	}

	void OpenGLShaderH2M::CompileOrGetVulkanBinary(std::unordered_map<uint32_t, std::vector<uint32_t>>& outputBinary, bool forceCompile)
	{
		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();
		for (auto [stage, source] : m_ShaderSource)
		{
			auto extension = GLShaderStageCachedVulkanFileExtension(stage);
			std::filesystem::path p = m_AssetPath;
			if (!forceCompile)
			{
				std::filesystem::path p = m_AssetPath;
				auto path = cacheDirectory / (p.filename().string() + extension);
				std::string cachedFilePath = path.string();

				FILE* f = fopen(cachedFilePath.c_str(), "rb");
				if (f)
				{
					fseek(f, 0, SEEK_END);
					uint64_t size = ftell(f);
					fseek(f, 0, SEEK_SET);
					outputBinary[stage] = std::vector<uint32_t>(size / sizeof(uint32_t));
					fread(outputBinary[stage].data(), sizeof(uint32_t), outputBinary[stage].size(), f);
					fclose(f);
				}
			}

			if (outputBinary[stage].size() == 0)
			{
				// Do we need to init a compiler for each stage?
				shaderc::Compiler compiler;
				shaderc::CompileOptions options;
				options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
				options.AddMacroDefinition("OPENGL");
				const bool optimize = false;
				if (optimize)
					options.SetOptimizationLevel(shaderc_optimization_level_performance);

				// Compile shader
				{
					auto& shaderSource = m_ShaderSource.at(stage);
					shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(shaderSource, GLShaderStageToShaderC(stage), m_AssetPath.c_str(), options);

					if (module.GetCompilationStatus() != shaderc_compilation_status_success)
					{
						H2M_CORE_ERROR(module.GetErrorMessage());
						H2M_CORE_ASSERT(false);
					}

					const uint8_t* begin = (const uint8_t*)module.cbegin();
					const uint8_t* end = (const uint8_t*)module.cend();
					const ptrdiff_t size = end - begin;

					outputBinary[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());
				}

				// Cache compiled shader
				{
					std::filesystem::path p = m_AssetPath;
					auto path = cacheDirectory / (p.filename().string() + extension);
					std::string cachedFilePath = path.string();

					FILE* f = fopen(cachedFilePath.c_str(), "wb");
					fwrite(outputBinary[stage].data(), sizeof(uint32_t), outputBinary[stage].size(), f);
					fclose(f);
				}
			}
		}
	}

	void OpenGLShaderH2M::CompileOrGetOpenGLBinary(const std::unordered_map<uint32_t, std::vector<uint32_t>>& vulkanBinaries, bool forceCompile)
	{
		if (m_RendererID)
			glDeleteProgram(m_RendererID);

		GLuint program = glCreateProgram();
		m_RendererID = program;

		std::vector<GLuint> shaderRendererIDs;
		shaderRendererIDs.reserve(vulkanBinaries.size());

		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

		m_ConstantBufferOffset = 0;
		std::vector<std::vector<uint32_t>> shaderData;
		for (auto [stage, binary] : vulkanBinaries)
		{
			shaderc::Compiler compiler;
			shaderc::CompileOptions options;
			options.SetTargetEnvironment(shaderc_target_env_opengl_compat, shaderc_env_version_opengl_4_5);

			{
				spirv_cross::CompilerGLSL glsl(binary);
				ParseConstantBuffers(glsl);

				std::filesystem::path p = m_AssetPath;
				auto path = cacheDirectory / (p.filename().string() + GLShaderStageCachedOpenGLFileExtension(stage));
				std::string cachedFilePath = path.string();

				std::vector<uint32_t>& shaderStageData = shaderData.emplace_back();

				if (!forceCompile)
				{
					FILE* f = fopen(cachedFilePath.c_str(), "rb");
					if (f)
					{
						fseek(f, 0, SEEK_END);
						uint64_t size = ftell(f);
						fseek(f, 0, SEEK_SET);
						shaderStageData = std::vector<uint32_t>(size / sizeof(uint32_t));
						fread(shaderStageData.data(), sizeof(uint32_t), shaderStageData.size(), f);
						fclose(f);
					}
				}

				if (!shaderStageData.size())
				{
					std::string source = glsl.compile();
#if PRINT_SHADERS
					printf("=========================================\n");
					printf("%s Shader:\n%s\n", GLShaderTypeToString(stage), source.c_str());
					printf("=========================================\n");
#endif
					shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, GLShaderStageToShaderC(stage), m_AssetPath.c_str(), options);

					if (module.GetCompilationStatus() != shaderc_compilation_status_success)
					{
						H2M_CORE_ERROR(module.GetErrorMessage());
						H2M_CORE_ASSERT(false);
					}

					shaderStageData = std::vector<uint32_t>(module.cbegin(), module.cend());

					{
						std::filesystem::path p = m_AssetPath;
						auto path = cacheDirectory / (p.filename().string() + GLShaderStageCachedOpenGLFileExtension(stage));
						std::string cachedFilePath = path.string();
						FILE* f = fopen(cachedFilePath.c_str(), "wb");
						fwrite(shaderStageData.data(), sizeof(uint32_t), shaderStageData.size(), f);
						fclose(f);
					}
				}

				GLuint shaderID = glCreateShader(stage);
				glShaderBinary(1, &shaderID, GL_SHADER_BINARY_FORMAT_SPIR_V, shaderStageData.data(), static_cast<GLsizei>(shaderStageData.size()) * sizeof(uint32_t));
				glSpecializeShader(shaderID, "main", 0, nullptr, nullptr);
				glAttachShader(program, shaderID);

				shaderRendererIDs.emplace_back(shaderID);
			}
		}

		// Link shader program
		glLinkProgram(program);

		// Note the different functions here: glGetProgram* instead of glGetShader*.
		GLint isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
			H2M_CORE_ERROR("Shader compilation failed ({0}):\n{1}", m_AssetPath, &infoLog[0]);

			// We don't need the program anymore.
			glDeleteProgram(program);
			// Don't leak shaders either.
			for (auto id : shaderRendererIDs)
				glDeleteShader(id);
		}

		// Always detach shaders after a successful link.
		for (auto id : shaderRendererIDs)
			glDetachShader(program, id);

		// Get uniform locations
		for (auto& [bufferName, buffer] : m_Buffers)
		{
			for (auto& [name, uniform] : buffer.Uniforms)
			{
				GLint location = glGetUniformLocation(m_RendererID, name.c_str());
				if (location == -1)
					H2M_CORE_WARN("{0}: could not find uniform location {0}", name);

				m_UniformLocations[name] = location;
			}
		}

		for (auto& shaderStageData : shaderData)
		{
			Reflect(shaderStageData);
		}
	}

	static ShaderUniformTypeH2M SPIRTypeToShaderUniformType(spirv_cross::SPIRType type)
	{
		switch (type.basetype)
		{
		case spirv_cross::SPIRType::Boolean:  return ShaderUniformTypeH2M::Bool;
		case spirv_cross::SPIRType::Int:      return ShaderUniformTypeH2M::Int;
		case spirv_cross::SPIRType::UInt:     return ShaderUniformTypeH2M::UInt;
		case spirv_cross::SPIRType::Float:
			if (type.vecsize == 1)            return ShaderUniformTypeH2M::Float;
			if (type.vecsize == 2)            return ShaderUniformTypeH2M::Vec2;
			if (type.vecsize == 3)            return ShaderUniformTypeH2M::Vec3;
			if (type.vecsize == 4)            return ShaderUniformTypeH2M::Vec4;

			if (type.columns == 3)            return ShaderUniformTypeH2M::Mat3;
			if (type.columns == 4)            return ShaderUniformTypeH2M::Mat4;
			break;
		}
		H2M_CORE_ASSERT(false, "Unknown type!");
		return ShaderUniformTypeH2M::None;
	}

	void OpenGLShaderH2M::Compile(const std::vector<uint32_t>& vertexBinary, const std::vector<uint32_t>& fragmentBinary)
	{

	}

	void OpenGLShaderH2M::ParseConstantBuffers(const spirv_cross::CompilerGLSL& compiler)
	{
		spirv_cross::ShaderResources res = compiler.get_shader_resources();
		for (const spirv_cross::Resource& resource : res.push_constant_buffers)
		{
			const auto& bufferName = resource.name;
			auto& bufferType = compiler.get_type(resource.base_type_id);
			auto bufferSize = compiler.get_declared_struct_size(bufferType);

			// Skip empty push constant buffers - these are for the renderer only
			if (bufferName.empty() || bufferName == "u_Renderer")
			{
				m_ConstantBufferOffset += static_cast<uint32_t>(bufferSize);
				continue;
			}

			auto location = compiler.get_decoration(resource.id, spv::DecorationLocation);
			int memberCount = static_cast<int>(bufferType.member_types.size());
			ShaderBufferH2M& buffer = m_Buffers[bufferName];
			buffer.Name = bufferName;
			buffer.Size = static_cast<uint32_t>(bufferSize) - m_ConstantBufferOffset;
			for (int i = 0; i < memberCount; i++)
			{
				auto type = compiler.get_type(bufferType.member_types[i]);
				const auto& memberName = compiler.get_member_name(bufferType.self, i);
				auto size = compiler.get_declared_struct_member_size(bufferType, i);
				auto offset = compiler.type_struct_member_offset(bufferType, i) - m_ConstantBufferOffset;

				std::string uniformName = bufferName + "." + memberName;
				buffer.Uniforms[uniformName] = ShaderUniformH2M(uniformName, SPIRTypeToShaderUniformType(type), static_cast<uint32_t>(size), offset);
			}

			m_ConstantBufferOffset += static_cast<uint32_t>(bufferSize);
		}
	}

	void OpenGLShaderH2M::Reflect(std::vector<uint32_t>& data)
	{
		spirv_cross::Compiler comp(data);
		spirv_cross::ShaderResources res = comp.get_shader_resources();

		MORAVA_CORE_TRACE("OpenGLShaderH2M::Reflect - {0}", m_AssetPath);
		MORAVA_CORE_TRACE("   {0} Uniform Buffers", res.uniform_buffers.size());
		MORAVA_CORE_TRACE("   {0} Resources", res.sampled_images.size());

		glUseProgram(m_RendererID);

		uint32_t bufferIndex = 0;
		for (const spirv_cross::Resource& resource : res.uniform_buffers)
		{
			auto& bufferType = comp.get_type(resource.base_type_id);
			int memberCount = static_cast<int>(bufferType.member_types.size());
			uint32_t bindingPoint = comp.get_decoration(resource.id, spv::DecorationBinding);
			uint32_t bufferSize = static_cast<uint32_t>(comp.get_declared_struct_size(bufferType));

			if (s_UniformBuffers.find(bindingPoint) == s_UniformBuffers.end())
			{
				ShaderUniformBufferH2M& buffer = s_UniformBuffers[bindingPoint];
				buffer.Name = resource.name;
				buffer.BindingPoint = bindingPoint;
				buffer.Size = bufferSize;

#if 0
				buffer.Uniforms.reserve(memberCount);
				for (int i = 0; i < memberCount; i++)
				{
					auto type = comp.get_type(bufferType.member_types[i]);
					const auto& name = comp.get_member_name(bufferType.self, i);
					auto size = comp.get_declared_struct_member_size(bufferType, i);
					auto offset = comp.type_struct_member_offset(bufferType, i);

					//ShaderUniformType uniformType = SPIRTypeToShaderUniformType(type);
					//buffer.Uniforms.emplace_back(name, uniformType, size, offset);
				}
#endif
				glCreateBuffers(1, &buffer.RendererID);
				glBindBuffer(GL_UNIFORM_BUFFER, buffer.RendererID);
				glBufferData(GL_UNIFORM_BUFFER, buffer.Size, nullptr, GL_DYNAMIC_DRAW);
				glBindBufferBase(GL_UNIFORM_BUFFER, buffer.BindingPoint, buffer.RendererID);

				MORAVA_CORE_TRACE("Created Uniform Buffer at binding point {0} with name '{1}', size is {2} bytes", buffer.BindingPoint, buffer.Name, buffer.Size);

				glBindBuffer(GL_UNIFORM_BUFFER, 0);
			}
			else
			{
				// Validation
				ShaderUniformBufferH2M& buffer = s_UniformBuffers.at(bindingPoint);
				H2M_CORE_ASSERT(buffer.Name == resource.name); // Must be the same buffer
				if (bufferSize > buffer.Size) // Resize buffer if needed
				{
					buffer.Size = bufferSize;

					glDeleteBuffers(1, &buffer.RendererID);
					glCreateBuffers(1, &buffer.RendererID);
					glBindBuffer(GL_UNIFORM_BUFFER, buffer.RendererID);
					glBufferData(GL_UNIFORM_BUFFER, buffer.Size, nullptr, GL_DYNAMIC_DRAW);
					glBindBufferBase(GL_UNIFORM_BUFFER, buffer.BindingPoint, buffer.RendererID);

					MORAVA_CORE_TRACE("Resized Uniform Buffer at binding point {0} with name '{1}', size is {2} bytes", buffer.BindingPoint, buffer.Name, buffer.Size);
				}
			}
		}

		int32_t sampler = 0;
		for (const spirv_cross::Resource& resource : res.sampled_images)
		{
			auto& type = comp.get_type(resource.base_type_id);
			auto binding = comp.get_decoration(resource.id, spv::DecorationBinding);
			const auto& name = resource.name;
			uint32_t dimension = type.image.dim;

			GLint location = glGetUniformLocation(m_RendererID, name.c_str());
			//H2M_CORE_ASSERT(location != -1);
			m_Resources[name] = ShaderResourceDeclarationH2M(name, binding, 1);
			glUniform1i(location, binding);
		}
	}

	void OpenGLShaderH2M::AddShaderReloadedCallback(const ShaderReloadedCallback& callback)
	{
		m_ShaderReloadedCallbacks.push_back(callback);
	}

	void OpenGLShaderH2M::Bind()
	{
		RendererH2M::Submit([=]() {
			glUseProgram(m_RendererID);
		});
	}

	std::string OpenGLShaderH2M::ReadShaderFromFile(const std::string& filepath) const
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
			H2M_CORE_ASSERT(false, "Could not load shader!");
		}
		in.close();
		return result;
	}

	std::unordered_map<GLenum, std::string> OpenGLShaderH2M::PreProcess(const std::string& source)
	{
		std::unordered_map<GLenum, std::string> shaderSources;

		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken, 0);
		while (pos != std::string::npos)
		{
			size_t eol = source.find_first_of("\r\n", pos);
			H2M_CORE_ASSERT(eol != std::string::npos, "Syntax error");
			size_t begin = pos + typeTokenLength + 1;
			std::string type = source.substr(begin, eol - begin);
			H2M_CORE_ASSERT(type == "vertex" || type == "fragment" || type == "pixel" || type == "compute", "Invalid shader type specified");

			size_t nextLinePos = source.find_first_not_of("\r\n", eol);
			pos = source.find(typeToken, nextLinePos);
			auto shaderType = ShaderTypeFromString(type);
			shaderSources[shaderType] = source.substr(nextLinePos, pos - (nextLinePos == std::string::npos ? source.size() - 1 : nextLinePos));

			// Compute shaders cannot contain other types
			if (shaderType == GL_COMPUTE_SHADER)
			{
				m_IsCompute = true;
				break;
			}
		}

		return shaderSources;
	}

	// Parsing helper functions
	const char* FindToken(const char* str, const std::string& token)
	{
		const char* t = str;
		while (t = strstr(t, token.c_str()))
		{
			bool left = str == t || isspace(t[-1]);
			bool right = !t[token.size()] || isspace(t[token.size()]);
			if (left && right)
				return t;

			t += token.size();
		}
		return nullptr;
	}

	const char* FindToken(const std::string& string, const std::string& token)
	{
		return FindToken(string.c_str(), token);
	}

	std::string GetBlock(const char* str, const char** outPosition)
	{
		const char* end = strstr(str, "}");
		if (!end)
			return str;

		if (outPosition)
			*outPosition = end;
		uint32_t length = static_cast<uint32_t>(end - str + 1);
		return std::string(str, length);
	}

	std::string GetStatement(const char* str, const char** outPosition)
	{
		const char* end = strstr(str, ";");
		if (!end)
			return str;

		if (outPosition)
			*outPosition = end;
		uint32_t length = static_cast<uint32_t>(end - str + 1);
		return std::string(str, length);
	}

	bool StartsWith(const std::string& string, const std::string& start)
	{
		return string.find(start) == 0;
	}

	static bool IsTypeStringResource(const std::string& type)
	{
		if (type == "sampler1D")		return true;
		if (type == "sampler2D")		return true;
		if (type == "sampler2DMS")		return true;
		if (type == "samplerCube")		return true;
		if (type == "sampler2DShadow")	return true;
		return false;
	}

	int32_t OpenGLShaderH2M::GetUniformLocation(const std::string& name) const
	{
		int32_t result = glGetUniformLocation(m_RendererID, name.c_str());
		if (result == -1)
			H2M_CORE_WARN("Could not find uniform '{0}' in shader", name);

		return result;
	}

	GLenum OpenGLShaderH2M::ShaderTypeFromString(const std::string& type)
	{
		if (type == "vertex")
			return GL_VERTEX_SHADER;
		if (type == "fragment" || type == "pixel")
			return GL_FRAGMENT_SHADER;
		if (type == "compute")
			return GL_COMPUTE_SHADER;

		return GL_NONE;
	}

	size_t OpenGLShaderH2M::GetHash() const
	{
		return std::hash<std::string>{}(m_AssetPath);
	}

	void OpenGLShaderH2M::SetUniformBuffer(const std::string& name, const void* data, uint32_t size)
	{
		ShaderUniformBufferH2M* uniformBuffer = nullptr;
		for (auto& [bindingPoint, ub] : s_UniformBuffers)
		{
			if (ub.Name == name)
			{
				uniformBuffer = &ub;
				break;
			}
		}

		H2M_CORE_ASSERT(uniformBuffer);
		H2M_CORE_ASSERT(uniformBuffer->Size >= size);
		glNamedBufferSubData(uniformBuffer->RendererID, 0, size, data);
	}

	void OpenGLShaderH2M::SetUniform(const std::string& fullname, float value)
	{
		RendererH2M::Submit([=]()
		{
			H2M_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
			GLint location = m_UniformLocations.at(fullname);
			glUniform1f(location, value);
		});
	}

	void OpenGLShaderH2M::SetUniform(const std::string& fullname, uint32_t value)
	{
		RendererH2M::Submit([=]()
		{
			H2M_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
			GLint location = m_UniformLocations.at(fullname);
			glUniform1ui(location, value);
		});
	}

	void OpenGLShaderH2M::SetUniform(const std::string& fullname, int value)
	{
		RendererH2M::Submit([=]()
		{
			H2M_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
			GLint location = m_UniformLocations.at(fullname);
			glUniform1i(location, value);
		});
	}

	void OpenGLShaderH2M::SetUniform(const std::string& fullname, const glm::vec2& value)
	{
		RendererH2M::Submit([=]()
		{
			H2M_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
			GLint location = m_UniformLocations.at(fullname);
			glUniform2fv(location, 1, glm::value_ptr(value));
		});
	}

	void OpenGLShaderH2M::SetUniform(const std::string& fullname, const glm::vec3& value)
	{
		RendererH2M::Submit([=]()
		{
			H2M_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
			GLint location = m_UniformLocations.at(fullname);
			glUniform3fv(location, 1, glm::value_ptr(value));
		});
	}

	void OpenGLShaderH2M::SetUniform(const std::string& fullname, const glm::vec4& value)
	{
		RendererH2M::Submit([=]()
		{
			H2M_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
			GLint location = m_UniformLocations.at(fullname);
			glUniform4fv(location, 1, glm::value_ptr(value));
		});
	}

	void OpenGLShaderH2M::SetUniform(const std::string& fullname, const glm::mat3& value)
	{
		RendererH2M::Submit([=]()
		{
			H2M_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
			GLint location = m_UniformLocations.at(fullname);
			glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
		});
	}

	void OpenGLShaderH2M::SetUniform(const std::string& fullname, const glm::mat4& value)
	{
		RendererH2M::Submit([=]()
		{
			H2M_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
			GLint location = m_UniformLocations.at(fullname);
			glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
		});
	}

	void OpenGLShaderH2M::SetUInt(const std::string& name, uint32_t value)
	{
		RendererH2M::Submit([=]() {
			UploadUniformUInt(name, value);
		});
	}

	void OpenGLShaderH2M::SetFloat(const std::string& name, float value)
	{
		RendererH2M::Submit([=]() {
			UploadUniformFloat(name, value);
		});
	}

	void OpenGLShaderH2M::SetInt(const std::string& name, int value)
	{
		RendererH2M::Submit([=]() {
			UploadUniformInt(name, value);
		});
	}

	void OpenGLShaderH2M::SetFloat2(const std::string& name, const glm::vec2& value)
	{
		RendererH2M::Submit([=]() {
			UploadUniformFloat2(name, value);
		});
	}

	void OpenGLShaderH2M::SetFloat3(const std::string& name, const glm::vec3& value)
	{
		RendererH2M::Submit([=]() {
			UploadUniformFloat3(name, value);
		});
	}

	void OpenGLShaderH2M::SetFloat4(const std::string& name, const glm::vec4& value)
	{
		RendererH2M::Submit([=]() {
			UploadUniformFloat4(name, value);
		});
	}

	void OpenGLShaderH2M::SetMat4(const std::string& name, const glm::mat4& value)
	{
		RendererH2M::Submit([=]() {
			UploadUniformMat4(name, value);
		});
	}

	void OpenGLShaderH2M::SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind)
	{
		if (bind)
		{
			UploadUniformMat4(name, value);
		}
		else
		{
			int location = glGetUniformLocation(m_RendererID, name.c_str());
			if (location != -1)
				UploadUniformMat4(location, value);
		}
	}

	void OpenGLShaderH2M::SetIntArray(const std::string& name, int* values, uint32_t size)
	{
		RendererH2M::Submit([=]() {
			UploadUniformIntArray(name, values, size);
		});
	}

	const ShaderResourceDeclarationH2M* OpenGLShaderH2M::GetShaderResource(const std::string& name)
	{
		if (m_Resources.find(name) == m_Resources.end())
			return nullptr;

		return &m_Resources.at(name);
	}

	void OpenGLShaderH2M::UploadUniformInt(uint32_t location, int32_t value)
	{
		glUniform1i(location, value);
	}


	void OpenGLShaderH2M::UploadUniformIntArray(uint32_t location, int32_t* values, int32_t count)
	{
		glUniform1iv(location, count, values);
	}

	void OpenGLShaderH2M::UploadUniformFloat(uint32_t location, float value)
	{
		glUniform1f(location, value);
	}

	void OpenGLShaderH2M::UploadUniformFloat2(uint32_t location, const glm::vec2& value)
	{
		glUniform2f(location, value.x, value.y);
	}

	void OpenGLShaderH2M::UploadUniformFloat3(uint32_t location, const glm::vec3& value)
	{
		glUniform3f(location, value.x, value.y, value.z);
	}

	void OpenGLShaderH2M::UploadUniformFloat4(uint32_t location, const glm::vec4& value)
	{
		glUniform4f(location, value.x, value.y, value.z, value.w);
	}

	void OpenGLShaderH2M::UploadUniformMat3(uint32_t location, const glm::mat3& value)
	{
		glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
	}

	void OpenGLShaderH2M::UploadUniformMat4(uint32_t location, const glm::mat4& value)
	{
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
	}

	void OpenGLShaderH2M::UploadUniformMat4Array(uint32_t location, const glm::mat4& values, uint32_t count)
	{
		glUniformMatrix4fv(location, count, GL_FALSE, glm::value_ptr(values));
	}

	void OpenGLShaderH2M::UploadUniformInt(const std::string& name, int32_t value)
	{
		int32_t location = GetUniformLocation(name);
		glUniform1i(location, value);
	}

	void OpenGLShaderH2M::UploadUniformUInt(const std::string& name, uint32_t value)
	{
		int32_t location = GetUniformLocation(name);
		glUniform1ui(location, value);
	}

	void OpenGLShaderH2M::UploadUniformIntArray(const std::string& name, int32_t* values, uint32_t count)
	{
		int32_t location = GetUniformLocation(name);
		glUniform1iv(location, count, values);
	}

	void OpenGLShaderH2M::UploadUniformFloat(const std::string& name, float value)
	{
		glUseProgram(m_RendererID);
		auto location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location != -1) {
			glUniform1f(location, value);
		}
		else {
			HZ_LOG_UNIFORM("OpenGLShaderH2M::UploadUniformFloat - uniform '{0}' not found!", name);
		}
	}

	void OpenGLShaderH2M::UploadUniformFloat2(const std::string& name, const glm::vec2& values)
	{
		glUseProgram(m_RendererID);
		auto location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location != -1) {
			glUniform2f(location, values.x, values.y);
		}
		else {
			HZ_LOG_UNIFORM("OpenGLShaderH2M::UploadUniformFloat2 - uniform '{0}' not found!", name);
		}
	}

	void OpenGLShaderH2M::UploadUniformFloat3(const std::string& name, const glm::vec3& values)
	{
		glUseProgram(m_RendererID);
		auto location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location != -1) {
			glUniform3f(location, values.x, values.y, values.z);
		}
		else {
			HZ_LOG_UNIFORM("OpenGLShaderH2M::UploadUniformFloat3 - uniform '{0}' not found!", name);
		}
	}

	void OpenGLShaderH2M::UploadUniformFloat4(const std::string& name, const glm::vec4& values)
	{
		glUseProgram(m_RendererID);
		auto location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location != -1) {
			glUniform4f(location, values.x, values.y, values.z, values.w);
		}
		else {
			HZ_LOG_UNIFORM("OpenGLShaderH2M::UploadUniformFloat4 - uniform '{0}' not found!", name);
		}
	}

	void OpenGLShaderH2M::UploadUniformMat4(const std::string& name, const glm::mat4& values)
	{
		glUseProgram(m_RendererID);
		auto location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location != -1) {
			glUniformMatrix4fv(location, 1, GL_FALSE, (const float*)&values);
		}
		else {
			HZ_LOG_UNIFORM("OpenGLShaderH2M::UploadUniformMat4 - uniform '{0}' not found!", name);
		}
	}
}
