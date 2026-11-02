#include "EnvMapVulkanSceneLoader.h"

#include "EnvMapVulkanMaterialLibrary.h"

#include "Core/Log.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <set>


namespace EnvMapVulkanSceneLoader
{
	bool IsModelFile(const std::string& filepath)
	{
		static const std::set<std::string> s_Extensions = { ".fbx", ".obj", ".gltf", ".glb", ".dae", ".3ds", ".blend", ".ply", ".stl", ".x", ".md5mesh" };
		std::string extension = std::filesystem::path(filepath).extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		return s_Extensions.find(extension) != s_Extensions.end();
	}

	static std::string GetMeshName(const H2M::RefH2M<H2M::MeshH2M>& mesh, uint32_t index)
	{
		return !mesh->MeshName.empty() ? mesh->MeshName : (!mesh->NodeName.empty() ? mesh->NodeName : "Mesh " + std::to_string(index));
	}

	EnvMapVulkanEntityID AddModel(EnvMapVulkanScene& scene, const std::string& filepath)
	{
		if (!std::filesystem::exists(filepath) || !IsModelFile(filepath))
		{
			Log::GetLogger()->error("Model '{0}' was not loaded: the file does not exist or is not a supported model file.", filepath);
			return NoEntity;
		}

		H2M::RefH2M<H2M::ModelH2M> model = H2M::RefH2M<H2M::ModelH2M>::Create(filepath);
		if (!model || model->GetMeshes().empty())
		{
			Log::GetLogger()->error("Model '{0}' was not loaded: no geometry found.", filepath);
			return NoEntity;
		}

		if (model->HasAnimations())
		{
			Log::GetLogger()->info("Model '{0}': {1} animation(s), {2} bones", filepath, model->GetAnimationCount(), model->GetBoneCount());
		}

		// The model entity, named after the file, and a part entity per mesh
		const EnvMapVulkanEntityID entity = scene.CreateEntity(std::filesystem::path(filepath).filename().string());
		ModelComponent& modelComponent = scene.Add<ModelComponent>(entity);
		modelComponent.FilePath = filepath;
		modelComponent.Model = model;

		// The model's materials its meshes use go into the Material Library (reused if this model was loaded before); each
		// mesh starts with the material the model assigns to it, or the library's Default material if the model has none for it
		std::set<EnvMapVulkanMaterial*> usedMaterials;
		for (uint32_t m = 0; m < (uint32_t)model->GetMeshes().size(); m++)
		{
			const H2M::RefH2M<H2M::MeshH2M>& mesh = model->GetMeshes()[m];
			EnvMapVulkanEntityID partEntity = scene.CreateEntity(GetMeshName(mesh, m), entity);
			MeshPartComponent& part = scene.Add<MeshPartComponent>(partEntity);
			part.MeshIndex = m;
			part.SourceMeshIndex = m;
			part.Material = EnvMapVulkanMaterialLibrary::ImportModelMaterial(model, mesh->MaterialIndex);
			usedMaterials.insert(part.Material.Raw());
			part.OriginalTransform = mesh->Transform;
			TransformComponent& partTransform = scene.Get<TransformComponent>(partEntity);
			partTransform.SetMatrix(mesh->Transform);
			part.AppliedTransform = partTransform; // the mesh keeps its exact matrix until the part is edited
		}

		Log::GetLogger()->info("Model '{0}' loaded: {1} meshes, {2} materials", filepath, model->GetMeshes().size(), usedMaterials.size());
		return entity;
	}

	bool AttachModel(EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity,
		const std::unordered_map<EnvMapVulkanEntityID, EnvMapVulkanSceneSerializer::MaterialRef>& materialRefs)
	{
		ModelComponent& component = scene.Get<ModelComponent>(entity);
		if (!std::filesystem::exists(component.FilePath) || !IsModelFile(component.FilePath))
		{
			Log::GetLogger()->error("Scene: model '{0}' was not found", component.FilePath);
			return false;
		}
		H2M::RefH2M<H2M::ModelH2M> model = H2M::RefH2M<H2M::ModelH2M>::Create(component.FilePath);
		if (!model || model->GetMeshes().empty())
		{
			Log::GetLogger()->error("Scene: model '{0}' could not be loaded", component.FilePath);
			return false;
		}
		component.Model = model;

		const uint32_t meshCount = (uint32_t)model->GetMeshes().size();
		std::vector<glm::mat4> originalTransforms;
		for (const auto& mesh : model->GetMeshes())
		{
			originalTransforms.push_back(mesh->Transform);
		}
		// The parts the scene has (one whose mesh the model file no longer has is dropped)
		std::vector<EnvMapVulkanEntityID> parts;
		for (EnvMapVulkanEntityID child : std::vector<EnvMapVulkanEntityID>(scene.GetChildren(entity)))
		{
			if (const MeshPartComponent* part = scene.TryGet<MeshPartComponent>(child))
			{
				if (part->SourceMeshIndex < meshCount)
				{
					parts.push_back(child);
				}
				else
				{
					Log::GetLogger()->warn("Scene: model '{0}' has no mesh {1} any more; part '{2}' dropped", component.FilePath, part->SourceMeshIndex,
						scene.Get<NameComponent>(child).Name);
					scene.DestroyEntity(child);
				}
			}
		}
		if (parts.empty())
		{
			// No parts in the file: one per mesh, as when the model is loaded
			for (uint32_t m = 0; m < meshCount; m++)
			{
				const H2M::RefH2M<H2M::MeshH2M>& mesh = model->GetMeshes()[m];
				EnvMapVulkanEntityID partEntity = scene.CreateEntity(GetMeshName(mesh, m), entity);
				MeshPartComponent& part = scene.Add<MeshPartComponent>(partEntity);
				part.MeshIndex = part.SourceMeshIndex = m;
				scene.Get<TransformComponent>(partEntity).SetMatrix(mesh->Transform);
				parts.push_back(partEntity);
			}
		}
		std::set<uint32_t> kept;
		for (EnvMapVulkanEntityID part : parts)
		{
			kept.insert(scene.Get<MeshPartComponent>(part).SourceMeshIndex);
		}
		// The meshes removed in the scene (none of its parts uses them), from the last, so the indices before stay valid
		if (!kept.empty())
		{
			for (int m = (int)meshCount - 1; m >= 0; m--)
			{
				if (kept.find((uint32_t)m) == kept.end())
				{
					model->RemoveMesh((uint32_t)m);
				}
			}
		}

		// The parts' materials: first those the scene refers to (in the library, or loaded from their files), then the
		// model's own for the others. In this order, a saved edit of one of the model's materials is reused rather than
		// imported again from the model next to it.
		for (EnvMapVulkanEntityID partEntity : parts)
		{
			MeshPartComponent& part = scene.Get<MeshPartComponent>(partEntity);
			part.MeshIndex = (uint32_t)std::distance(kept.begin(), kept.find(part.SourceMeshIndex)); // its place among the kept meshes
			part.OriginalTransform = originalTransforms[part.SourceMeshIndex];
			part.AppliedTransform.SetMatrix(part.OriginalTransform); // the mesh keeps its exact matrix unless the part was moved
			part.Material = H2M::RefH2M<EnvMapVulkanMaterial>();

			auto ref = materialRefs.find(partEntity);
			H2M::RefH2M<EnvMapVulkanMaterial> material;
			if (ref != materialRefs.end())
			{
				material = EnvMapVulkanMaterialLibrary::FindByID(ref->second.ID);
				if (!material && !ref->second.File.empty() && std::filesystem::exists(ref->second.File))
				{
					material = EnvMapVulkanMaterialLibrary::Load(ref->second.File);
				}
				if (!material)
				{
					Log::GetLogger()->warn("Scene: part '{0}': its material (ID {1}, '{2}') was not found; the model's own is used",
						scene.Get<NameComponent>(partEntity).Name, ref->second.ID, ref->second.File);
				}
			}
			part.Material = material;
		}
		for (EnvMapVulkanEntityID partEntity : parts)
		{
			MeshPartComponent& part = scene.Get<MeshPartComponent>(partEntity);
			if (!part.Material)
			{
				part.Material = EnvMapVulkanMaterialLibrary::ImportModelMaterial(model, model->GetMeshes()[part.MeshIndex]->MaterialIndex);
			}
		}
		return true;
	}

	bool Open(const std::string& filepath, EnvMapVulkanScene& scene, std::string& sceneName, EnvMapVulkanRenderSettings& renderSettings,
		std::string& error)
	{
		// Read into a scratch scene first: a file that can't be read leaves the scene as it is
		{
			EnvMapVulkanScene check;
			std::string name;
			EnvMapVulkanRenderSettings settings;
			std::unordered_map<EnvMapVulkanEntityID, EnvMapVulkanSceneSerializer::MaterialRef> refs;
			if (!EnvMapVulkanSceneSerializer::Deserialize(filepath, check, name, settings, refs, error))
			{
				return false;
			}
		}

		scene.Clear();
		std::unordered_map<EnvMapVulkanEntityID, EnvMapVulkanSceneSerializer::MaterialRef> materialRefs;
		EnvMapVulkanSceneSerializer::Deserialize(filepath, scene, sceneName, renderSettings, materialRefs, error);

		// The models (an entity whose model can't be loaded is removed, with its children)
		std::vector<EnvMapVulkanEntityID> models;
		scene.Each<ModelComponent>([&](EnvMapVulkanEntityID entity, ModelComponent&) { models.push_back(entity); });
		for (EnvMapVulkanEntityID entity : models)
		{
			if (scene.Exists(entity) && !AttachModel(scene, entity, materialRefs))
			{
				scene.DestroyEntity(entity);
			}
		}
		return true;
	}
}
