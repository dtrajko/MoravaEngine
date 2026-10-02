#include "EnvMapVulkanMaterialLibrary.h"

#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include "Core/Log.h"

#include <algorithm>
#include <filesystem>


std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> EnvMapVulkanMaterialLibrary::s_Materials;
H2M::RefH2M<EnvMapVulkanMaterial> EnvMapVulkanMaterialLibrary::s_DefaultMaterial;

static const char* s_MapTextureNames[EnvMapVulkanMaterial::MapCount] = {
	"u_AlbedoTexture", "u_NormalTexture", "u_MetalnessTexture", "u_RoughnessTexture", "u_EmissiveTexture", "u_AOTexture" };

static const char* s_MapToggleNames[EnvMapVulkanMaterial::MapCount] = {
	"u_MaterialUniforms.AlbedoTexToggle",
	"u_MaterialUniforms.NormalTexToggle",
	"u_MaterialUniforms.MetalnessTexToggle",
	"u_MaterialUniforms.RoughnessTexToggle",
	"u_MaterialUniforms.EmissiveTexToggle",
	"u_MaterialUniforms.AOTexToggle",
};

// The material values (the Material push constant block of the mesh shaders), copied by name on import and duplicate
static const char* s_FloatValueNames[] = {
	"u_MaterialUniforms.Metalness",
	"u_MaterialUniforms.Roughness",
	"u_MaterialUniforms.RadiancePrefilter",
	"u_MaterialUniforms.AlbedoTexToggle",
	"u_MaterialUniforms.NormalTexToggle",
	"u_MaterialUniforms.MetalnessTexToggle",
	"u_MaterialUniforms.RoughnessTexToggle",
	"u_MaterialUniforms.TilingFactor",
	"u_MaterialUniforms.EmissiveTexToggle",
	"u_MaterialUniforms.AOTexToggle",
	"u_MaterialUniforms.EmissiveIntensity",
	"u_MaterialUniforms.MetalRoughPacked",
};

// Library materials are created with the shared HazelPBR_Static shader (shader library), so their descriptor sets don't depend
// on any mesh's own shader instance. Set 1 is declared identically in HazelPBR_Anim, so skinned meshes can bind them too.
static H2M::RefH2M<H2M::VulkanShaderH2M> GetMaterialShader()
{
	return H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static").As<H2M::VulkanShaderH2M>();
}

const char* EnvMapVulkanMaterial::GetMapTextureName(uint32_t slot)
{
	return slot < MapCount ? s_MapTextureNames[slot] : "";
}

const char* EnvMapVulkanMaterial::GetMapToggleName(uint32_t slot)
{
	return slot < MapCount ? s_MapToggleNames[slot] : "";
}

EnvMapVulkanMaterial::EnvMapVulkanMaterial(const std::string& name)
	: m_Name(name)
{
	H2M::RefH2M<H2M::ShaderH2M> shader = GetMaterialShader().As<H2M::ShaderH2M>();
	m_Values = H2M::RefH2M<H2M::VulkanMaterialH2M>::Create(shader, name);

	// Defaults (the storage starts zeroed: no maps, metalness 0)
	Get<glm::vec3>("u_MaterialUniforms.AlbedoColor") = glm::vec3(0.8f);
	Get<float>("u_MaterialUniforms.Roughness") = 0.5f;
	Get<float>("u_MaterialUniforms.TilingFactor") = 1.0f;
	Get<float>("u_MaterialUniforms.EmissiveIntensity") = 1.0f;

	// Every binding of the set must be written before the set is used
	for (uint32_t slot = 0; slot < MapCount; slot++)
	{
		WriteMapDescriptor(slot, H2M::RendererH2M::GetWhiteTexture());
	}
}

VkDescriptorSet EnvMapVulkanMaterial::GetDescriptorSet()
{
	const auto& descriptorSets = m_Values->GetDescriptorSet().DescriptorSets;
	return descriptorSets.empty() ? VK_NULL_HANDLE : descriptorSets[0];
}

void EnvMapVulkanMaterial::WriteMapDescriptor(uint32_t slot, H2M::RefH2M<H2M::Texture2D_H2M> texture)
{
	VkDescriptorSet descriptorSet = GetDescriptorSet();
	const VkWriteDescriptorSet* binding = GetMaterialShader()->GetDescriptorSet(s_MapTextureNames[slot], H2M::VulkanShaderH2M::MaterialDescriptorSet);
	if (descriptorSet == VK_NULL_HANDLE || !binding)
	{
		Log::GetLogger()->error("Material '{0}': '{1}' could not be written (no material descriptor set or binding).", m_Name, s_MapTextureNames[slot]);
		return;
	}

	VkWriteDescriptorSet write = *binding;
	write.dstSet = descriptorSet;
	write.descriptorCount = 1;
	write.pImageInfo = &texture.As<H2M::VulkanTexture2D_H2M>()->GetVulkanDescriptorInfo();

	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

void EnvMapVulkanMaterial::SetMap(uint32_t slot, H2M::RefH2M<H2M::Texture2D_H2M> texture)
{
	if (slot >= MapCount || !texture)
	{
		return;
	}
	WriteMapDescriptor(slot, texture);
	m_Maps[slot] = texture;
	Get<float>(s_MapToggleNames[slot]) = 1.0f;
}

void EnvMapVulkanMaterial::RemoveMap(uint32_t slot)
{
	if (slot >= MapCount)
	{
		return;
	}
	WriteMapDescriptor(slot, H2M::RendererH2M::GetWhiteTexture());
	m_Maps[slot] = H2M::RefH2M<H2M::Texture2D_H2M>();
	Get<float>(s_MapToggleNames[slot]) = 0.0f;
}

H2M::RefH2M<EnvMapVulkanMaterial> EnvMapVulkanMaterialLibrary::CreateMaterial(const std::string& name)
{
	H2M::RefH2M<EnvMapVulkanMaterial> material = H2M::RefH2M<EnvMapVulkanMaterial>::Create(MakeUniqueName(name));
	s_Materials.push_back(material);
	return material;
}

H2M::RefH2M<EnvMapVulkanMaterial> EnvMapVulkanMaterialLibrary::Duplicate(H2M::RefH2M<EnvMapVulkanMaterial> source)
{
	H2M::RefH2M<EnvMapVulkanMaterial> material = CreateMaterial(source->GetName());

	material->Get<glm::vec3>("u_MaterialUniforms.AlbedoColor") = source->Get<glm::vec3>("u_MaterialUniforms.AlbedoColor");
	for (const char* name : s_FloatValueNames)
	{
		material->Get<float>(name) = source->Get<float>(name);
	}
	for (uint32_t slot = 0; slot < EnvMapVulkanMaterial::MapCount; slot++)
	{
		if (source->HasMap(slot))
		{
			float toggle = source->Get<float>(s_MapToggleNames[slot]); // the map may be present but switched off
			material->SetMap(slot, source->GetMap(slot));
			material->Get<float>(s_MapToggleNames[slot]) = toggle;
		}
	}
	return material;
}

std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> EnvMapVulkanMaterialLibrary::ImportMeshMaterials(H2M::RefH2M<H2M::MeshH2M> mesh)
{
	std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> result;
	H2M::Texture2D_H2M* whiteTexture = H2M::RendererH2M::GetWhiteTexture().Raw();

	auto& meshMaterials = mesh->GetMaterials();
	for (uint32_t i = 0; i < (uint32_t)meshMaterials.size(); i++)
	{
		// Already imported with an earlier copy of this model
		H2M::RefH2M<EnvMapVulkanMaterial> material;
		for (auto& existing : s_Materials)
		{
			if (existing->m_SourceFile == mesh->GetFilePath() && existing->m_SourceIndex == i)
			{
				material = existing;
				break;
			}
		}
		if (material)
		{
			result.push_back(material);
			continue;
		}

		H2M::RefH2M<H2M::VulkanMaterialH2M> source = meshMaterials[i].As<H2M::VulkanMaterialH2M>();
		// Unnamed materials are named after the model ("boblampclean Material 2"), so they can be told apart in the library
		std::string name = !source->GetName().empty() ? source->GetName() :
			std::filesystem::path(mesh->GetFilePath()).stem().string() + " Material " + std::to_string(i);
		material = CreateMaterial(name);
		material->m_SourceFile = mesh->GetFilePath();
		material->m_SourceIndex = i;

		material->Get<glm::vec3>("u_MaterialUniforms.AlbedoColor") = source->Get<glm::vec3>("u_MaterialUniforms.AlbedoColor");
		for (const char* valueName : s_FloatValueNames)
		{
			material->Get<float>(valueName) = source->Get<float>(valueName);
		}

		// The maps the model loader bound to the mesh's own material set (missing maps got the white placeholder)
		const H2M::MeshH2M::MaterialDescriptor* meshDescriptor = mesh->FindDescriptorSet(i);
		for (uint32_t slot = 0; slot < EnvMapVulkanMaterial::MapCount; slot++)
		{
			float toggle = material->Get<float>(s_MapToggleNames[slot]);
			H2M::RefH2M<H2M::Texture2D_H2M> texture;
			if (meshDescriptor)
			{
				auto textureIt = meshDescriptor->Textures.find(s_MapTextureNames[slot]);
				if (textureIt != meshDescriptor->Textures.end()) texture = textureIt->second;
			}
			if (texture && texture.Raw() != whiteTexture)
			{
				material->SetMap(slot, texture);
				material->Get<float>(s_MapToggleNames[slot]) = toggle;
			}
			else
			{
				material->Get<float>(s_MapToggleNames[slot]) = 0.0f; // nothing to sample
			}
		}

		result.push_back(material);
	}

	return result;
}

H2M::RefH2M<EnvMapVulkanMaterial> EnvMapVulkanMaterialLibrary::GetDefaultMaterial()
{
	if (!s_DefaultMaterial)
	{
		s_DefaultMaterial = CreateMaterial("Default");
	}
	return s_DefaultMaterial;
}

void EnvMapVulkanMaterialLibrary::Remove(H2M::RefH2M<EnvMapVulkanMaterial> material)
{
	s_Materials.erase(std::remove(s_Materials.begin(), s_Materials.end(), material), s_Materials.end());
	if (s_DefaultMaterial == material)
	{
		s_DefaultMaterial = H2M::RefH2M<EnvMapVulkanMaterial>();
	}
}

void EnvMapVulkanMaterialLibrary::Clear()
{
	s_Materials.clear();
	s_DefaultMaterial = H2M::RefH2M<EnvMapVulkanMaterial>();
}

void EnvMapVulkanMaterialLibrary::Rename(H2M::RefH2M<EnvMapVulkanMaterial> material, const std::string& name)
{
	size_t first = name.find_first_not_of(" \t");
	if (!material || first == std::string::npos)
	{
		return;
	}
	std::string trimmed = name.substr(first, name.find_last_not_of(" \t") - first + 1);
	if (trimmed != material->GetName())
	{
		material->SetName(MakeUniqueName(trimmed));
	}
}

std::string EnvMapVulkanMaterialLibrary::MakeUniqueName(const std::string& name)
{
	auto taken = [](const std::string& candidate) {
		for (auto& material : s_Materials)
		{
			if (material->GetName() == candidate) return true;
		}
		return false;
	};

	if (!taken(name))
	{
		return name;
	}
	for (uint32_t n = 2; ; n++)
	{
		std::string candidate = name + " (" + std::to_string(n) + ")";
		if (!taken(candidate))
		{
			return candidate;
		}
	}
}
