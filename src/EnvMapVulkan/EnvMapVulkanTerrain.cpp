#include "EnvMapVulkanTerrain.h"

#include "EnvMapVulkanMaterialLibrary.h"
#include "EnvMapVulkanScene.h"
#include "EnvMapVulkanSceneData.h"

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
		std::shared_ptr<const TerrainLods> Lods;
		float LodTolerance = 1.0f;
		Uniforms Values;
		bool Seen = false; // in this frame's scene
	};
	static std::unordered_map<EnvMapVulkanEntityID, TerrainGPU> s_Terrains;
	static std::unordered_map<const H2M::ModelH2M*, EnvMapVulkanEntityID> s_ByModel;
	static glm::vec3 s_ViewPosition = glm::vec3(0.0f);

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

	static uint64_t MixHash(uint64_t hash, const void* data, size_t size)
	{
		const uint8_t* bytes = static_cast<const uint8_t*>(data);
		for (size_t i = 0; i < size; i++)
		{
			hash = (hash ^ bytes[i]) * 1099511628211ull; // FNV-1a
		}
		return hash;
	}

	// A guide's place in the terrain's space (a terrain is only moved: its translation)
	static glm::mat4 GuideTransform(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID terrainEntity, EnvMapVulkanEntityID guide)
	{
		glm::mat4 transform = scene.GetWorldTransform(guide);
		transform[3] -= glm::vec4(glm::vec3(scene.GetWorldTransform(terrainEntity)[3]), 0.0f);
		return transform;
	}

	uint64_t GetGuidesHash(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, const TerrainComponent& terrain)
	{
		uint64_t hash = 14695981039346656037ull;
		bool any = false;
		for (EnvMapVulkanEntityID guide : terrain.Guides)
		{
			H2M::RefH2M<H2M::ModelH2M> model = scene.Exists(guide) && scene.Has<ModelComponent>(guide) ?
				EnvMapVulkanSceneData::GetModel(scene, guide) : H2M::RefH2M<H2M::ModelH2M>();
			if (!model)
			{
				continue; // gone, or not a model: nothing to follow
			}
			any = true;
			const glm::mat4 transform = GuideTransform(scene, entity, guide);
			const H2M::ModelH2M* raw = model.Raw();
			const size_t meshes = model->GetMeshes().size();
			hash = MixHash(hash, &guide, sizeof(guide));
			hash = MixHash(hash, &raw, sizeof(raw));
			hash = MixHash(hash, &meshes, sizeof(meshes));
			hash = MixHash(hash, &transform, sizeof(transform));
			const std::string& file = model->GetFilePath();
			hash = MixHash(hash, file.data(), file.size());
		}
		return any ? (hash == 0 ? 1 : hash) : 0;
	}

	TerrainGuides GatherGuides(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, const TerrainComponent& terrain)
	{
		TerrainGuides guides;
		guides.Hash = GetGuidesHash(scene, entity, terrain);
		for (EnvMapVulkanEntityID guide : terrain.Guides)
		{
			H2M::RefH2M<H2M::ModelH2M> model = scene.Exists(guide) && scene.Has<ModelComponent>(guide) ?
				EnvMapVulkanSceneData::GetModel(scene, guide) : H2M::RefH2M<H2M::ModelH2M>();
			if (!model)
			{
				continue;
			}
			const glm::mat4 guideTransform = GuideTransform(scene, entity, guide);
			const auto& meshes = model->GetMeshes();
			for (uint32_t m = 0; m < (uint32_t)meshes.size(); m++)
			{
				// Each mesh where it is drawn (as physics makes a mesh collider of it)
				const glm::mat4 transform = EnvMapVulkanSceneData::GetMeshTransform(model, meshes[m], guideTransform);
				for (const H2M::TriangleH2M& triangle : model->GetTriangleCache(m))
				{
					guides.Triangles.push_back({ glm::vec3(transform * glm::vec4(triangle.V0.Position, 1.0f)),
						glm::vec3(transform * glm::vec4(triangle.V1.Position, 1.0f)), glm::vec3(transform * glm::vec4(triangle.V2.Position, 1.0f)) });
				}
			}
		}
		return guides;
	}

	bool UpdateHeights(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, TerrainComponent& terrain)
	{
		const uint64_t hash = Terrain::GetShapeHash(terrain.Settings, GetGuidesHash(scene, entity, terrain));
		if (terrain.Heights && terrain.Heights->ShapeHash == hash)
		{
			return false;
		}
		if (terrain.Guides.empty())
		{
			terrain.Heights = Terrain::GenerateHeights(terrain.Settings);
			return true;
		}
		const auto start = std::chrono::steady_clock::now();
		const TerrainGuides guides = GatherGuides(scene, entity, terrain);
		const float gatherMilliseconds = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();
		terrain.Heights = Terrain::GenerateHeights(terrain.Settings, &guides);
		Log::GetLogger()->info("Terrain '{0}': follows {1} guide triangles (gathered in {2:.0f} ms)", scene.Get<NameComponent>(entity).Name,
			guides.Triangles.size(), gatherMilliseconds);
		return true;
	}

	// The heights and the chunks the settings ask for
	static void Generate(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, TerrainComponent& terrain, const std::string& name)
	{
		UpdateHeights(scene, entity, terrain);
		// The chunks of these heights (physics may have made them again first: a game starts before its first frame)
		if (terrain.Model && terrain.Lods && terrain.ModelShapeHash == terrain.Heights->ShapeHash)
		{
			return;
		}
		const auto start = std::chrono::steady_clock::now();
		auto lods = std::make_shared<TerrainLods>();
		ModelData data = Terrain::BuildModelData(*terrain.Heights, lods.get(), terrain.Settings.MaxVertices);
		size_t triangles = 0; // the chunks' own (their base levels), not the coarser levels'
		for (const ModelMeshData& mesh : data.Meshes)
		{
			triangles += mesh.TriangleCount;
		}
		const size_t chunks = data.Meshes.size(), vertices = data.StaticVertices.size(), levelTriangles = data.Indices.size() - triangles;
		terrain.Lods = lods;
		terrain.Model = H2M::RefH2M<H2M::ModelH2M>::Create("Terrain '" + name + "'", std::move(data));
		terrain.ModelShapeHash = terrain.Heights->ShapeHash;
		const float meshMilliseconds = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();
		const glm::vec2 flat = terrain.Settings.FlatCenter;
		Log::GetLogger()->info("Terrain '{0}': {1:.0f} m, seed {2}, heights {3:.1f} to {4:.1f} m, the flat area at {5:.1f} m: {6} chunks, {7} vertices, "
			"{8} triangles and {9} more in the levels of detail (heights in {10:.0f} ms, meshes in {11:.0f} ms)", name, terrain.Heights->Size, terrain.Settings.Seed,
			terrain.Heights->MinHeight, terrain.Heights->MaxHeight, terrain.Heights->GetHeight(flat.x, flat.y), chunks, vertices, triangles, levelTriangles,
			terrain.Heights->Milliseconds, meshMilliseconds);
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
			Generate(scene, entity, terrain, name);
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
			gpu.Lods = terrain.Lods;
			gpu.LodTolerance = terrain.Settings.LodTolerance;
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
			values.LodTint = -1.0f;
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

	void SetViewPosition(const glm::vec3& worldPosition)
	{
		s_ViewPosition = worldPosition;
	}

	bool ChooseChunk(const H2M::ModelH2M* model, size_t meshIndex, const glm::mat4& meshTransform, ChunkDraw& draw,
		const glm::vec3* viewPosition, float toleranceScale)
	{
		auto entity = s_ByModel.find(model);
		if (entity == s_ByModel.end())
		{
			return false;
		}
		auto gpu = s_Terrains.find(entity->second);
		if (gpu == s_Terrains.end() || !gpu->second.Lods || meshIndex >= gpu->second.Lods->Chunks.size() || meshIndex >= model->GetMeshes().size())
		{
			return false;
		}
		// The distance from the camera to the chunk's box in the world (the box turned and scaled with the terrain)
		const H2M::AABB_H2M& box = model->GetMeshes()[meshIndex]->BoundingBox;
		const glm::vec3 center = glm::vec3(meshTransform * glm::vec4((box.Min + box.Max) * 0.5f, 1.0f));
		const glm::vec3 half = (box.Max - box.Min) * 0.5f;
		const glm::vec3 extent = glm::abs(glm::vec3(meshTransform[0])) * half.x + glm::abs(glm::vec3(meshTransform[1])) * half.y +
			glm::abs(glm::vec3(meshTransform[2])) * half.z;
		const glm::vec3 view = viewPosition ? *viewPosition : s_ViewPosition;
		const float distance = glm::length(glm::max(glm::abs(view - center) - extent, glm::vec3(0.0f)));
		const TerrainLods::Chunk& chunk = gpu->second.Lods->Chunks[meshIndex];
		draw.Level = TerrainLods::LevelAt(chunk, distance, gpu->second.LodTolerance * toleranceScale);
		draw.Surface = chunk.Surface[draw.Level];
		draw.Skirt = chunk.Skirt[draw.Level];
		return true;
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
