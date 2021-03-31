/**
 * Contains all relevant info about all materials loaded through Assimp or some other asset loader
 * Manages relations between materials and submeshes
 */

#pragma once

#include "Hazel/Core/Ref.h"
#include "Hazel/Renderer/HazelMaterial.h"

#include "EnvMap/EnvMapMaterial.h"
#include "EnvMap/EnvMapEditorLayer.h"

#include <vector>
#include <string>


struct MaterialData : public Hazel::RefCounted
{
	MaterialUUID UUID;
	std::string Name;
	Hazel::Ref<Hazel::HazelMaterial> Material;
	Hazel::Ref<EnvMapMaterial> EnvMapMaterial;

};

class MaterialLibrary {

public:

	static Hazel::Ref<MaterialData> AddNew();
	static void AddEnvMapMaterial(MaterialUUID UUID, Hazel::Ref<EnvMapMaterial> envMapMaterial);
	static void RenameMaterial(Hazel::Ref<EnvMapMaterial> envMapMaterial, std::string newName);
	static void AddSubmeshMaterialRelation(SubmeshUUID submeshUUID, MaterialUUID materialUUID);
	static void Cleanup();

public:
	static std::vector<Hazel::Ref<MaterialData>> s_MaterialData;
	static std::map<MaterialUUID, Hazel::Ref<EnvMapMaterial>> s_EnvMapMaterials;
	static std::map<SubmeshUUID, MaterialUUID> s_SubmeshMaterialUUIDs;

};
