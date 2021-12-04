#define _CRT_SECURE_NO_WARNINGS

#pragma once

/**
 * Contains all relevant info about all materials loaded through Assimp or some other asset loader
 * Manages relations between materials and submeshes
 */

#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/MaterialH2M.h"

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

struct MaterialData : public H2M::RefCountedH2M
{
	MaterialUUID UUID;
	std::string Name;
	H2M::RefH2M<H2M::MaterialH2M> Material;
	H2M::RefH2M<EnvMapMaterial> EnvMapMaterialRef;
	H2M::RefH2M<H2M::SubmeshH2M> Submesh;

};

class MaterialLibrary {

public:

	static void Init();
	static H2M::RefH2M<MaterialData> AddNewMaterial(std::string name = "");
	static H2M::RefH2M<MaterialData> AddNewMaterial(H2M::RefH2M<H2M::MaterialH2M> material, H2M::RefH2M<H2M::SubmeshH2M> submesh);
	static void RenameMaterial(H2M::RefH2M<EnvMapMaterial> envMapMaterial, std::string newName);
	static void LoadEnvMapMaterials(H2M::RefH2M<H2M::MeshH2M> mesh, H2M::EntityH2M entity);
	static H2M::RefH2M<EnvMapMaterial> CreateDefaultMaterial(std::string materialName);
	static void AddEnvMapMaterial(MaterialUUID UUID, H2M::RefH2M<EnvMapMaterial> envMapMaterial);
	static std::string NewMaterialName();
	static SubmeshUUID GetSubmeshUUID(H2M::EntityH2M entity, H2M::RefH2M<H2M::SubmeshH2M> submesh);
	static void SetDefaultMaterialToSubmeshes(H2M::RefH2M<H2M::MeshH2M> mesh, H2M::EntityH2M entity, H2M::RefH2M<EnvMapMaterial> defaultMaterial);
	static void SetMaterialsToSubmeshes(H2M::RefH2M<H2M::MeshH2M> mesh, H2M::EntityH2M entity, H2M::RefH2M<EnvMapMaterial> defaultMaterial);
	static void AddMaterialFromComponent(H2M::EntityH2M entity);
	static void AddTextureToEnvMapMaterial(MaterialTextureType textureType, const std::string& texturePath, H2M::RefH2M<EnvMapMaterial> envMapMaterial);
	static MaterialUUID GetSubmeshMaterialUUID(H2M::RefH2M<H2M::MeshH2M> mesh, H2M::RefH2M<H2M::SubmeshH2M> submesh, H2M::EntityH2M entity);
	static void Cleanup();

private:
	static H2M::RefH2M<MaterialData> CreateMaterialData(std::string name, H2M::RefH2M<H2M::SubmeshH2M> submesh);
	static void AddSubmeshMaterialRelation(SubmeshUUID submeshUUID, MaterialUUID materialUUID);

public:
	static std::vector<H2M::RefH2M<MaterialData>> s_MaterialData;
	static std::map<MaterialUUID, H2M::RefH2M<EnvMapMaterial>> s_EnvMapMaterials;
	static std::map<SubmeshUUID, MaterialUUID> s_SubmeshMaterialUUIDs;
	static TextureInfo s_TextureInfoDefault;
	static std::map<std::string, TextureInfo> s_TextureInfo;
	static uint32_t s_MaterialIndex;

};
