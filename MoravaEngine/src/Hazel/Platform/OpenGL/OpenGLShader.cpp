#include "OpenGLShader.h"
#include "../../Core/Assert.h"
#include "../../Renderer/HazelRenderer.h"
#include "../../Core/HazelLog.h"
#include "../../Renderer/ShaderUniform.h"

#include <string>
#include <sstream>
#include <limits>
#include <fstream>
#include <filesystem>

#include <glm/gtc/type_ptr.hpp>

#include <shaderc/shaderc.hpp>


namespace Hazel {

#define UNIFORM_LOGGING 0
#if UNIFORM_LOGGING
#define HZ_LOG_UNIFORM(...) HZ_CORE_WARN(__VA_ARGS__)
#else
#define HZ_LOG_UNIFORM
#endif

#define PRINT_SHADERS 1

	OpenGLShader::OpenGLShader(const std::string& filepath, bool forceRecompile)
		: m_AssetPath(filepath)
	{
		size_t found = filepath.find_last_of("/\\");
		m_Name = found != std::string::npos ? filepath.substr(found + 1) : filepath;
		found = m_Name.find_last_of(".");
		m_Name = found != std::string::npos ? m_Name.substr(0, found) : m_Name;

		Reload(forceRecompile);
	}

	Ref<OpenGLShader> OpenGLShader::CreateFromString(const std::string& source)
	{
		Ref<OpenGLShader> shader = Ref<OpenGLShader>::Create();
		shader->Load(source, true);
		return shader;
	}

	void OpenGLShader::Reload(bool forceCompile)
	{
		std::string source = ReadShaderFromFile(m_AssetPath);
		Load(source, forceCompile);
	}

	void OpenGLShader::Load(const std::string& source, bool forceCompile)
	{
		m_ShaderSource = PreProcess(source);

		Ref<OpenGLShader> instance = this;
		HazelRenderer::Submit([instance, forceCompile]() mutable
		{
		});

		std::array<std::vector<uint32_t>, 2> vulkanBinaries;
		std::unordered_map<uint32_t, std::vector<uint32_t>> shaderData;
		this->CompileOrGetVulkanBinary(shaderData, forceCompile);
		this->CompileOrGetOpenGLBinary(shaderData, forceCompile);
	}

	static const char* GLShaderStageCachedVulkanFileExtension(uint32_t stage)
	{
		switch (stage)
		{
		case GL_VERTEX_SHADER:    return ".cached_vulkan.vert";
		case GL_FRAGMENT_SHADER:  return ".cached_vulkan.frag";
		case GL_COMPUTE_SHADER:   return ".cached_vulkan.comp";
		}
		HZ_CORE_ASSERT(false);
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
		HZ_CORE_ASSERT(false);
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
		HZ_CORE_ASSERT(false);
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
		HZ_CORE_ASSERT(false);
		return "";
	}

	void OpenGLShader::CompileOrGetVulkanBinary(std::unordered_map<uint32_t, std::vector<uint32_t>>& outputBinary, bool forceCompile)
	{
		for (auto [stage, source] : m_ShaderSource)
		{
			auto extension = GLShaderStageCachedVulkanFileExtension(stage);
			if (!forceCompile)
			{
				std::filesystem::path p = m_AssetPath;
				auto path = p.parent_path() / "cached" / (p.filename().string() + extension);
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

				const bool optimize = false;
				if (optimize)
					options.SetOptimizationLevel(shaderc_optimization_level_performance);

				// Compile shader
				{
					auto& shaderSource = m_ShaderSource.at(stage);
					shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(shaderSource, GLShaderStageToShaderC(stage), m_AssetPath.c_str(), options);

					if (module.GetCompilationStatus() != shaderc_compilation_status_success)
					{
						HZ_CORE_ERROR(module.GetErrorMessage());
						HZ_CORE_ASSERT(false);
					}

					const uint8_t* begin = (const uint8_t*)module.cbegin();
					const uint8_t* end = (const uint8_t*)module.cend();
					const ptrdiff_t size = end - begin;

					outputBinary[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());
				}

				// Cache compiled shader
				{
					std::filesystem::path p = m_AssetPath;
					auto path = p.parent_path() / "cached" / (p.filename().string() + extension);
					std::string cachedFilePath = path.string();

					FILE* f = fopen(cachedFilePath.c_str(), "wb");
					fwrite(outputBinary[stage].data(), sizeof(uint32_t), outputBinary[stage].size(), f);
					fclose(f);
				}
			}
		}
	}

	void OpenGLShader::CompileOrGetOpenGLBinary(const std::unordered_map<uint32_t, std::vector<uint32_t>>& vulkanBinaries, bool forceCompile)
	{
		if (m_RendererID)
			glDeleteProgram(m_RendererID);

		GLuint program = glCreateProgram();
		m_RendererID = program;

		std::vector<GLuint> shaderRendererIDs;
		shaderRendererIDs.reserve(vulkanBinaries.size());

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
				auto path = p.parent_path() / "cached" / (p.filename().string() + GLShaderStageCachedOpenGLFileExtension(stage));
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
						HZ_CORE_ERROR(module.GetErrorMessage());
						HZ_CORE_ASSERT(false);
					}

					shaderStageData = std::vector<uint32_t>(module.cbegin(), module.cend());

					{
						std::filesystem::path p = m_AssetPath;
						auto path = p.parent_path() / "cached" / (p.filename().string() + GLShaderStageCachedOpenGLFileExtension(stage));
						std::string cachedFilePath = path.string();
						FILE* f = fopen(cachedFilePath.c_str(), "wb");
						fwrite(shaderStageData.data(), sizeof(uint32_t), shaderStageData.size(), f);
						fclose(f);
					}
				}

				GLuint shaderID = glCreateShader(stage);
				glShaderBinary(1, &shaderID, GL_SHADER_BINARY_FORMAT_SPIR_V, shaderStageData.data(), (GLsizei)shaderStageData.size() * sizeof(uint32_t));
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
			HZ_CORE_ERROR("Shader compilation failed ({0}):\n{1}", m_AssetPath, &infoLog[0]);

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
		for (auto& [name, buffer] : m_Buffers)
		{
			for (auto& [name, uniform] : buffer.Uniforms)
			{
				const auto& name = uniform.GetName();
				GLint location = glGetUniformLocation(m_RendererID, name.c_str());
				if (location == -1)
					HZ_CORE_WARN("{0}: could not find uniform location {0}", name);

				m_UniformLocations[name] = location;
			}
		}

		for (auto& shaderStageData : shaderData)
			Reflect(shaderStageData);
	}

	static ShaderUniformType SPIRTypeToShaderUniformType(spirv_cross::SPIRType type)
	{
		switch (type.basetype)
		{
		case spirv_cross::SPIRType::Boolean:  return ShaderUniformType::Bool;
		case spirv_cross::SPIRType::Int:      return ShaderUniformType::Int;
		case spirv_cross::SPIRType::Float:
			if (type.vecsize == 1)            return ShaderUniformType::Float;
			if (type.vecsize == 2)            return ShaderUniformType::Vec2;
			if (type.vecsize == 3)            return ShaderUniformType::Vec3;
			if (type.vecsize == 4)            return ShaderUniformType::Vec4;

			if (type.columns == 3)            return ShaderUniformType::Mat3;
			if (type.columns == 4)            return ShaderUniformType::Mat4;
			break;
		}
		HZ_CORE_ASSERT(false, "Unknown type!");
		return ShaderUniformType::None;
	}

	void OpenGLShader::Compile(const std::vector<uint32_t>& vertexBinary, const std::vector<uint32_t>& fragmentBinary)
	{

	}

	void OpenGLShader::ParseConstantBuffers(const spirv_cross::CompilerGLSL& compiler)
	{
		spirv_cross::ShaderResources res = compiler.get_shader_resources();
		for (const spirv_cross::Resource& resource : res.push_constant_buffers)
		{
			auto& bufferType = compiler.get_type(resource.base_type_id);
			auto location = compiler.get_decoration(resource.id, spv::DecorationLocation);
			const auto& bufferName = resource.name;
			auto bufferSize = compiler.get_declared_struct_size(bufferType);
			int memberCount = (int)bufferType.member_types.size();
			ShaderBuffer& buffer = m_Buffers[bufferName];
			buffer.Name = bufferName;
			buffer.Size = (uint32_t)bufferSize;
			for (int i = 0; i < memberCount; i++)
			{
				auto type = compiler.get_type(bufferType.member_types[i]);
				const auto& memberName = compiler.get_member_name(bufferType.self, i);
				auto size = compiler.get_declared_struct_member_size(bufferType, i);
				auto offset = compiler.type_struct_member_offset(bufferType, i);

				std::string uniformName = bufferName + "." + memberName;
				buffer.Uniforms[uniformName] = ShaderUniform(uniformName, SPIRTypeToShaderUniformType(type), (uint32_t)size, offset);
			}

		}
	}

	void OpenGLShader::Reflect(std::vector<uint32_t>& data)
	{
		spirv_cross::Compiler comp(data);
		spirv_cross::ShaderResources res = comp.get_shader_resources();

		Log::GetLogger()->trace("OpenGLShader::Reflect - {0}", m_AssetPath);
		Log::GetLogger()->trace("   {0} Uniform Buffers", res.uniform_buffers.size());
		Log::GetLogger()->trace("   {0} Resources", res.sampled_images.size());

		glUseProgram(m_RendererID);

		uint32_t bufferIndex = 0;
		for (const spirv_cross::Resource& resource : res.uniform_buffers)
		{
			auto& bufferType = comp.get_type(resource.base_type_id);
			int memberCount = (int)bufferType.member_types.size();
			uint32_t bindingPoint = comp.get_decoration(resource.id, spv::DecorationBinding);

			if (s_UniformBuffers.find(bindingPoint) == s_UniformBuffers.end())
			{
				ShaderUniformBuffer& buffer = s_UniformBuffers[bindingPoint];
				buffer.Name = resource.name;
				buffer.BindingPoint = bindingPoint;
				buffer.Size = (uint32_t)comp.get_declared_struct_size(bufferType);

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

				Log::GetLogger()->trace("Created Uniform Buffer at binding point {0} with name '{1}', size is {2} bytes", buffer.BindingPoint, buffer.Name, buffer.Size);

				glBindBuffer(GL_UNIFORM_BUFFER, 0);
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
			HZ_CORE_ASSERT(location != -1);
			m_Resources[name] = ShaderResourceDeclaration(name, binding, 1);
			glUniform1i(location, binding);
		}
	}

	void OpenGLShader::AddShaderReloadedCallback(const ShaderReloadedCallback& callback)
	{
		m_ShaderReloadedCallbacks.push_back(callback);
	}

	void OpenGLShader::Bind()
	{
		HazelRenderer::Submit([=]() {
		});

		glUseProgram(m_RendererID);
	}

	std::string OpenGLShader::ReadShaderFromFile(const std::string& filepath) const
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

	std::unordered_map<GLenum, std::string> OpenGLShader::PreProcess(const std::string& source)
	{
		std::unordered_map<GLenum, std::string> shaderSources;

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

	std::vector<std::string> SplitString(const std::string& string, const std::string& delimiters)
	{
		size_t start = 0;
		size_t end = string.find_first_of(delimiters);

		std::vector<std::string> result;

		while (end <= std::string::npos)
		{
			std::string token = string.substr(start, end - start);
			if (!token.empty())
				result.push_back(token);

			if (end == std::string::npos)
				break;

			start = end + 1;
			end = string.find_first_of(delimiters, start);
		}

		return result;
	}

	std::vector<std::string> SplitString(const std::string& string, const char delimiter)
	{
		return SplitString(string, std::string(1, delimiter));
	}

	std::vector<std::string> Tokenize(const std::string& string)
	{
		return SplitString(string, " \t\n");
	}

	std::vector<std::string> GetLines(const std::string& string)
	{
		return SplitString(string, "\n");
	}

	std::string GetBlock(const char* str, const char** outPosition)
	{
		const char* end = strstr(str, "}");
		if (!end)
			return str;

		if (outPosition)
			*outPosition = end;
		uint32_t length = (uint32_t)(end - str + 1);
		return std::string(str, length);
	}

	std::string GetStatement(const char* str, const char** outPosition)
	{
		const char* end = strstr(str, ";");
		if (!end)
			return str;

		if (outPosition)
			*outPosition = end;
		uint32_t length = (uint32_t)(end - str + 1);
		return std::string(str, length);
	}

	bool StartsWith(const std::string& string, const std::string& start)
	{
		return string.find(start) == 0;
	}

	static bool IsTypeStringResource(const std::string& type)
	{
		if (type == "sampler2D")		return true;
		if (type == "sampler2DMS")		return true;
		if (type == "samplerCube")		return true;
		if (type == "sampler2DShadow")	return true;
		return false;
	}

	int32_t OpenGLShader::GetUniformLocation(const std::string& name) const
	{
		int32_t result = glGetUniformLocation(m_RendererID, name.c_str());
		if (result == -1)
			HZ_CORE_WARN("Could not find uniform '{0}' in shader", name);

		return result;
	}

	GLenum OpenGLShader::ShaderTypeFromString(const std::string& type)
	{
		if (type == "vertex")
			return GL_VERTEX_SHADER;
		if (type == "fragment" || type == "pixel")
			return GL_FRAGMENT_SHADER;
		if (type == "compute")
			return GL_COMPUTE_SHADER;

		return GL_NONE;
	}

	void OpenGLShader::CompileAndUploadShader()
	{
		std::vector<GLuint> shaderRendererIDs;

		GLuint program = glCreateProgram();
		for (auto& kv : m_ShaderSource)
		{
			GLenum type = kv.first;
			std::string& source = kv.second;

			GLuint shaderRendererID = glCreateShader(type);
			const GLchar* sourceCstr = (const GLchar*)source.c_str();
			glShaderSource(shaderRendererID, 1, &sourceCstr, 0);

			glCompileShader(shaderRendererID);

			GLint isCompiled = 0;
			glGetShaderiv(shaderRendererID, GL_COMPILE_STATUS, &isCompiled);
			if (isCompiled == GL_FALSE)
			{
				GLint maxLength = 0;
				glGetShaderiv(shaderRendererID, GL_INFO_LOG_LENGTH, &maxLength);

				// The maxLength includes the NULL character
				std::vector<GLchar> infoLog(maxLength);
				glGetShaderInfoLog(shaderRendererID, maxLength, &maxLength, &infoLog[0]);

				// HZ_CORE_ERROR("Shader compilation failed:\n{0}", &infoLog[0]);
				Log::GetLogger()->critical("Shader compilation failed:\n{0}", &infoLog[0]);

				// We don't need the shader anymore.
				glDeleteShader(shaderRendererID);

				HZ_CORE_ASSERT(false, "Failed");
			}

			shaderRendererIDs.push_back(shaderRendererID);
			glAttachShader(program, shaderRendererID);
		}

		// Link our program
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
			// HZ_CORE_ERROR("Shader compilation failed:\n{0}", &infoLog[0]);
			Log::GetLogger()->critical("Shader compilation failed:\n{0}", &infoLog[0]);

			// We don't need the program anymore.
			glDeleteProgram(program);
			// Don't leak shaders either.
			for (auto id : shaderRendererIDs)
				glDeleteShader(id);
		}

		// Always detach shaders after a successful link.
		for (auto id : shaderRendererIDs)
			glDetachShader(program, id);

		m_RendererID = program;
	}

	size_t OpenGLShader::GetHash() const
	{
		return std::hash<std::string>{}(m_AssetPath);
	}

	void OpenGLShader::SetUniformBuffer(const std::string& name, const void* data, uint32_t size)
	{
		uint8_t* buffer = new uint8_t[size];
		memcpy(buffer, data, size);

		Ref<OpenGLShader> instance = this;
		HazelRenderer::Submit([=]()
		{
		});

		{
			ShaderUniformBuffer* uniformBuffer = nullptr;
			for (auto& [bindingPoint, ub] : s_UniformBuffers)
			{
				if (ub.Name == name)
				{
					uniformBuffer = &ub;
					break;
				}
			}

			HZ_CORE_ASSERT(uniformBuffer);
			HZ_CORE_ASSERT(uniformBuffer->Size >= size);
			glNamedBufferSubData(uniformBuffer->RendererID, 0, size, buffer);

			delete[] buffer;
		}
	}

	void OpenGLShader::SetUniform(const std::string& fullname, float value)
	{
		HazelRenderer::Submit([=]()
		{
		});

		{
			HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
			GLint location = m_UniformLocations.at(fullname);
			glUniform1f(location, value);
		}
	}

	void OpenGLShader::SetUniform(const std::string& fullname, int value)
	{
		HazelRenderer::Submit([=]()
		{
		});

		{
			HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
			GLint location = m_UniformLocations.at(fullname);
			glUniform1i(location, value);
		}
	}

	void OpenGLShader::SetUniform(const std::string& fullname, const glm::vec2& value)
	{
		HazelRenderer::Submit([=]()
		{
		});

		{
			HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
			GLint location = m_UniformLocations.at(fullname);
			glUniform2fv(location, 1, glm::value_ptr(value));
		}
	}

	void OpenGLShader::SetUniform(const std::string& fullname, const glm::vec3& value)
	{
		HazelRenderer::Submit([=]()
		{
		});

		{
			HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
			GLint location = m_UniformLocations.at(fullname);
			glUniform3fv(location, 1, glm::value_ptr(value));
		}
	}

	void OpenGLShader::SetUniform(const std::string& fullname, const glm::vec4& value)
	{
		HazelRenderer::Submit([=]()
		{
		});

		{
			HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
			GLint location = m_UniformLocations.at(fullname);
			glUniform4fv(location, 1, glm::value_ptr(value));
		}
	}

	void OpenGLShader::SetUniform(const std::string& fullname, const glm::mat3& value)
	{
		HazelRenderer::Submit([=]()
		{
		});

		{
			HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
			GLint location = m_UniformLocations.at(fullname);
			glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
		}
	}

	void OpenGLShader::SetUniform(const std::string& fullname, const glm::mat4& value)
	{
		HazelRenderer::Submit([=]()
		{
		});

		{
			HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
			GLint location = m_UniformLocations.at(fullname);
			glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
		}
	}

	void OpenGLShader::SetFloat(const std::string& name, float value)
	{
		HazelRenderer::Submit([=]() {
		});

		{
			UploadUniformFloat(name, value);
		}
	}

	void OpenGLShader::SetInt(const std::string& name, int value)
	{
		HazelRenderer::Submit([=]() {
		});

		{
			UploadUniformInt(name, value);
		}
	}

	void OpenGLShader::SetBool(const std::string& name, bool value)
	{
		HazelRenderer::Submit([=]() {
		});

		{
			UploadUniformInt(name, value);
		}
	}

	void OpenGLShader::SetFloat3(const std::string& name, const glm::vec3& value)
	{
		HazelRenderer::Submit([=]() {
		});

		{
			UploadUniformFloat3(name, value);
		}
	}

	void OpenGLShader::SetMat4(const std::string& name, const glm::mat4& value)
	{
		HazelRenderer::Submit([=]() {
		});

		{
			UploadUniformMat4(name, value);
		}
	}

	void OpenGLShader::SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind)
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

	void OpenGLShader::SetIntArray(const std::string& name, int* values, uint32_t size)
	{
		HazelRenderer::Submit([=]() {
		});

		{
			UploadUniformIntArray(name, values, size);
		}
	}

	void OpenGLShader::UploadUniformInt(uint32_t location, int32_t value)
	{
		glUniform1i(location, value);
	}

	void OpenGLShader::UploadUniformIntArray(uint32_t location, int32_t* values, int32_t count)
	{
		glUniform1iv(location, count, values);
	}

	void OpenGLShader::UploadUniformFloat(uint32_t location, float value)
	{
		glUniform1f(location, value);
	}

	void OpenGLShader::UploadUniformFloat2(uint32_t location, const glm::vec2& value)
	{
		glUniform2f(location, value.x, value.y);
	}

	void OpenGLShader::UploadUniformFloat3(uint32_t location, const glm::vec3& value)
	{
		glUniform3f(location, value.x, value.y, value.z);
	}

	void OpenGLShader::UploadUniformFloat4(uint32_t location, const glm::vec4& value)
	{
		glUniform4f(location, value.x, value.y, value.z, value.w);
	}

	void OpenGLShader::UploadUniformMat3(uint32_t location, const glm::mat3& value)
	{
		glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
	}

	void OpenGLShader::UploadUniformMat4(uint32_t location, const glm::mat4& value)
	{
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
	}

	void OpenGLShader::UploadUniformMat4Array(uint32_t location, const glm::mat4& values, uint32_t count)
	{
		glUniformMatrix4fv(location, count, GL_FALSE, glm::value_ptr(values));
	}

	void OpenGLShader::UploadUniformInt(const std::string& name, int32_t value)
	{
		int32_t location = GetUniformLocation(name);
		glUniform1i(location, value);
	}

	void OpenGLShader::UploadUniformIntArray(const std::string& name, int32_t* values, uint32_t count)
	{
		int32_t location = GetUniformLocation(name);
		glUniform1iv(location, count, values);
	}

	void OpenGLShader::UploadUniformFloat(const std::string& name, float value)
	{
		glUseProgram(m_RendererID);
		auto location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location != -1)
			glUniform1f(location, value);
		else
			HZ_LOG_UNIFORM("Uniform '{0}' not found!", name);
	}

	void OpenGLShader::UploadUniformFloat2(const std::string& name, const glm::vec2& values)
	{
		glUseProgram(m_RendererID);
		auto location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location != -1)
			glUniform2f(location, values.x, values.y);
		else
			HZ_LOG_UNIFORM("Uniform '{0}' not found!", name);
	}

	void OpenGLShader::UploadUniformFloat3(const std::string& name, const glm::vec3& values)
	{
		glUseProgram(m_RendererID);
		auto location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location != -1)
			glUniform3f(location, values.x, values.y, values.z);
		else
			HZ_LOG_UNIFORM("Uniform '{0}' not found!", name);
	}

	void OpenGLShader::UploadUniformFloat4(const std::string& name, const glm::vec4& values)
	{
		glUseProgram(m_RendererID);
		auto location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location != -1)
			glUniform4f(location, values.x, values.y, values.z, values.w);
		else
			HZ_LOG_UNIFORM("Uniform '{0}' not found!", name);
	}

	void OpenGLShader::UploadUniformMat4(const std::string& name, const glm::mat4& values)
	{
		glUseProgram(m_RendererID);
		auto location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location != -1)
			glUniformMatrix4fv(location, 1, GL_FALSE, (const float*)&values);
		else
			HZ_LOG_UNIFORM("Uniform '{0}' not found!", name);
	}

}
