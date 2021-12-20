#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/TextureH2M.h"

#include <glm/glm.hpp>


using MaterialUUID = std::string;

class EnvMapMaterial : public H2M::RefCountedH2M
{
	struct AlbedoInput;
	struct NormalInput;
	struct MetalnessInput;
	struct RoughnessInput;
	struct EmissiveInput;
	struct AOInput;

public:
	EnvMapMaterial();
	EnvMapMaterial(std::string name = "Untitled");
	EnvMapMaterial(std::string name, H2M::RefH2M<EnvMapMaterial> other);
	~EnvMapMaterial() = default;

	inline AlbedoInput& GetAlbedoInput() { return m_AlbedoInput; }
	inline NormalInput& GetNormalInput() { return m_NormalInput; }
	inline MetalnessInput& GetMetalnessInput() { return m_MetalnessInput; }
	inline RoughnessInput& GetRoughnessInput() { return m_RoughnessInput; }
	inline EmissiveInput& GetEmissiveInput() { return m_EmissiveInput; }
	inline AOInput& GetAOInput() { return m_AOInput; }

	inline float& GetTilingFactor() { return m_TilingFactor; }

	inline void SetName(std::string name) { m_Name = name; };
	inline std::string GetName() { return m_Name; };

	inline MaterialUUID GetUUID() { return m_UUID; };

	static MaterialUUID NewMaterialUUID();

private:

	/**** BEGIN properties EditorLayer ****/
	struct AlbedoInput
	{
		glm::vec3 Color = { 0.972f, 0.96f, 0.915f }; // Silver, from https://docs.unrealengine.com/en-us/Engine/Rendering/Materials/PhysicallyBased
		H2M::RefH2M<H2M::Texture2D_H2M> TextureMap;
		bool SRGB = true;
		bool UseTexture = false;
	};
	AlbedoInput m_AlbedoInput;

	struct NormalInput
	{
		H2M::RefH2M<H2M::Texture2D_H2M> TextureMap;
		bool UseTexture = false;
	};
	NormalInput m_NormalInput;

	struct MetalnessInput
	{
		float Value = 0.0f;
		H2M::RefH2M<H2M::Texture2D_H2M> TextureMap;
		bool UseTexture = false;
	};
	MetalnessInput m_MetalnessInput;

	struct RoughnessInput
	{
		float Value = 1.0f;
		H2M::RefH2M<H2M::Texture2D_H2M> TextureMap;
		bool UseTexture = false;
	};
	RoughnessInput m_RoughnessInput;

	struct EmissiveInput
	{
		float Value = 0.0f;
		H2M::RefH2M<H2M::Texture2D_H2M> TextureMap;
		bool SRGB = true;
		bool UseTexture = false;
	};
	EmissiveInput m_EmissiveInput;

	struct AOInput
	{
		float Value = 1.0f;
		H2M::RefH2M<H2M::Texture2D_H2M> TextureMap;
		bool UseTexture = false;
	};
	AOInput m_AOInput;

	float m_TilingFactor = 1.0f;
	std::string m_Name = "Untitled";

	MaterialUUID m_UUID;

};
