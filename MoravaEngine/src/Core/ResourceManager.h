#pragma once

#include "H2M/Renderer/TextureH2M.h"

#include "Material/Material.h"
#include "Texture/MoravaTexture.h"

#include <map>
#include <tuple>


class ResourceManager
{
public:
	static void Init();
	static void LoadTexture(std::string name, std::string filePath);
	static void LoadTexture(std::string name, std::string filePath, GLenum filter, bool force);
	static void LoadMaterial(std::string name, TextureInfo textureInfo);
	static H2M::RefH2M<MoravaTexture> HotLoadTexture(std::string textureName);
	static H2M::RefH2M<Material> HotLoadMaterial(std::string materialName);

	// Getters
	static inline H2M::RefH2M<MoravaTexture> GetTexture(std::string textureName) { return s_Textures[textureName]; };
	static inline std::map<std::string, H2M::RefH2M<MoravaTexture>>* GetTextures() { return &s_Textures; };
	static inline std::map<std::string, H2M::RefH2M<Material>>* GetMaterials() { return &s_Materials; };
	static inline std::map<std::string, std::string>* GetTextureInfo() { return &s_TextureInfo; };
	static inline std::map<std::string, TextureInfo>* GetMaterialInfo() { return &s_MaterialInfo; };
	static inline std::map<std::string, H2M::RefH2M<MoravaShader>>* GetShaders() { return &s_ShaderCacheByTitle; };

	// Loading Texture2D_H2M through the texture cache. A file is loaded once per color space: sRGB (color data, e.g. albedo)
	// and linear (raw data, e.g. roughness) are different GPU formats, so one file can be cached as both.
	static H2M::RefH2M<H2M::Texture2D_H2M> LoadTexture2D_H2M(std::string filePath, bool sRGB);
	// Removes the cached textures nobody else uses any more (only the cache holds them), which frees their GPU memory.
	// The GPU must not be using them: call it when the device is idle. Returns the number of textures released.
	static uint32_t PurgeUnusedTextures2D();

	static void AddShader(std::string name, H2M::RefH2M<MoravaShader> shader);
	static H2M::RefH2M<MoravaShader> GetShader(std::string name);

	// Caching shaders
	static H2M::RefH2M<MoravaShader> CreateOrLoadShader(MoravaShaderSpecification moravaShaderSpecification);

public:
	static float s_MaterialSpecular;
	static float s_MaterialShininess;

private:
	// Asset loading
	static std::map<std::string, std::string> s_TextureInfo;
	static std::map<std::string, TextureInfo> s_MaterialInfo;

	static std::map<std::string, H2M::RefH2M<MoravaTexture>> s_Textures;
	static std::map<std::string, H2M::RefH2M<Material>> s_Materials;

	// Texture cache key: the file (normalized path, see NormalizeTexturePath) and the color space
	struct TextureCacheKey
	{
		std::string Path;
		bool SRGB;
		bool operator<(const TextureCacheKey& other) const { return std::tie(Path, SRGB) < std::tie(other.Path, other.SRGB); }
	};
	static std::string NormalizeTexturePath(const std::string& filePath);

	static std::map<TextureCacheKey, H2M::RefH2M<H2M::Texture2D_H2M>> s_HazelTextures2D;

	static std::map<std::string, H2M::RefH2M<MoravaShader>> s_ShaderCacheByTitle;

	static std::map<std::string, H2M::RefH2M<MoravaShader>> s_ShadersCacheByFilepath;

};
