#include "EnvMapVulkanTerrain.h"

#include "EnvMapVulkanMaterialLibrary.h"
#include "EnvMapVulkanScene.h"

#include "Assets/AssetFiles.h"
#include "Core/Log.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"
#include "H2M/Renderer/ModelH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include <glm/gtc/constants.hpp>

#include <array>
#include <chrono>
#include <cmath>
#include <unordered_map>


namespace EnvMapVulkanTerrain
{
	// The layers' maps in the set: binding 2 * layer its albedo map, 2 * layer + 1 its normal map (the material set's six
	// bindings, in their order: see Include/TerrainMaterial.glslh)
	static constexpr uint32_t BindingCount = TerrainComponent::LayerCount * 2;

	struct TerrainGPU
	{
		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet DescriptorSet;
		std::array<H2M::RefH2M<H2M::Texture2D_H2M>, BindingCount> Bound; // what each binding has (null: not written yet)
		const H2M::ModelH2M* Model = nullptr;
		Uniforms Values;
		bool Seen = false; // in this frame's scene
	};
	static std::unordered_map<EnvMapVulkanEntityID, TerrainGPU> s_Terrains;
	static std::unordered_map<const H2M::ModelH2M*, EnvMapVulkanEntityID> s_ByModel;

	static H2M::RefH2M<H2M::VulkanShaderH2M> GetMaterialShader()
	{
		return H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static").As<H2M::VulkanShaderH2M>();
	}

	// The material a layer names: the one in the library with its ID, or the one its file holds
	static void ResolveLayer(TerrainComponent::LayerMaterial& layer, const std::string& terrainName, uint32_t index)
	{
		if (layer.ID == 0)
		{
			layer.Material = H2M::RefH2M<EnvMapVulkanMaterial>();
			return;
		}
		if (layer.Material && layer.Material->GetID() == layer.ID)
		{
			return;
		}
		H2M::RefH2M<EnvMapVulkanMaterial> material = EnvMapVulkanMaterialLibrary::FindByID(layer.ID);
		if (!material && !layer.File.empty() && AssetFiles::Exists(layer.File))
		{
			material = EnvMapVulkanMaterialLibrary::Load(layer.File);
		}
		if (!material)
		{
			Log::GetLogger()->warn("Terrain '{0}': the {1} layer's material (ID {2}, '{3}') was not found: a plain color is used", terrainName,
				TerrainComponent::GetLayerName(index), layer.ID, layer.File);
			layer.ID = 0; // asked once
		}
		layer.Material = material;
	}

	// The heights and the chunks the settings ask for
	static void Generate(TerrainComponent& terrain, const std::string& name)
	{
		const uint64_t hash = terrain.Settings.GetShapeHash();
		if (terrain.Heights && terrain.Heights->ShapeHash == hash && terrain.Model)
		{
			return;
		}
		if (!terrain.Heights || terrain.Heights->ShapeHash != hash)
		{
			terrain.Heights = Terrain::GenerateHeights(terrain.Settings);
		}
		const auto start = std::chrono::steady_clock::now();
		ModelData data = Terrain::BuildModelData(*terrain.Heights);
		const size_t chunks = data.Meshes.size(), vertices = data.StaticVertices.size(), triangles = data.Indices.size();
		terrain.Model = H2M::RefH2M<H2M::ModelH2M>::Create("Terrain '" + name + "'", std::move(data));
		const float meshMilliseconds = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();
		const glm::vec2 flat = terrain.Settings.FlatCenter;
		Log::GetLogger()->info("Terrain '{0}': {1:.0f} m, seed {2}, heights {3:.1f} to {4:.1f} m, the flat area at {5:.1f} m: {6} chunks, {7} vertices, "
			"{8} triangles (heights in {9:.0f} ms, meshes in {10:.0f} ms)", name, terrain.Heights->Size, terrain.Settings.Seed, terrain.Heights->MinHeight,
			terrain.Heights->MaxHeight, terrain.Heights->GetHeight(flat.x, flat.y), chunks, vertices, triangles, terrain.Heights->Milliseconds, meshMilliseconds);
	}

	static float MaterialValue(H2M::RefH2M<EnvMapVulkanMaterial> material, const char* name)
	{
		return material ? material->Get<float>(name) : 0.0f;
	}

	void Update(EnvMapVulkanScene& scene)
	{
		for (auto& [entity, gpu] : s_Terrains)
		{
			gpu.Seen = false;
		}
		s_ByModel.clear();

		bool waited = false;
		scene.Each<TerrainComponent>([&](EnvMapVulkanEntityID entity, TerrainComponent& terrain)
		{
			const std::string& name = scene.Get<NameComponent>(entity).Name;
			Generate(terrain, name);
			if (!terrain.Model)
			{
				return;
			}
			for (uint32_t layer = 0; layer < TerrainComponent::LayerCount; layer++)
			{
				ResolveLayer(terrain.Layers[layer], name, layer);
			}

			TerrainGPU& gpu = s_Terrains[entity];
			gpu.Seen = true;
			gpu.Model = terrain.Model.Raw();
			s_ByModel[gpu.Model] = entity;
			if (gpu.DescriptorSet.DescriptorSets.empty())
			{
				gpu.DescriptorSet = GetMaterialShader()->CreateDescriptorSets(H2M::VulkanShaderH2M::MaterialDescriptorSet);
			}

			// The layers' maps into the set (a binding that changed: the GPU may still read the set, so it waits first)
			Uniforms& values = gpu.Values;
			uint32_t toggles = 0;
			for (uint32_t layer = 0; layer < TerrainComponent::LayerCount; layer++)
			{
				const H2M::RefH2M<EnvMapVulkanMaterial>& material = terrain.Layers[layer].Material;
				values.LayerMaterial[layer] = material ? MaterialValue(material, "u_MaterialUniforms.MaterialIndex") : -1.0f;
				for (uint32_t map = 0; map < 2; map++)
				{
					const uint32_t slot = map == 0 ? EnvMapVulkanMaterial::Albedo : EnvMapVulkanMaterial::Normal;
					const bool used = material && material->HasMap(slot) && MaterialValue(material, EnvMapVulkanMaterial::GetMapToggleName(slot)) > 0.5f;
					H2M::RefH2M<H2M::Texture2D_H2M> texture = used ? material->GetMap(slot) : H2M::RendererH2M::GetWhiteTexture();
					if (used)
					{
						toggles |= 1u << (layer * 2 + map);
					}
					const uint32_t binding = layer * 2 + map;
					if (gpu.Bound[binding] == texture || gpu.DescriptorSet.DescriptorSets.empty())
					{
						continue;
					}
					const VkWriteDescriptorSet* declared = GetMaterialShader()->GetDescriptorSet(EnvMapVulkanMaterial::GetMapTextureName(binding),
						H2M::VulkanShaderH2M::MaterialDescriptorSet);
					if (!declared)
					{
						continue;
					}
					if (!waited && gpu.Bound[binding])
					{
						vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice());
						waited = true;
					}
					VkWriteDescriptorSet write = *declared;
					write.dstSet = gpu.DescriptorSet.DescriptorSets[0];
					write.descriptorCount = 1;
					write.pImageInfo = &texture.As<H2M::VulkanTexture2D_H2M>()->GetVulkanDescriptorInfo();
					vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), 1, &write, 0, nullptr);
					gpu.Bound[binding] = texture;
				}
			}
			values.MapToggles = (float)toggles;
			const H2M::RefH2M<EnvMapVulkanMaterial>& grass = terrain.Layers[TerrainComponent::Grass].Material;
			values.MaterialIndex = grass ? MaterialValue(grass, "u_MaterialUniforms.MaterialIndex") :
				MaterialValue(EnvMapVulkanMaterialLibrary::GetDefaultMaterial(), "u_MaterialUniforms.MaterialIndex");

			// The look's heights count from the terrain's lowest point, in the world; the slope as 1 - the normal's y, over
			// a few degrees around the setting
			const float base = scene.GetWorldTransform(entity)[3].y + terrain.Heights->MinHeight;
			values.SandHeight = base + terrain.Settings.SandHeight;
			values.RockHeight = base + terrain.Settings.RockHeight;
			const float slope = glm::radians(glm::clamp(terrain.Settings.RockSlope, 0.0f, 90.0f));
			values.RockSlopeStart = 1.0f - std::cos(std::max(slope - glm::radians(4.0f), 0.0f));
			values.RockSlopeEnd = 1.0f - std::cos(std::min(slope + glm::radians(4.0f), glm::half_pi<float>()));
		});

		// Terrains gone from the scene: their sets stay with their pools (a few per editing session), the entries go
		for (auto it = s_Terrains.begin(); it != s_Terrains.end();)
		{
			it = it->second.Seen ? std::next(it) : s_Terrains.erase(it);
		}
	}

	void Shutdown()
	{
		s_Terrains.clear();
		s_ByModel.clear();
	}

	bool Find(const H2M::ModelH2M* model, VkDescriptorSet& descriptorSet, Uniforms& uniforms)
	{
		auto entity = s_ByModel.find(model);
		if (entity == s_ByModel.end())
		{
			return false;
		}
		auto gpu = s_Terrains.find(entity->second);
		if (gpu == s_Terrains.end() || gpu->second.DescriptorSet.DescriptorSets.empty())
		{
			return false;
		}
		descriptorSet = gpu->second.DescriptorSet.DescriptorSets[0];
		uniforms = gpu->second.Values;
		return true;
	}
}
