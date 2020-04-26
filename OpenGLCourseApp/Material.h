#pragma once

#include <glm/glm.hpp>

#include <map>


enum class MapType
{
	Albedo           =  0,
	Normal           =  1,
	Specular         =  2,
	Metalness        =  3,
	Roughness        =  4,
	AmbientOcclusion =  5,
	Height           =  6,
	Emission         =  7,
};


class Material
{
public:
	Material();
	Material(float specularIntensity, float shininess);
	Material(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess); // based on LearnOpenGL material classint 
	Material(int albedo, int specular, int normal, float shininess); // used in SceneNanosuit
	void UseMaterial(int specularIntensityLocation, int shininessLocation);
	void AddMap(MapType mapType, int textureSlot);
	~Material();

public:
	int m_AlbedoMap;      // sampler2D, texture slot - diffuse/albedo
	int m_SpecularMap;    // sampler2D, texture slot
	int m_NormalMap;      // sampler2D, texture slot
	float m_Shininess;

private:
	std::map<int, MapType> m_Maps;

	glm::vec3 m_Ambient;  // color value
	glm::vec3 m_Diffuse;  // color value
	glm::vec3 m_Specular; // color value

	glm::vec3 m_AlbedoColor;   // diffuse/albedo color

	int m_MetalnessMap;        // sampler2D, texture slot
	int m_RoughnessMap;        // sampler2D, texture slot
	int m_HeightMap;           // sampler2D, texture slot
	int m_AmbientOcclusionMap; // sampler2D, texture slot
	int m_EmissionMap;         // sampler2D, texture slot

	float m_SpecularIntensity;
};
