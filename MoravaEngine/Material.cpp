#include "Material.h"

#include "TextureLoader.h"

#include <GL/glew.h>


Material::Material()
{
	m_SpecularIntensity = 0.0f;
	m_Shininess = 0.0f;

	m_Maps = std::map<int, MapType>();

	m_Ambient  = glm::vec3(0.0f);
	m_Diffuse  = glm::vec3(0.0f);
	m_Specular = glm::vec3(0.0f);

	m_AlbedoColor = glm::vec3(0.0f);

	m_AlbedoMap           = -1;
	m_MetalnessMap        = -1;
	m_RoughnessMap        = -1;
	m_NormalMap           = -1;
	m_HeightMap           = -1;
	m_AmbientOcclusionMap = -1;
	m_EmissionMap         = -1;

	m_TexturePlaceholder = TextureLoader::Get()->GetTexture("Textures/plain.png", false, false);
}

Material::Material(TextureInfo textureInfo, float specularIntensity, float shininess) : Material(specularIntensity, shininess)
{
	m_TextureAlbedo    = TextureLoader::Get()->GetTexture(textureInfo.albedo.c_str(), false, false);
	m_TextureNormal    = TextureLoader::Get()->GetTexture(textureInfo.normal.c_str(), false, false);
	m_TextureMetallic  = TextureLoader::Get()->GetTexture(textureInfo.metallic.c_str(), false, false);
	m_TextureRoughness = TextureLoader::Get()->GetTexture(textureInfo.roughness.c_str(), false, false);
	m_TextureAO        = TextureLoader::Get()->GetTexture(textureInfo.ao.c_str(), false, false);
}

Material::Material(float specularIntensity, float shininess) : Material()
{
	m_SpecularIntensity = specularIntensity;
	m_Shininess = shininess;
}

Material::Material(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess) : Material()
{
	m_Ambient = ambient;
	m_Diffuse = diffuse;
	m_Specular = specular;
	m_Shininess = shininess;
}

Material::Material(int txSlotAlbedo, int txSlotSpecular, int txSlotNormalMap, float shininess)
{
	m_AlbedoMap = txSlotAlbedo;
	m_SpecularMap = txSlotSpecular;
	m_NormalMap = txSlotNormalMap;
	m_Shininess = shininess;
}

void Material::BindTextures(unsigned int slot)
{
	glActiveTexture(GL_TEXTURE0 + slot + 0);
	if (m_TextureAlbedo != nullptr) {
		glBindTexture(GL_TEXTURE_2D, m_TextureAlbedo->GetID());
	}
	else {
		glBindTexture(GL_TEXTURE_2D, m_TexturePlaceholder->GetID());
	}

	glActiveTexture(GL_TEXTURE0 + slot + 1);
	if (m_TextureNormal != nullptr) {
		glBindTexture(GL_TEXTURE_2D, m_TextureNormal->GetID());
	}
	else {
		glBindTexture(GL_TEXTURE_2D, m_TexturePlaceholder->GetID());
	}

	glActiveTexture(GL_TEXTURE0 + slot + 2);
	if (m_TextureMetallic != nullptr) {
		glBindTexture(GL_TEXTURE_2D, m_TextureMetallic->GetID());
	}
	else {
		glBindTexture(GL_TEXTURE_2D, m_TexturePlaceholder->GetID());
	}

	glActiveTexture(GL_TEXTURE0 + slot + 3);
	if (m_TextureRoughness != nullptr) {
		glBindTexture(GL_TEXTURE_2D, m_TextureRoughness->GetID());
	}
	else {
		glBindTexture(GL_TEXTURE_2D, m_TexturePlaceholder->GetID());
	}

	glActiveTexture(GL_TEXTURE0 + slot + 4);
	if (m_TextureAO != nullptr) {
		glBindTexture(GL_TEXTURE_2D, m_TextureAO->GetID());
	}
	else {
		glBindTexture(GL_TEXTURE_2D, m_TexturePlaceholder->GetID());
	}
}

void Material::UseMaterial(int specularIntensityLocation, int shininessLocation)
{
	glUniform1f(specularIntensityLocation, m_SpecularIntensity);
	glUniform1f(shininessLocation, m_Shininess);
}

void Material::AddMap(MapType mapType, int textureSlot)
{
	m_Maps.insert(std::make_pair(textureSlot, mapType));
}

Material::~Material()
{
	m_Maps.clear();
}
