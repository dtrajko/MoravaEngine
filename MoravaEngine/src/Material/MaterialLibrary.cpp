#include "MaterialLibrary.h"

#include <map>


std::vector<Hazel::Ref<MaterialData>> MaterialLibrary::s_MaterialData;
std::map<MaterialUUID, Hazel::Ref<EnvMapMaterial>> MaterialLibrary::s_EnvMapMaterials;
std::map<SubmeshUUID, MaterialUUID> MaterialLibrary::s_SubmeshMaterialUUIDs;
TextureInfo MaterialLibrary::s_TextureInfoDefault;
std::map<std::string, TextureInfo> MaterialLibrary::s_TextureInfo;
uint32_t MaterialLibrary::s_MaterialIndex = 0;


void MaterialLibrary::Init()
{
    // Setup default texture info
    s_TextureInfoDefault = {};
    s_TextureInfoDefault.albedo    = "Textures/default_material_albedo.png";
    s_TextureInfoDefault.normal    = "Textures/PBR/non_reflective/normal.png";
    s_TextureInfoDefault.metallic  = "Textures/PBR/non_reflective/metallic.png";
    s_TextureInfoDefault.roughness = "Textures/PBR/non_reflective/roughness.png";
    s_TextureInfoDefault.emissive  = "Textures/PBR/non_reflective/emissive.png";
    s_TextureInfoDefault.ao        = "Textures/PBR/non_reflective/ao.png";
}

Hazel::Ref<MaterialData> MaterialLibrary::AddNewMaterial(std::string name)
{
    if (name == "") {
        NewMaterialName();
    }

	return CreateMaterialData(name);
}

Hazel::Ref<MaterialData> MaterialLibrary::AddNewMaterial(Hazel::Ref<Hazel::HazelMaterial> material)
{
    return CreateMaterialData(material->GetName());
}

Hazel::Ref<MaterialData> MaterialLibrary::CreateMaterialData(std::string name)
{
    Hazel::Ref<MaterialData> materialData = Hazel::Ref<MaterialData>::Create();
    materialData->Name = name;
    s_MaterialData.push_back(materialData);

    Hazel::Ref<EnvMapMaterial> defaultEnvMapMaterial = MaterialLibrary::CreateDefaultMaterial(materialData->Name);
    AddEnvMapMaterial(defaultEnvMapMaterial->GetUUID(), defaultEnvMapMaterial);
    materialData->EnvMapMaterial = defaultEnvMapMaterial;

    return materialData;
}

void MaterialLibrary::AddEnvMapMaterial(MaterialUUID UUID, Hazel::Ref<EnvMapMaterial> envMapMaterial)
{
	if (s_EnvMapMaterials.find(envMapMaterial->GetName()) != s_EnvMapMaterials.end())
	{
		Log::GetLogger()->warn("Material name '{0}' already taken!", envMapMaterial->GetName());
		return;
	}

	s_EnvMapMaterials.insert(std::make_pair(UUID, envMapMaterial));
}

void MaterialLibrary::AddTextureToEnvMapMaterial(MaterialTextureType textureType, const std::string& texturePath, Hazel::Ref<EnvMapMaterial> envMapMaterial)
{
    switch (textureType)
    {
    case MaterialTextureType::Albedo:
        envMapMaterial->GetAlbedoInput().TextureMap = ResourceManager::LoadHazelTexture2D(texturePath);
        envMapMaterial->GetAlbedoInput().UseTexture = true;
        break;
    case MaterialTextureType::Normal:
        envMapMaterial->GetNormalInput().TextureMap = ResourceManager::LoadHazelTexture2D(texturePath);
        envMapMaterial->GetNormalInput().UseTexture = true;
        break;
    case MaterialTextureType::Metalness:
        envMapMaterial->GetMetalnessInput().TextureMap = ResourceManager::LoadHazelTexture2D(texturePath);
        envMapMaterial->GetMetalnessInput().UseTexture = true;
        break;
    case MaterialTextureType::Roughness:
        envMapMaterial->GetRoughnessInput().TextureMap = ResourceManager::LoadHazelTexture2D(texturePath);
        envMapMaterial->GetRoughnessInput().UseTexture = true;
        break;
    case MaterialTextureType::AO:
        envMapMaterial->GetAOInput().TextureMap = ResourceManager::LoadHazelTexture2D(texturePath);
        envMapMaterial->GetAOInput().UseTexture = true;
        break;
    case MaterialTextureType::Emissive:
        envMapMaterial->GetEmissiveInput().TextureMap = ResourceManager::LoadHazelTexture2D(texturePath);
        envMapMaterial->GetEmissiveInput().UseTexture = true;
        break;
    }
}

void MaterialLibrary::Cleanup()
{
	s_EnvMapMaterials.clear();
	s_MaterialData.clear();
	s_SubmeshMaterialUUIDs.clear();
}

void MaterialLibrary::RenameMaterial(Hazel::Ref<EnvMapMaterial> envMapMaterial, std::string newName)
{
    Log::GetLogger()->debug("MaterialLibrary::RenameMaterial from '{0}' to '{1}'", envMapMaterial->GetName(), newName);

    std::string oldName = envMapMaterial->GetName();
    MaterialUUID materialUUID = envMapMaterial->GetUUID();

    // TODO: Make sure that the new material name is not already taken
    for (auto emm_it = s_EnvMapMaterials.begin(); emm_it != s_EnvMapMaterials.end(); emm_it++) {
        if (emm_it->second->GetName() == newName) {
            Log::GetLogger()->error("Material name is already taken [Name: '{0}', UUID: '{1}']!", newName, emm_it->second->GetUUID());
            return;
        }
    }

    // TODO: Rename object attribute
    envMapMaterial->SetName(newName);

    // TODO: Rename in s_EnvMapMaterials
    for (auto emm_it = s_EnvMapMaterials.begin(); emm_it != s_EnvMapMaterials.end();) {
        if (emm_it->second->GetUUID() == materialUUID) {
            emm_it->second->SetName(newName);
            Log::GetLogger()->error("s_EnvMapMaterials: '{0}' => '{1}'", oldName, newName);
            break;
        }
        else {
            ++emm_it;
        }
    }
}

void MaterialLibrary::AddSubmeshMaterialRelation(SubmeshUUID submeshUUID, MaterialUUID materialUUID)
{
    s_SubmeshMaterialUUIDs.insert(std::make_pair(submeshUUID, materialUUID));
}

Hazel::Ref<EnvMapMaterial> MaterialLibrary::CreateDefaultMaterial(std::string materialName)
{
    Hazel::Ref<EnvMapMaterial> envMapMaterial = Hazel::Ref<EnvMapMaterial>::Create(materialName);

    TextureInfo textureInfo;
    if (s_TextureInfo.find(materialName) != s_TextureInfo.end()) {
        textureInfo = s_TextureInfo.at(materialName);
    }
    else {
        textureInfo = s_TextureInfoDefault;
    }

    // Load Hazel/Renderer/HazelTexture
    envMapMaterial->GetAlbedoInput().TextureMap = ResourceManager::LoadHazelTexture2D(textureInfo.albedo);
    envMapMaterial->GetAlbedoInput().UseTexture = true;
    envMapMaterial->GetNormalInput().TextureMap = ResourceManager::LoadHazelTexture2D(textureInfo.normal);
    envMapMaterial->GetNormalInput().UseTexture = true;
    envMapMaterial->GetMetalnessInput().TextureMap = ResourceManager::LoadHazelTexture2D(textureInfo.metallic);
    envMapMaterial->GetMetalnessInput().UseTexture = true;
    envMapMaterial->GetRoughnessInput().TextureMap = ResourceManager::LoadHazelTexture2D(textureInfo.roughness);
    envMapMaterial->GetRoughnessInput().UseTexture = true;
    envMapMaterial->GetEmissiveInput().TextureMap = ResourceManager::LoadHazelTexture2D(textureInfo.emissive);
    envMapMaterial->GetEmissiveInput().UseTexture = true;
    envMapMaterial->GetAOInput().TextureMap = ResourceManager::LoadHazelTexture2D(textureInfo.ao);
    envMapMaterial->GetAOInput().UseTexture = true;

    return envMapMaterial;
}

void MaterialLibrary::LoadEnvMapMaterials(Hazel::Ref<Hazel::HazelMesh> mesh, Hazel::Entity entity)
{
    //  for (auto material : m_EnvMapMaterials) {
    //      delete material.second;
    //  }
    //  
    //  m_EnvMapMaterials.clear();

    std::vector<Hazel::Submesh>& submeshes = mesh->GetSubmeshes();

    for (Hazel::Submesh& submesh : submeshes)
    {
        std::string materialUUID = Hazel::HazelMesh::GetSubmeshMaterialUUID(mesh, submesh, &entity);

        Log::GetLogger()->debug("EnvMapEditorLayer::LoadEnvMapMaterials materialUUID = '{0}'", materialUUID);

        if (MaterialLibrary::s_EnvMapMaterials.find(materialUUID) != MaterialLibrary::s_EnvMapMaterials.end()) {
            continue;
        }

        Hazel::Ref<EnvMapMaterial> envMapMaterial = MaterialLibrary::CreateDefaultMaterial(materialUUID);
        MaterialLibrary::AddEnvMapMaterial(materialUUID, envMapMaterial);
    }

    //  // If no submeshes, add a default material for entity
    //  if (submeshes.empty())
    //  {
    //      EnvMapMaterial* envMapMaterial = CreateDefaultMaterial(meshName);
    //      m_EnvMapMaterials.insert(std::make_pair(meshName, envMapMaterial));
    //  }

    for (auto& material : MaterialLibrary::s_EnvMapMaterials)
    {
        Log::GetLogger()->debug("EnvMapEditorLayer::LoadEnvMapMaterials material name: '{0}' UUID: '{1}'", material.second->GetName(), material.first);
    }
}

SubmeshUUID MaterialLibrary::GetSubmeshUUID(Hazel::Entity* entity, Hazel::Submesh* submesh)
{
    std::string entityHandle = entity ? std::to_string(entity->GetHandle()) : "0000";
    SubmeshUUID submeshUUID = "E_" + entityHandle + "_S_" + submesh->MeshName;
    // Log::GetLogger()->debug("EnvMapEditorLayer::GetSubmeshUUID: '{0}'", submeshUUID);
    return submeshUUID;
}

void MaterialLibrary::SetDefaultMaterialToSubmeshes(Hazel::Ref<Hazel::HazelMesh> mesh, Hazel::Entity entity, Hazel::Ref<EnvMapMaterial> defaultMaterial)
{
    for (auto submesh : mesh->GetSubmeshes())
    {
        SubmeshUUID submeshUUID = GetSubmeshUUID(&entity, &submesh);
        MaterialUUID materialUUID = defaultMaterial->GetUUID();
        MaterialLibrary::AddSubmeshMaterialRelation(submeshUUID, materialUUID);
    }
}

std::string MaterialLibrary::NewMaterialName()
{
    std::string materialName = "MAT_" + std::to_string(s_MaterialIndex);
    s_MaterialIndex++;

    return materialName;
}

void MaterialLibrary::AddMaterialFromComponent(Hazel::Entity entity)
{
    // If entity contains MaterialComponent, load generic material for the entire entity (all submeshes)
    if (entity.HasComponent<Hazel::MaterialComponent>())
    {
        if (entity.GetComponent<Hazel::MaterialComponent>().Material)
        {
            Hazel::Ref<MaterialData> newMaterialData = AddNewMaterial("");
            auto material = entity.GetComponent<Hazel::MaterialComponent>().Material;
            material->SetName(newMaterialData->Name);
        }
    }
}
