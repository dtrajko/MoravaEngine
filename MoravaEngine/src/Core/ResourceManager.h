#pragma once

#include "H2M/Renderer/TextureH2M.h"

#include "Material/Material.h"
#include "Texture/MoravaTexture.h"

#include <map>


class ResourceManager
{
public:
	static void Init();
	static void LoadTexture(std::string name, std::string filePath);
	static void LoadTexture(std::string name, std::string filePath, GLenum filter, bool force);
	static void LoadMaterial(std::string name, TextureInfo textureInfo);
	static H2M::Ref<MoravaTexture> HotLoadTexture(std::string textureName);
	static H2M::Ref<Material> HotLoadMaterial(std::string materialName);

	// Getters
	static inline H2M::Ref<MoravaTexture> GetTexture(std::string textureName) { return s_Textures[textureName]; };
	static inline std::map<std::string, H2M::Ref<MoravaTexture>>* GetTextures() { return &s_Textures; };
	static inline std::map<std::string, H2M::Ref<Material>>* GetMaterials() { return &s_Materials; };
	static inline std::map<std::string, std::string>* GetTextureInfo() { return &s_TextureInfo; };
	static inline std::map<std::string, TextureInfo>* GetMaterialInfo() { return &s_MaterialInfo; };
	static inline std::map<std::string, H2M::Ref<MoravaShader>>* GetShaders() { return &s_ShaderCacheByTitle; };

	// Loading Texture2DH2M
	static H2M::Ref<H2M::Texture2DH2M> LoadTexture2DH2M(std::string filePath);

	static void AddShader(std::string name, H2M::Ref<MoravaShader> shader);
	static const H2M::Ref<MoravaShader>& GetShader(std::string name);

	// Caching shaders
	static const H2M::Ref<MoravaShader>& CreateOrLoadShader(MoravaShaderSpecification moravaShaderSpecification);

public:
	static float s_MaterialSpecular;
	static float s_MaterialShininess;

private:
	// Asset loading
	static std::map<std::string, std::string> s_TextureInfo;
	static std::map<std::string, TextureInfo> s_MaterialInfo;

	static std::map<std::string, H2M::Ref<MoravaTexture>> s_Textures;
	static std::map<std::string, H2M::Ref<Material>> s_Materials;

	static std::map<std::string, H2M::Ref<H2M::Texture2DH2M>> s_HazelTextures2D;

	static std::map<std::string, H2M::Ref<MoravaShader>> s_ShaderCacheByTitle;

	static std::map<std::string, H2M::Ref<MoravaShader>> s_ShadersCacheByFilepath;

};
