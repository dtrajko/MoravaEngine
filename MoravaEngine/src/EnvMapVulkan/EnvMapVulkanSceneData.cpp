#include "EnvMapVulkanSceneData.h"

#include "EnvMapVulkanMaterialLibrary.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>


namespace EnvMapVulkanSceneData
{
	static void CollectModels(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, std::vector<EnvMapVulkanEntityID>& models)
	{
		if (scene.Has<ModelComponent>(entity))
		{
			models.push_back(entity);
		}
		for (EnvMapVulkanEntityID child : scene.GetChildren(entity))
		{
			CollectModels(scene, child, models);
		}
	}

	std::vector<EnvMapVulkanEntityID> GetModels(const EnvMapVulkanScene& scene)
	{
		std::vector<EnvMapVulkanEntityID> models;
		for (EnvMapVulkanEntityID root : scene.GetRoots())
		{
			CollectModels(scene, root, models);
		}
		return models;
	}

	H2M::RefH2M<H2M::ModelH2M> GetModel(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID model)
	{
		const ModelComponent* component = scene.TryGet<ModelComponent>(model);
		return component ? component->Model : H2M::RefH2M<H2M::ModelH2M>();
	}

	EnvMapVulkanEntityID FindPart(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID model, int meshIndex)
	{
		for (EnvMapVulkanEntityID child : scene.GetChildren(model))
		{
			const MeshPartComponent* part = scene.TryGet<MeshPartComponent>(child);
			if (part && (int)part->MeshIndex == meshIndex)
			{
				return child;
			}
		}
		return NoEntity;
	}

	std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> GetMeshMaterials(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID model)
	{
		H2M::RefH2M<H2M::ModelH2M> modelRef = GetModel(scene, model);
		std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> materials(modelRef ? modelRef->GetMeshes().size() : 0);
		for (EnvMapVulkanEntityID child : scene.GetChildren(model))
		{
			const MeshPartComponent* part = scene.TryGet<MeshPartComponent>(child);
			if (part && part->MeshIndex < materials.size())
			{
				materials[part->MeshIndex] = part->Material;
			}
		}
		for (auto& material : materials)
		{
			if (!material)
			{
				material = EnvMapVulkanMaterialLibrary::GetDefaultMaterial();
			}
		}
		return materials;
	}

	glm::mat4 GetMeshTransform(const H2M::RefH2M<H2M::ModelH2M>& model, const H2M::RefH2M<H2M::MeshH2M>& mesh, const glm::mat4& transform)
	{
		return (model->IsSkinned() && mesh->IsRigged) ? transform * model->GetRootTransform() : transform * mesh->Transform;
	}

	EnvMapVulkanEntityID LightEntities::Get(LightKind kind, int index) const
	{
		switch (kind)
		{
			case LightKind::Sun:   return Sun;
			case LightKind::Point: return index >= 0 && index < (int)PointLights.size() ? PointLights[index] : NoEntity;
			case LightKind::Spot:  return index >= 0 && index < (int)SpotLights.size() ? SpotLights[index] : NoEntity;
			default:               return NoEntity;
		}
	}

	LightKind LightEntities::Find(EnvMapVulkanEntityID entity, int& index) const
	{
		index = 0;
		if (entity == NoEntity)
		{
			return LightKind::None;
		}
		if (entity == Sun)
		{
			return LightKind::Sun;
		}
		for (int i = 0; i < (int)PointLights.size(); i++)
		{
			if (PointLights[i] == entity)
			{
				index = i;
				return LightKind::Point;
			}
		}
		for (int i = 0; i < (int)SpotLights.size(); i++)
		{
			if (SpotLights[i] == entity)
			{
				index = i;
				return LightKind::Spot;
			}
		}
		return LightKind::None;
	}

	template<typename Component>
	static void CollectEntitiesWith(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, std::vector<EnvMapVulkanEntityID>& out)
	{
		if (scene.Has<Component>(entity))
		{
			out.push_back(entity);
		}
		for (EnvMapVulkanEntityID child : scene.GetChildren(entity))
		{
			CollectEntitiesWith<Component>(scene, child, out);
		}
	}

	LightEntities ReadLights(const EnvMapVulkanScene& scene, EnvMapVulkanLightEnvironment& lights)
	{
		LightEntities entities;
		entities.Sun = scene.FindFirst<SunComponent>();
		lights.Sun = ReadSun(scene, entities.Sun);
		for (EnvMapVulkanEntityID root : scene.GetRoots())
		{
			CollectEntitiesWith<PointLightComponent>(scene, root, entities.PointLights);
			CollectEntitiesWith<SpotLightComponent>(scene, root, entities.SpotLights);
		}
		lights.PointLights.clear();
		for (EnvMapVulkanEntityID entity : entities.PointLights)
		{
			lights.PointLights.push_back(ReadPointLight(scene, entity));
		}
		lights.SpotLights.clear();
		for (EnvMapVulkanEntityID entity : entities.SpotLights)
		{
			lights.SpotLights.push_back(ReadSpotLight(scene, entity));
		}
		return entities;
	}

	glm::vec3 GetEntityPosition(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity)
	{
		return glm::vec3(scene.GetWorldTransform(entity)[3]);
	}

	void SetEntityPosition(EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, const glm::vec3& position)
	{
		glm::mat4 world = scene.GetWorldTransform(entity);
		world[3] = glm::vec4(position, 1.0f);
		scene.SetWorldTransform(entity, world);
	}

	glm::vec3 GetEntityLightDirection(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity)
	{
		TransformComponent world;
		world.SetMatrix(scene.GetWorldTransform(entity));
		return world.GetLightDirection();
	}

	void SetEntityLightDirection(EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, const glm::vec3& direction)
	{
		TransformComponent world;
		world.SetMatrix(scene.GetWorldTransform(entity));
		world.SetLightDirection(direction);
		scene.SetWorldTransform(entity, world.GetMatrix());
	}

	EnvMapVulkanDirectionalLight ReadSun(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity)
	{
		EnvMapVulkanDirectionalLight sun;
		const SunComponent* component = scene.TryGet<SunComponent>(entity);
		if (!component)
		{
			sun.Enabled = false;
			return sun;
		}
		sun.Enabled = component->Enabled;
		sun.Color = component->Color;
		sun.Intensity = component->Intensity;
		sun.FollowEnvironmentRotation = component->FollowEnvironmentRotation;
		sun.CastShadows = component->CastShadows;
		sun.SetDirection(-GetEntityLightDirection(scene, entity)); // the struct's direction points towards the sun
		sun.IconMoved = component->IconPlaced;
		sun.IconPosition = GetEntityPosition(scene, entity);
		return sun;
	}

	void WriteSun(EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, const EnvMapVulkanDirectionalLight& sun, const EnvMapVulkanDirectionalLight& before)
	{
		SunComponent* component = scene.TryGet<SunComponent>(entity);
		if (!component)
		{
			return;
		}
		component->Enabled = sun.Enabled;
		component->Color = sun.Color;
		component->Intensity = sun.Intensity;
		component->FollowEnvironmentRotation = sun.FollowEnvironmentRotation;
		component->CastShadows = sun.CastShadows;
		component->IconPlaced = sun.IconMoved;
		if (sun.Azimuth != before.Azimuth || sun.Elevation != before.Elevation)
		{
			SetEntityLightDirection(scene, entity, -sun.GetDirection());
		}
		if (sun.IconPosition != before.IconPosition)
		{
			SetEntityPosition(scene, entity, sun.IconPosition);
		}
	}

	EnvMapVulkanPointLight ReadPointLight(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity)
	{
		EnvMapVulkanPointLight light;
		const PointLightComponent& component = scene.Get<PointLightComponent>(entity);
		light.Name = scene.Get<NameComponent>(entity).Name;
		light.Enabled = component.Enabled;
		light.Color = component.Color;
		light.Intensity = component.Intensity;
		light.Position = GetEntityPosition(scene, entity);
		light.Range = component.Range;
		light.CastShadows = component.CastShadows;
		return light;
	}

	void WritePointLight(EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, const EnvMapVulkanPointLight& light, const EnvMapVulkanPointLight* before)
	{
		PointLightComponent& component = scene.Get<PointLightComponent>(entity);
		scene.Get<NameComponent>(entity).Name = light.Name;
		component.Enabled = light.Enabled;
		component.Color = light.Color;
		component.Intensity = light.Intensity;
		component.Range = light.Range;
		component.CastShadows = light.CastShadows;
		if (!before || light.Position != before->Position)
		{
			SetEntityPosition(scene, entity, light.Position);
		}
	}

	EnvMapVulkanSpotLight ReadSpotLight(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity)
	{
		EnvMapVulkanSpotLight light;
		const SpotLightComponent& component = scene.Get<SpotLightComponent>(entity);
		light.Name = scene.Get<NameComponent>(entity).Name;
		light.Enabled = component.Enabled;
		light.Color = component.Color;
		light.Intensity = component.Intensity;
		light.Position = GetEntityPosition(scene, entity);
		light.Range = component.Range;
		light.SetDirection(GetEntityLightDirection(scene, entity));
		light.InnerAngle = component.InnerAngle;
		light.OuterAngle = component.OuterAngle;
		light.CastShadows = component.CastShadows;
		return light;
	}

	void WriteSpotLight(EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, const EnvMapVulkanSpotLight& light, const EnvMapVulkanSpotLight* before)
	{
		SpotLightComponent& component = scene.Get<SpotLightComponent>(entity);
		scene.Get<NameComponent>(entity).Name = light.Name;
		component.Enabled = light.Enabled;
		component.Color = light.Color;
		component.Intensity = light.Intensity;
		component.Range = light.Range;
		component.InnerAngle = light.InnerAngle;
		component.OuterAngle = light.OuterAngle;
		component.CastShadows = light.CastShadows;
		if (!before || light.Position != before->Position)
		{
			SetEntityPosition(scene, entity, light.Position);
		}
		if (!before || light.Azimuth != before->Azimuth || light.Elevation != before->Elevation)
		{
			SetEntityLightDirection(scene, entity, light.GetDirection());
		}
	}

	EnvMapVulkanEntityID ReadWater(const EnvMapVulkanScene& scene, EnvMapVulkanWaterSettings& settings)
	{
		const EnvMapVulkanEntityID entity = scene.FindFirst<WaterComponent>();
		if (entity == NoEntity)
		{
			settings.Enabled = false;
			return NoEntity;
		}
		settings = scene.Get<WaterComponent>(entity).Settings;
		const glm::mat4 world = scene.GetWorldTransform(entity);
		const glm::vec3 sideX = glm::vec3(world[0]);
		const glm::vec3 sideZ = glm::vec3(world[2]);
		settings.Enabled = true;
		settings.Center = glm::vec2(world[3].x, world[3].z);
		settings.Height = world[3].y;
		settings.Rotation = glm::degrees(std::atan2(-sideX.z, sideX.x));
		settings.Size = glm::max(glm::vec2(glm::length(sideX), glm::length(sideZ)), glm::vec2(0.1f));
		return entity;
	}

	EnvMapVulkanEntityID ReadProbeVolume(const EnvMapVulkanScene& scene, EnvMapVulkanProbeVolumeSettings& settings)
	{
		const EnvMapVulkanEntityID entity = scene.FindFirst<ProbeVolumeComponent>();
		if (entity == NoEntity)
		{
			settings.Exists = false;
			return NoEntity;
		}
		settings = scene.Get<ProbeVolumeComponent>(entity).Settings;
		const glm::mat4 world = scene.GetWorldTransform(entity);
		settings.Exists = true;
		settings.Center = glm::vec3(world[3]);
		settings.Size = glm::max(glm::vec3(glm::length(glm::vec3(world[0])), glm::length(glm::vec3(world[1])), glm::length(glm::vec3(world[2]))),
			glm::vec3(0.1f));
		settings.Counts = settings.GetCounts();
		return entity;
	}

	glm::mat4 GetProbeVolumeTransform(const EnvMapVulkanProbeVolumeSettings& settings)
	{
		return glm::translate(glm::mat4(1.0f), settings.Center) * glm::scale(glm::mat4(1.0f), settings.Size);
	}

	void PlaceCamera(H2M::CameraH2M& camera, const TransformComponent& transform)
	{
		// The camera's yaw is measured from +X towards +Z (-90 looks along -Z)
		const float pitch = glm::clamp(transform.Rotation.x, -89.0f, 89.0f);
		const float yaw = -transform.Rotation.y - 90.0f;
		camera.SetPosition(transform.Translation);
		camera.SetYaw(yaw);
		camera.SetPitch(pitch);
		// As the camera's controller does from the yaw and the pitch (CameraController::CalculateFront: a positive pitch
		// looks down)
		camera.SetFront(glm::normalize(glm::vec3(std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch)),
			-std::sin(glm::radians(pitch)), std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch)))));
		camera.OnUpdate(H2M::TimestepH2M(0.0f));
	}
}
