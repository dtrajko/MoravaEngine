#include "EnvMapVulkanFoliage.h"

#include "EnvMapVulkanMaterialLibrary.h"
#include "EnvMapVulkanScene.h"

#include "Assets/AssetFiles.h"
#include "Core/Log.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanVertexBufferH2M.h"
#include "H2M/Renderer/ModelH2M.h"
#include "Physics/ScenePhysics.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <unordered_set>


namespace EnvMapVulkanFoliage
{
	// A species of a component: its model, the material of each mesh, and the transform its meshes have in the file (folded
	// into every copy's: the instanced shaders read one transform per copy)
	struct Species
	{
		std::string File;
		H2M::RefH2M<H2M::ModelH2M> Model;
		std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> Materials; // per mesh
		glm::mat4 Local = glm::mat4(1.0f);
		glm::vec3 BoundsMin = glm::vec3(0.0f), BoundsMax = glm::vec3(0.0f); // the meshes' boxes, in the copy's space
	};

	struct ComponentState
	{
		std::vector<Species> Species;
		bool Seen = false;
	};

	// A species' copies in one cell: a range of the instance buffer, and its box in the world
	struct Range
	{
		const Species* Kind = nullptr;
		uint32_t First = 0, Count = 0;
		glm::vec3 Min = glm::vec3(0.0f), Max = glm::vec3(0.0f);
		float DrawDistance = 0.0f, ShadowDistance = 0.0f;
		uint32_t Cell = 0; // in s_CellBoxes
	};

	static std::unordered_map<EnvMapVulkanEntityID, ComponentState> s_Components;
	static std::vector<Range> s_Ranges;     // species by species, each one's cells in order
	static std::vector<CellBox> s_CellBoxes;
	static H2M::RefH2M<H2M::VertexBufferH2M> s_InstanceBuffer;
	static uint64_t s_BufferKey = 0;        // what the buffer was written from (MakeBufferKey)
	static std::vector<Draw> s_MainViewDraws;
	static Stats s_Stats;

	static uint64_t Mix(uint64_t hash, uint64_t value)
	{
		hash ^= value + 0x9E3779B97F4A7C15ull + (hash << 6) + (hash >> 2);
		return hash;
	}

	static uint64_t HashMatrix(uint64_t hash, const glm::mat4& matrix)
	{
		const uint32_t* bits = reinterpret_cast<const uint32_t*>(&matrix[0][0]);
		for (int i = 0; i < 16; i++)
		{
			hash = Mix(hash, bits[i]);
		}
		return hash;
	}

	// A species' model and materials, loaded when its file changed
	static void LoadSpecies(Species& species, const std::string& file, const std::string& owner)
	{
		if (species.File == file && (species.Model || file.empty()))
		{
			return;
		}
		species = Species();
		species.File = file;
		if (file.empty())
		{
			return;
		}
		if (!AssetFiles::Exists(file))
		{
			Log::GetLogger()->warn("Foliage '{0}': the model '{1}' was not found", owner, file);
			return;
		}
		H2M::RefH2M<H2M::ModelH2M> model = H2M::ModelH2M::LoadShared(file);
		if (!model || model->GetMeshes().empty())
		{
			Log::GetLogger()->warn("Foliage '{0}': the model '{1}' could not be loaded", owner, file);
			return;
		}
		if (model->IsSkinned())
		{
			Log::GetLogger()->warn("Foliage '{0}': the model '{1}' is skinned: foliage is drawn from static models only", owner, file);
			return;
		}
		species.Model = model;
		const auto& meshes = model->GetMeshes();
		species.Local = meshes[0]->Transform;
		bool sameTransforms = true;
		glm::vec3 boundsMin(1e30f), boundsMax(-1e30f);
		for (const H2M::RefH2M<H2M::MeshH2M>& mesh : meshes)
		{
			species.Materials.push_back(EnvMapVulkanMaterialLibrary::ImportModelMaterial(model, mesh->MaterialIndex));
			sameTransforms = sameTransforms && mesh->Transform == species.Local;
			const H2M::AABB_H2M& box = mesh->BoundingBox;
			for (int corner = 0; corner < 8; corner++)
			{
				const glm::vec3 point((corner & 1) ? box.Max.x : box.Min.x, (corner & 2) ? box.Max.y : box.Min.y, (corner & 4) ? box.Max.z : box.Min.z);
				const glm::vec3 placed = glm::vec3(species.Local * glm::vec4(point, 1.0f));
				boundsMin = glm::min(boundsMin, placed);
				boundsMax = glm::max(boundsMax, placed);
			}
		}
		species.BoundsMin = boundsMin;
		species.BoundsMax = boundsMax;
		if (!sameTransforms)
		{
			Log::GetLogger()->warn("Foliage '{0}': the meshes of '{1}' have different transforms in the file: all are drawn with the first one's "
				"(apply the transforms when exporting it)", owner, file);
		}
	}

	// The copies of a component, made again when its settings or its terrain's shape changed, or it has none (Scatter Again)
	static void Scatter(EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, FoliageComponent& foliage, const TerrainComponent& terrain,
		std::unique_ptr<ScenePhysics>& physics)
	{
		const uint64_t hash = Foliage::GetScatterHash(foliage.Settings, *terrain.Heights);
		if (foliage.Instances && foliage.Instances->ScatterHash == hash)
		{
			return;
		}
		// The keep-out: the scene's static colliders (the road, its walls), made once for all the components of this frame
		if (!physics)
		{
			const auto start = std::chrono::steady_clock::now();
			physics = std::make_unique<ScenePhysics>();
			physics->StartStatic(scene);
			Log::GetLogger()->info("Foliage: the static colliders for the keep-out in {0:.0f} ms",
				std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count());
		}
		const glm::mat4 world = scene.GetWorldTransform(entity);
		ScenePhysics* query = physics.get();
		Foliage::KeepOutQuery keepOut = [query, world](const glm::vec3& ground, float radius, float height)
		{
			if (radius <= 0.0f && height <= 0.0f)
			{
				return false;
			}
			const glm::vec3 bottom = glm::vec3(world * glm::vec4(ground, 1.0f));
			const glm::vec3 top = glm::vec3(world * glm::vec4(ground + glm::vec3(0.0f, std::max(height, 0.0f), 0.0f), 1.0f));
			return query->OverlapsCapsule(bottom, top, std::max(radius, 0.05f), ObjectLayers::StaticBit);
		};
		foliage.Instances = Foliage::Scatter(foliage.Settings, *terrain.Heights, keepOut);

		const FoliageInstances& instances = *foliage.Instances;
		std::string counts;
		for (size_t s = 0; s < instances.CopiesBySpecies.size(); s++)
		{
			counts += (s > 0 ? ", " : "") + std::to_string(instances.CopiesBySpecies[s]);
		}
		Log::GetLogger()->info("Foliage '{0}': {1} copies ({2} by species) in {3} cells, {4} places kept out, in {5:.0f} ms", scene.Get<NameComponent>(entity).Name,
			instances.Copies.size(), counts, instances.Cells.size(), instances.KeptOut, instances.Milliseconds);
	}

	// What the instance buffer is written from: every drawn component's copies, its place, its species' models
	static uint64_t MakeBufferKey(EnvMapVulkanScene& scene)
	{
		uint64_t key = 0x5EED;
		scene.Each<FoliageComponent>([&](EnvMapVulkanEntityID entity, FoliageComponent& foliage)
		{
			auto it = s_Components.find(entity);
			if (!foliage.Enabled || !foliage.Instances || it == s_Components.end())
			{
				return;
			}
			key = Mix(key, entity);
			key = Mix(key, (uint64_t)(uintptr_t)foliage.Instances.get());
			key = Mix(key, foliage.Instances->ScatterHash);
			key = HashMatrix(key, scene.GetWorldTransform(entity));
			key = Mix(key, std::hash<float>()(foliage.Settings.DrawDistance));
			key = Mix(key, std::hash<float>()(foliage.Settings.ShadowDistance));
			key = Mix(key, (uint64_t)(uintptr_t)it->second.Species.data()); // the ranges point into it
			for (const Species& species : it->second.Species)
			{
				key = Mix(key, (uint64_t)(uintptr_t)species.Model.Raw());
			}
		});
		return key;
	}

	static void WriteInstanceBuffer(EnvMapVulkanScene& scene)
	{
		std::vector<glm::mat4> transforms;
		s_Ranges.clear();
		s_CellBoxes.clear();
		scene.Each<FoliageComponent>([&](EnvMapVulkanEntityID entity, FoliageComponent& foliage)
		{
			auto it = s_Components.find(entity);
			if (!foliage.Enabled || !foliage.Instances || it == s_Components.end())
			{
				return;
			}
			const FoliageInstances& instances = *foliage.Instances;
			const std::vector<Species>& kinds = it->second.Species;
			const glm::mat4 world = scene.GetWorldTransform(entity);
			const uint32_t firstCell = (uint32_t)s_CellBoxes.size();
			for (size_t c = 0; c < instances.Cells.size(); c++)
			{
				s_CellBoxes.push_back({ glm::vec3(1e30f), glm::vec3(-1e30f), false });
			}
			// Species by species (one model bound for all its cells), each one's copies cell by cell
			for (uint32_t s = 0; s < (uint32_t)kinds.size() && s < (uint32_t)foliage.Settings.Species.size(); s++)
			{
				const Species& kind = kinds[s];
				if (!kind.Model)
				{
					continue;
				}
				for (size_t c = 0; c < instances.Cells.size(); c++)
				{
					const FoliageInstances::Cell& cell = instances.Cells[c];
					Range range;
					range.Kind = &kind;
					range.First = (uint32_t)transforms.size();
					range.Min = glm::vec3(1e30f);
					range.Max = glm::vec3(-1e30f);
					range.DrawDistance = foliage.Settings.DrawDistance;
					range.ShadowDistance = foliage.Settings.ShadowDistance;
					range.Cell = firstCell + (uint32_t)c;
					for (uint32_t i = cell.First; i < cell.First + cell.Count; i++)
					{
						const FoliageInstances::Copy& copy = instances.Copies[i];
						if (copy.Species != s)
						{
							continue;
						}
						const glm::mat4 transform = world * copy.Transform * kind.Local;
						transforms.push_back(transform);
						// The copy's box: the species' box turned with the copy (its 8 corners)
						const glm::mat4 boxTransform = world * copy.Transform;
						for (int corner = 0; corner < 8; corner++)
						{
							const glm::vec3 point((corner & 1) ? kind.BoundsMax.x : kind.BoundsMin.x, (corner & 2) ? kind.BoundsMax.y : kind.BoundsMin.y,
								(corner & 4) ? kind.BoundsMax.z : kind.BoundsMin.z);
							const glm::vec3 placed = glm::vec3(boxTransform * glm::vec4(point, 1.0f));
							range.Min = glm::min(range.Min, placed);
							range.Max = glm::max(range.Max, placed);
						}
					}
					range.Count = (uint32_t)transforms.size() - range.First;
					if (range.Count == 0)
					{
						continue;
					}
					CellBox& box = s_CellBoxes[range.Cell];
					box.Min = glm::min(box.Min, range.Min);
					box.Max = glm::max(box.Max, range.Max);
					s_Ranges.push_back(range);
				}
			}
		});

		// The buffer is used by the frames in flight: replaced only when the GPU is done with them (a scatter, a moved terrain)
		vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice());
		s_InstanceBuffer = H2M::RefH2M<H2M::VertexBufferH2M>();
		if (!transforms.empty())
		{
			s_InstanceBuffer = H2M::VertexBufferH2M::Create(transforms.data(), (uint32_t)(transforms.size() * sizeof(glm::mat4)));
		}
		s_Stats.Copies = (uint32_t)transforms.size();
		s_Stats.Cells = (uint32_t)s_CellBoxes.size();
	}

	void Update(EnvMapVulkanScene& scene)
	{
		for (auto& [entity, state] : s_Components)
		{
			state.Seen = false;
		}
		std::unique_ptr<ScenePhysics> physics; // made the first time a component scatters
		uint32_t speciesCount = 0;
		scene.Each<FoliageComponent>([&](EnvMapVulkanEntityID entity, FoliageComponent& foliage)
		{
			const TerrainComponent* terrain = scene.TryGet<TerrainComponent>(entity);
			if (!foliage.Enabled || !terrain || !terrain->Heights)
			{
				return; // a terrain's heights are made by EnvMapVulkanTerrain::Update, before this
			}
			ComponentState& state = s_Components[entity];
			state.Seen = true;
			const std::string& name = scene.Get<NameComponent>(entity).Name;
			state.Species.resize(foliage.Settings.Species.size());
			for (size_t s = 0; s < foliage.Settings.Species.size(); s++)
			{
				LoadSpecies(state.Species[s], foliage.Settings.Species[s].Model, name);
				speciesCount += state.Species[s].Model ? 1 : 0;
			}
			Scatter(scene, entity, foliage, *terrain, physics);
		});
		if (physics)
		{
			physics->Stop();
		}
		for (auto it = s_Components.begin(); it != s_Components.end();)
		{
			it = it->second.Seen ? std::next(it) : s_Components.erase(it);
		}
		s_Stats.Species = speciesCount;

		const uint64_t key = MakeBufferKey(scene);
		if (key != s_BufferKey)
		{
			s_BufferKey = key;
			WriteInstanceBuffer(scene);
		}
	}

	void Shutdown()
	{
		s_InstanceBuffer = H2M::RefH2M<H2M::VertexBufferH2M>();
		s_Components.clear();
		s_Ranges.clear();
		s_CellBoxes.clear();
		s_MainViewDraws.clear();
		s_BufferKey = 0;
		s_Stats = Stats();
	}

	static bool IsBoxOutside(const glm::vec4 planes[6], const glm::vec3& boxMin, const glm::vec3& boxMax)
	{
		const glm::vec3 center = 0.5f * (boxMin + boxMax), extent = 0.5f * (boxMax - boxMin);
		for (int p = 0; p < 6; p++)
		{
			const glm::vec3 normal(planes[p]);
			const float reach = glm::dot(extent, glm::abs(normal));
			if (glm::dot(normal, center) + planes[p].w + reach < 0.0f)
			{
				return true;
			}
		}
		return false;
	}

	static float DistanceToBox(const glm::vec3& point, const glm::vec3& boxMin, const glm::vec3& boxMax)
	{
		return glm::length(glm::clamp(point, boxMin, boxMax) - point);
	}

	// The draws of the ranges that pass: neighboring ranges of a species (one after the other in the buffer) in one draw
	template<typename Passes>
	static void MakeDraws(std::vector<Draw>& draws, Passes passes, uint32_t* copiesDrawn = nullptr)
	{
		draws.clear();
		const Range* runStart = nullptr;
		uint32_t runCount = 0;
		auto flush = [&]()
		{
			if (!runStart)
			{
				return;
			}
			const Species& kind = *runStart->Kind;
			const auto& meshes = kind.Model->GetMeshes();
			for (uint32_t m = 0; m < (uint32_t)meshes.size() && m < (uint32_t)kind.Materials.size(); m++)
			{
				if (!kind.Materials[m] || kind.Materials[m]->IsGlass())
				{
					continue; // glass isn't drawn instanced (it has passes of its own)
				}
				Draw draw;
				draw.Model = kind.Model.Raw();
				draw.Mesh = m;
				draw.Material = const_cast<EnvMapVulkanMaterial*>(kind.Materials[m].Raw());
				draw.FirstInstance = runStart->First;
				draw.InstanceCount = runCount;
				draws.push_back(draw);
			}
			if (copiesDrawn)
			{
				*copiesDrawn += runCount;
			}
			runStart = nullptr;
			runCount = 0;
		};
		for (const Range& range : s_Ranges)
		{
			if (!passes(range))
			{
				flush();
				continue;
			}
			if (runStart && runStart->Kind == range.Kind && runStart->First + runCount == range.First)
			{
				runCount += range.Count;
				continue;
			}
			flush();
			runStart = &range;
			runCount = range.Count;
		}
		flush();
	}

	void CullMainView(const glm::vec4 planes[6], const glm::vec3& viewPosition, bool culling)
	{
		for (CellBox& box : s_CellBoxes)
		{
			box.Drawn = false;
		}
		s_Stats.CopiesDrawn = 0;
		MakeDraws(s_MainViewDraws, [&](const Range& range)
		{
			if (DistanceToBox(viewPosition, range.Min, range.Max) > range.DrawDistance || (culling && IsBoxOutside(planes, range.Min, range.Max)))
			{
				return false;
			}
			s_CellBoxes[range.Cell].Drawn = true;
			return true;
		}, &s_Stats.CopiesDrawn);
		s_Stats.CellsDrawn = (uint32_t)std::count_if(s_CellBoxes.begin(), s_CellBoxes.end(), [](const CellBox& box) { return box.Drawn; });
	}

	const std::vector<Draw>& GetMainViewDraws()
	{
		return s_MainViewDraws;
	}

	void CollectShadowDraws(const glm::vec4 planes[6], const glm::vec3& viewPosition, std::vector<Draw>& draws, const glm::vec3* lightCenter, float lightRadius)
	{
		MakeDraws(draws, [&](const Range& range)
		{
			if (DistanceToBox(viewPosition, range.Min, range.Max) > range.ShadowDistance)
			{
				return false;
			}
			if (lightCenter && DistanceToBox(*lightCenter, range.Min, range.Max) > lightRadius)
			{
				return false;
			}
			return !IsBoxOutside(planes, range.Min, range.Max);
		});
	}

	VkBuffer GetInstanceBuffer()
	{
		return s_InstanceBuffer ? s_InstanceBuffer.As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer() : VK_NULL_HANDLE;
	}

	bool HasCopies()
	{
		return s_InstanceBuffer && !s_Ranges.empty();
	}

	const std::vector<CellBox>& GetCellBoxes()
	{
		return s_CellBoxes;
	}

	const Stats& GetStats()
	{
		return s_Stats;
	}
}
