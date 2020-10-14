#pragma once

#include "Hazel/Renderer/HazelTexture.h"

#include <glm/glm.hpp>


class EnvMapMaterial
{
	struct AlbedoInput;
	struct NormalInput;
	struct MetalnessInput;
	struct RoughnessInput;

public:
	EnvMapMaterial();
	~EnvMapMaterial();

	inline AlbedoInput& GetAlbedoInput() { return m_AlbedoInput; }
	inline NormalInput& GetNormalInput() { return m_NormalInput; }
	inline MetalnessInput& GetMetalnessInput() { return m_MetalnessInput; }
	inline RoughnessInput& GetRoughnessInput() { return m_RoughnessInput; }

private:

	/**** BEGIN properties EditorLayer ****/
	struct AlbedoInput
	{
		glm::vec3 Color = { 0.972f, 0.96f, 0.915f }; // Silver, from https://docs.unrealengine.com/en-us/Engine/Rendering/Materials/PhysicallyBased
		Hazel::HazelTexture2D* TextureMap;
		bool SRGB = true;
		bool UseTexture = false;
	};
	AlbedoInput m_AlbedoInput;

	struct NormalInput
	{
		Hazel::HazelTexture2D* TextureMap;
		bool UseTexture = false;
	};
	NormalInput m_NormalInput;

	struct MetalnessInput
	{
		float Value = 1.0f;
		Hazel::HazelTexture2D* TextureMap;
		bool UseTexture = false;
	};
	MetalnessInput m_MetalnessInput;

	struct RoughnessInput
	{
		float Value = 0.2f;
		Hazel::HazelTexture2D* TextureMap;
		bool UseTexture = false;
	};
	RoughnessInput m_RoughnessInput;

};
