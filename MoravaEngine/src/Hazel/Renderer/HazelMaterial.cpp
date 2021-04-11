#include "HazelMaterial.h"

#include "Hazel/Platform/Vulkan/VulkanMaterial.h"
#include "Hazel/Platform/OpenGL/OpenGLMaterial.h"

#include "Hazel/Renderer/RendererAPI.h"


namespace Hazel {

	HazelMaterial::HazelMaterial()
	{
	}

	HazelMaterial::HazelMaterial(const Ref<HazelShader>& shader, const std::string& name)
	{
		// Create(shader, name);
	}

	Ref<HazelMaterial> HazelMaterial::Create(const Ref<HazelShader>& shader, const std::string& name)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return Ref<HazelMaterial>();
		case RendererAPIType::Vulkan: return Ref<VulkanMaterial>::Create(shader, name);
		case RendererAPIType::OpenGL: return Ref<OpenGLMaterial>::Create(shader, name);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<HazelMaterial>();
	}

}
