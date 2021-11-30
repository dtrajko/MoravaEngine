#define _CRT_SECURE_NO_WARNINGS

#pragma once

/**
 * Contains all relevant info about all materials loaded through Assimp or some other asset loader
 * Manages relations between materials and submeshes
 */

#include "H2M/Core/Ref.h"
#include "H2M/Renderer/HazelMaterial.h"

#include "EnvMap/EnvMapMaterial.h"
#include "EnvMap/EnvMapEditorLayer.h"
#include "H2M/Renderer/MeshH2M.h"

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

struct MaterialData : public H2M::RefCounted
{
	MaterialUUID UUID;
	std::string Name;
	H2M::Ref<H2M::HazelMaterial> Material;
	H2M::Ref<EnvMapMaterial> EnvMapMaterialRef;
	H2M::Ref<H2M::SubmeshH2M> Submesh;

};

class MaterialLibrary {

public:

	static void Init();
	static H2M::Ref<MaterialData> AddNewMaterial(std::string name = "");
	static H2M::Ref<MaterialData> AddNewMaterial(H2M::Ref<H2M::HazelMaterial> material, H2M::Ref<H2M::SubmeshH2M> submesh);
	static void RenameMaterial(H2M::Ref<EnvMapMaterial> envMapMaterial, std::string newName);
	static void LoadEnvMapMaterials(H2M::Ref<H2M::MeshH2M> mesh, H2M::EntityH2M entity);
	static H2M::Ref<EnvMapMaterial> CreateDefaultMaterial(std::string materialName);
	static void AddEnvMapMaterial(MaterialUUID UUID, H2M::Ref<EnvMapMaterial> envMapMaterial);
	static std::string NewMaterialName();
	static SubmeshUUID GetSubmeshUUID(H2M::EntityH2M entity, H2M::Ref<H2M::SubmeshH2M> submesh);
	static void SetDefaultMaterialToSubmeshes(H2M::Ref<H2M::MeshH2M> mesh, H2M::EntityH2M entity, H2M::Ref<EnvMapMaterial> defaultMaterial);
	static void SetMaterialsToSubmeshes(H2M::Ref<H2M::MeshH2M> mesh, H2M::EntityH2M entity, H2M::Ref<EnvMapMaterial> defaultMaterial);
	static void AddMaterialFromComponent(H2M::EntityH2M entity);
	static void AddTextureToEnvMapMaterial(MaterialTextureType textureType, const std::string& texturePath, H2M::Ref<EnvMapMaterial> envMapMaterial);
	static MaterialUUID GetSubmeshMaterialUUID(H2M::Ref<H2M::MeshH2M> mesh, H2M::Ref<H2M::SubmeshH2M> submesh, H2M::EntityH2M entity);
	static void Cleanup();

private:
	static H2M::Ref<MaterialData> CreateMaterialData(std::string name, H2M::Ref<H2M::SubmeshH2M> submesh);
	static void AddSubmeshMaterialRelation(SubmeshUUID submeshUUID, MaterialUUID materialUUID);

public:
	static std::vector<H2M::Ref<MaterialData>> s_MaterialData;
	static std::map<MaterialUUID, H2M::Ref<EnvMapMaterial>> s_EnvMapMaterials;
	static std::map<SubmeshUUID, MaterialUUID> s_SubmeshMaterialUUIDs;
	static TextureInfo s_TextureInfoDefault;
	static std::map<std::string, TextureInfo> s_TextureInfo;
	static uint32_t s_MaterialIndex;

};
