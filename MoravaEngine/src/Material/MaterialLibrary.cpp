#include "MaterialLibrary.h"

#include "H2M/Scene/ComponentsH2M.h"

#include <map>


std::vector<H2M::RefH2M<MaterialData>> MaterialLibrary::s_MaterialData;
std::map<MaterialUUID, H2M::RefH2M<EnvMapMaterial>> MaterialLibrary::s_EnvMapMaterials;
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

H2M::RefH2M<MaterialData> MaterialLibrary::AddNewMaterial(std::string name)
{
    if (name == "") {
        name = NewMaterialName();
    }

	return CreateMaterialData(name, H2M::RefH2M<H2M::SubmeshH2M>());
}

H2M::RefH2M<MaterialData> MaterialLibrary::AddNewMaterial(H2M::RefH2M<H2M::MaterialH2M> material, H2M::RefH2M<H2M::SubmeshH2M> submesh)
{
    return CreateMaterialData(material->GetName(), submesh);
}

H2M::RefH2M<MaterialData> MaterialLibrary::CreateMaterialData(std::string name, H2M::RefH2M<H2M::SubmeshH2M> submesh)
{
    H2M::RefH2M<MaterialData> materialData = H2M::RefH2M<MaterialData>::Create();

    // If material is "DefaultMaterial", try to assign more meaningful name from submesh info
    if ((
        name == "DefaultMaterial" ||
        name == "Material" ||
        name == ""
        ) && submesh)
    {
        materialData->Name = Util::SpaceToUnderscore(submesh->NodeName) + "__" + Util::SpaceToUnderscore(submesh->MeshName);
    }
    else {
        materialData->Name = name;
    }

    materialData->Submesh = submesh;
    s_MaterialData.push_back(materialData);

    H2M::RefH2M<EnvMapMaterial> defaultEnvMapMaterial = MaterialLibrary::CreateDefaultMaterial(materialData->Name);
    AddEnvMapMaterial(defaultEnvMapMaterial->GetUUID(), defaultEnvMapMaterial);
    materialData->EnvMapMaterialRef = defaultEnvMapMaterial;

    return materialData;
}

void MaterialLibrary::AddEnvMapMaterial(MaterialUUID UUID, H2M::RefH2M<EnvMapMaterial> envMapMaterial)
{
	if (s_EnvMapMaterials.find(envMapMaterial->GetName()) != s_EnvMapMaterials.end())
	{
		Log::GetLogger()->warn("Material name '{0}' already taken!", envMapMaterial->GetName());
		return;
	}

	s_EnvMapMaterials.insert(std::make_pair(UUID, envMapMaterial));
}

void MaterialLibrary::AddTextureToEnvMapMaterial(MaterialTextureType textureType, const std::string& texturePath, H2M::RefH2M<EnvMapMaterial> envMapMaterial)
{
    switch (textureType)
    {
    case MaterialTextureType::Albedo:
        envMapMaterial->GetAlbedoInput().TextureMap = ResourceManager::LoadTexture2D_H2M(texturePath, true);
        envMapMaterial->GetAlbedoInput().UseTexture = true;
        break;
    case MaterialTextureType::Normal:
        envMapMaterial->GetNormalInput().TextureMap = ResourceManager::LoadTexture2D_H2M(texturePath, false);
        envMapMaterial->GetNormalInput().UseTexture = true;
        break;
    case MaterialTextureType::Metalness:
        envMapMaterial->GetMetalnessInput().TextureMap = ResourceManager::LoadTexture2D_H2M(texturePath, false);
        envMapMaterial->GetMetalnessInput().UseTexture = true;
        break;
    case MaterialTextureType::Roughness:
        envMapMaterial->GetRoughnessInput().TextureMap = ResourceManager::LoadTexture2D_H2M(texturePath, false);
        envMapMaterial->GetRoughnessInput().UseTexture = true;
        break;
    case MaterialTextureType::AO:
        envMapMaterial->GetAOInput().TextureMap = ResourceManager::LoadTexture2D_H2M(texturePath, false);
        envMapMaterial->GetAOInput().UseTexture = true;
        break;
    case MaterialTextureType::Emissive:
        envMapMaterial->GetEmissiveInput().TextureMap = ResourceManager::LoadTexture2D_H2M(texturePath, false);
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

void MaterialLibrary::RenameMaterial(H2M::RefH2M<EnvMapMaterial> envMapMaterial, std::string newName)
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
    if (s_SubmeshMaterialUUIDs.find(submeshUUID) == s_SubmeshMaterialUUIDs.end())
    {
        s_SubmeshMaterialUUIDs.insert(std::make_pair(submeshUUID, materialUUID));
    }
    else {
        // Do we really want to overwrite the existing submesh/material relation?
        // s_SubmeshMaterialUUIDs.find(submeshUUID)->second = materialUUID;
    }
}

H2M::RefH2M<EnvMapMaterial> MaterialLibrary::CreateDefaultMaterial(std::string materialName)
{
    H2M::RefH2M<EnvMapMaterial> envMapMaterial = H2M::RefH2M<EnvMapMaterial>::Create(materialName);

    TextureInfo textureInfo;
    if (s_TextureInfo.find(materialName) != s_TextureInfo.end()) {
        textureInfo = s_TextureInfo.at(materialName);
    }
    else {
        textureInfo = s_TextureInfoDefault;
    }

    // Load Hazel/Renderer/HazelTexture
    if (textureInfo.albedo != "") {
        envMapMaterial->GetAlbedoInput().TextureMap = ResourceManager::LoadTexture2D_H2M(textureInfo.albedo, true);
        envMapMaterial->GetAlbedoInput().UseTexture = true;
    }
    if (textureInfo.normal != "")
    {
        envMapMaterial->GetNormalInput().TextureMap = ResourceManager::LoadTexture2D_H2M(textureInfo.normal, false);
        envMapMaterial->GetNormalInput().UseTexture = true;    
    }
    if (textureInfo.metallic != "")
    {
        envMapMaterial->GetMetalnessInput().TextureMap = ResourceManager::LoadTexture2D_H2M(textureInfo.metallic, false);
        envMapMaterial->GetMetalnessInput().UseTexture = true;
    }
    if (textureInfo.roughness != "")
    {
        envMapMaterial->GetRoughnessInput().TextureMap = ResourceManager::LoadTexture2D_H2M(textureInfo.roughness, false);
        envMapMaterial->GetRoughnessInput().UseTexture = true;
    }
    if (textureInfo.emissive != "")
    {
        envMapMaterial->GetEmissiveInput().TextureMap = ResourceManager::LoadTexture2D_H2M(textureInfo.emissive, false);
        envMapMaterial->GetEmissiveInput().UseTexture = true;
    }
    if (textureInfo.ao != "")
    {
        envMapMaterial->GetAOInput().TextureMap = ResourceManager::LoadTexture2D_H2M(textureInfo.ao, false);
        envMapMaterial->GetAOInput().UseTexture = true;
    }

    return envMapMaterial;
}

void MaterialLibrary::LoadEnvMapMaterials(H2M::RefH2M<H2M::MeshH2M> mesh, H2M::EntityH2M entity)
{
    //  for (auto material : m_EnvMapMaterials) {
    //      delete material.second;
    //  }
    //  
    //  m_EnvMapMaterials.clear();

    std::vector<H2M::RefH2M<H2M::SubmeshH2M>>& submeshes = mesh->GetSubmeshes();

    for (H2M::RefH2M<H2M::SubmeshH2M> submesh : submeshes)
    {
        std::string materialUUID = GetSubmeshMaterialUUID(mesh, submesh, entity);

        Log::GetLogger()->debug("EnvMapEditorLayer::LoadEnvMapMaterials materialUUID = '{0}'", materialUUID);

        if (MaterialLibrary::s_EnvMapMaterials.find(materialUUID) != MaterialLibrary::s_EnvMapMaterials.end()) {
            continue;
        }

        H2M::RefH2M<EnvMapMaterial> envMapMaterial = MaterialLibrary::CreateDefaultMaterial(materialUUID);
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

SubmeshUUID MaterialLibrary::GetSubmeshUUID(H2M::EntityH2M entity, H2M::RefH2M<H2M::SubmeshH2M> submesh)
{
    std::string entityHandle = entity ? std::to_string(entity.GetHandle()) : "0000";
    SubmeshUUID submeshUUID = "E_" + entityHandle + "_S_" + submesh->MeshName;
    // Log::GetLogger()->debug("EnvMapEditorLayer::GetSubmeshUUID: '{0}'", submeshUUID);
    return submeshUUID;
}

void MaterialLibrary::SetDefaultMaterialToSubmeshes(H2M::RefH2M<H2M::MeshH2M> mesh, H2M::EntityH2M entity, H2M::RefH2M<EnvMapMaterial> defaultMaterial)
{
    if (!defaultMaterial)
    {
        Log::GetLogger()->error("MaterialLibrary::SetDefaultMaterialToSubmeshes: defaultMaterial is undefined!");
        return;
    }

    for (auto submesh : mesh->GetSubmeshes())
    {
        SubmeshUUID submeshUUID = GetSubmeshUUID(entity, submesh);
        MaterialUUID materialUUID = defaultMaterial->GetUUID();
        MaterialLibrary::AddSubmeshMaterialRelation(submeshUUID, materialUUID);
    }
}

/****
 * The more intelligent version of SetDefaultMaterialToSubmeshes
 * Instead of just assigning the default material to each submesh, this method tries to detect the correct material and assign to the submesh
 * If it fails to do so, it loads the default material
 */
void MaterialLibrary::SetMaterialsToSubmeshes(H2M::RefH2M<H2M::MeshH2M> mesh, H2M::EntityH2M entity, H2M::RefH2M<EnvMapMaterial> defaultMaterial)
{
    for (H2M::RefH2M<H2M::SubmeshH2M> submesh : mesh->GetSubmeshes())
    {
        bool correctMaterialFound = false;
        // Let's try to detect a correct material from the list of loaded materials in MaterialLibrary
        for (auto materialData : s_MaterialData)
        {
            if (materialData->Submesh && materialData->EnvMapMaterialRef)
            {
                if (submesh->MeshName == materialData->Submesh->MeshName)
                {
                    SubmeshUUID submeshUUID = GetSubmeshUUID(entity, submesh);
                    MaterialUUID materialUUID = materialData->EnvMapMaterialRef->GetUUID();
                    MaterialLibrary::AddSubmeshMaterialRelation(submeshUUID, materialUUID);
                    correctMaterialFound = true;
                    break;
                }
            }
        }

        // If we fail to find a correct material for the submesh, fallback to default material
        if (!correctMaterialFound) {
            SetDefaultMaterialToSubmeshes(mesh, entity, defaultMaterial);
        }
    }
}

std::string MaterialLibrary::NewMaterialName()
{
    std::string materialName = "MAT_" + std::to_string(s_MaterialIndex);
    s_MaterialIndex++;

    return materialName;
}

void MaterialLibrary::AddMaterialFromComponent(H2M::EntityH2M entity)
{
    // If entity contains MaterialComponent, load generic material for the entire entity (all submeshes)
    if (entity.HasComponent<H2M::MaterialComponentH2M>())
    {
        if (entity.GetComponent<H2M::MaterialComponentH2M>().Material)
        {
            H2M::RefH2M<MaterialData> newMaterialData = AddNewMaterial("");
            auto material = entity.GetComponent<H2M::MaterialComponentH2M>().Material;
            material->SetName(newMaterialData->Name);
        }
    }
}

MaterialUUID MaterialLibrary::GetSubmeshMaterialUUID(H2M::RefH2M<H2M::MeshH2M> mesh, H2M::RefH2M<H2M::SubmeshH2M> submesh, H2M::EntityH2M entity)
{
    MaterialUUID materialUUID = "";

    EnvMapMaterial* envMapMaterial = nullptr;
    bool hasMaterialComponent = entity && entity.HasComponent<H2M::MaterialComponentH2M>();
    if (hasMaterialComponent) {
        H2M::MaterialComponentH2M materialComponent = entity.GetComponent<H2M::MaterialComponentH2M>();
        H2M::RefH2M<EnvMapMaterial> envMapMaterial = materialComponent.Material;
    }

    std::string submeshUUID = GetSubmeshUUID(entity, submesh);

    if (s_SubmeshMaterialUUIDs.find(submeshUUID) != s_SubmeshMaterialUUIDs.end()) {
        materialUUID = s_SubmeshMaterialUUIDs.at(submeshUUID);
    }
    else if (hasMaterialComponent && envMapMaterial) {
        materialUUID = envMapMaterial->GetUUID();
    }
    else {
        std::string meshName = Util::StripExtensionFromFileName(Util::GetFileNameFromFullPath(mesh->GetFilePath()));
        materialUUID = EnvMapMaterial::NewMaterialUUID();
    }

    return materialUUID;
}
