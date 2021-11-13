#define _CRT_SECURE_NO_WARNINGS

#pragma once

/**
 * Contains all relevant info about all materials loaded through Assimp or some other asset loader
 * Manages relations between materials and submeshes
 */

#include "Hazel/Core/Ref.h"
#include "Hazel/Renderer/HazelMaterial.h"

#include "EnvMap/EnvMapMaterial.h"
#include "EnvMap/EnvMapEditorLayer.h"
#include "HazelLegacy/Renderer/MeshHazelLegacy.h"

#include <vector>
#include <string>


enum class MaterialTextureType {
	Albedo,
	Normal,
	Metalness,
	Roughness,
	AO,
	Emissive,
};

struct MaterialData : public Hazel::RefCounted
{
	MaterialUUID UUID;
	std::string Name;
	Hazel::Ref<Hazel::HazelMaterial> Material;
	Hazel::Ref<EnvMapMaterial> EnvMapMaterialRef;
	Hazel::Ref < Hazel::SubmeshHazelLegacy> Submesh;
};

class MaterialLibrary {

public:

	static void Init();
	static Hazel::Ref<MaterialData> AddNewMaterial(std::string name = "");
	static Hazel::Ref<MaterialData> AddNewMaterial(Hazel::Ref<Hazel::HazelMaterial> material, Hazel::Ref<Hazel::SubmeshHazelLegacy> submesh);
	static void RenameMaterial(Hazel::Ref<EnvMapMaterial> envMapMaterial, std::string newName);
	static void LoadEnvMapMaterials(Hazel::Ref<Hazel::MeshHazelLegacy> mesh, Hazel::EntityHazelLegacy entity);
	static Hazel::Ref<EnvMapMaterial> CreateDefaultMaterial(std::string materialName);
	static void AddEnvMapMaterial(MaterialUUID UUID, Hazel::Ref<EnvMapMaterial> envMapMaterial);
	static std::string NewMaterialName();
	static SubmeshUUID GetSubmeshUUID(Hazel::EntityHazelLegacy* entity, Hazel::Ref<Hazel::SubmeshHazelLegacy> submesh);
	static void SetDefaultMaterialToSubmeshes(Hazel::Ref<Hazel::MeshHazelLegacy> mesh, Hazel::EntityHazelLegacy entity, Hazel::Ref<EnvMapMaterial> defaultMaterial);
	static void SetMaterialsToSubmeshes(Hazel::Ref<Hazel::MeshHazelLegacy> mesh, Hazel::EntityHazelLegacy entity, Hazel::Ref<EnvMapMaterial> defaultMaterial);
	static void AddMaterialFromComponent(Hazel::EntityHazelLegacy entity);
	static void AddTextureToEnvMapMaterial(MaterialTextureType textureType, const std::string& texturePath, Hazel::Ref<EnvMapMaterial> envMapMaterial);
	static MaterialUUID GetSubmeshMaterialUUID(Hazel::Ref<Hazel::MeshHazelLegacy> mesh, Hazel::Ref<Hazel::SubmeshHazelLegacy> submesh, Hazel::EntityHazelLegacy* entity);
	static void Cleanup();

private:
	static Hazel::Ref<MaterialData> CreateMaterialData(std::string name, Hazel::Ref<Hazel::SubmeshHazelLegacy> submesh);
	static void AddSubmeshMaterialRelation(SubmeshUUID submeshUUID, MaterialUUID materialUUID);

public:
	static std::vector<Hazel::Ref<MaterialData>> s_MaterialData;
	static std::map<MaterialUUID, Hazel::Ref<EnvMapMaterial>> s_EnvMapMaterials;
	static std::map<SubmeshUUID, MaterialUUID> s_SubmeshMaterialUUIDs;
	static TextureInfo s_TextureInfoDefault;
	static std::map<std::string, TextureInfo> s_TextureInfo;
	static uint32_t s_MaterialIndex;

};
