#include "ShaderCompile.h"

#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"

#include <shaderc/shaderc.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>


namespace ShaderCompile
{
	static bool ReadTextFile(const std::filesystem::path& path, std::string& text)
	{
		std::ifstream in(path, std::ios::in | std::ios::binary);
		if (!in)
		{
			return false;
		}
		std::stringstream buffer;
		buffer << in.rdbuf();
		text = buffer.str();
		return true;
	}

	// #include "file" in shaders: the path is relative to the file that includes it (e.g. "Include/PBR_FrameSet.glslh")
	class Includer : public shaderc::CompileOptions::IncluderInterface
	{
		struct IncludeData
		{
			std::string Name;
			std::string Content;
		};

	public:
		shaderc_include_result* GetInclude(const char* requestedSource, shaderc_include_type, const char* requestingSource, size_t) override
		{
			std::filesystem::path path = std::filesystem::path(requestingSource).parent_path() / requestedSource;
			IncludeData* data = new IncludeData();
			if (ReadTextFile(path, data->Content))
			{
				data->Name = path.lexically_normal().generic_string();
			}
			else
			{
				data->Content = "cannot open include file '" + path.generic_string() + "'"; // an empty name reports the content as the error
			}
			return new shaderc_include_result{ data->Name.c_str(), data->Name.size(), data->Content.c_str(), data->Content.size(), data };
		}

		void ReleaseInclude(shaderc_include_result* result) override
		{
			delete static_cast<IncludeData*>(result->user_data);
			delete result;
		}
	};

	static shaderc_shader_kind GetShaderKind(VkShaderStageFlagBits stage)
	{
		switch (stage)
		{
			case VK_SHADER_STAGE_VERTEX_BIT:   return shaderc_vertex_shader;
			case VK_SHADER_STAGE_FRAGMENT_BIT: return shaderc_fragment_shader;
			default:                           return shaderc_compute_shader;
		}
	}

	static bool Compile(const std::string& source, VkShaderStageFlagBits stage, const std::string& path, std::vector<uint32_t>& spirv, std::string& error)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		options.SetIncluder(std::make_unique<Includer>());

		const shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, GetShaderKind(stage), path.c_str(), options);
		if (module.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			error = module.GetErrorMessage();
			return false;
		}
		spirv.assign(module.cbegin(), module.cend());
		return true;
	}

	void Install()
	{
		H2M::VulkanShaderH2M::SetCompiler(&Compile);
	}
}
