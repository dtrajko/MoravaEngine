#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/TextureH2M.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>


// What a render texture holds: the camera's finished picture (tonemapped with the camera's own exposure: what a screen
// or a UI image shows), or its light before tonemapping (a mirror, exposed with the view that sees it)
enum class RenderTextureFormat : int32_t { Color = 0, HDR = 1 };

struct RenderTextureSettings
{
	uint32_t Width = 512;
	uint32_t Height = 512;
	RenderTextureFormat Format = RenderTextureFormat::Color;

	static constexpr uint32_t MinSize = 16;
	static constexpr uint32_t MaxSize = 4096;

	bool operator==(const RenderTextureSettings& other) const
	{
		return Width == other.Width && Height == other.Height && Format == other.Format;
	}
	bool operator!=(const RenderTextureSettings& other) const { return !(*this == other); }

	static const char* GetFormatName(RenderTextureFormat format);
};

/**
 * A render texture (docs/engine/Game_Engine_Runtime_Guide.html, 7.5): a picture a camera draws into instead of the screen
 * (CameraComponent::Target), shown on a mesh (a material's map: a monitor, a mirror) or by a UI image (a minimap). Its
 * file (.mrtex) holds its size and format; materials and UI images name the file as they name an image's, and get its
 * texture through the texture cache (ResourceManager::LoadTexture2D_H2M), an ordinary texture whose path is the file.
 *
 * The renderer copies the camera's picture into the texture each frame it draws (7.5.2). Until a camera has drawn it,
 * the texture shows a placeholder: dark diagonal stripes in a light frame.
 *
 * Each file is loaded once and shared. A changed size or format makes a new texture: what showed the old one is pointed
 * at the new one by the editor (it asks GetRevision).
 */
class EnvMapVulkanRenderTexture
{
public:
	static constexpr int Version = 1;
	static constexpr const char* FileExtension = ".mrtex";

	static bool IsRenderTextureFile(const std::string& filepath);

	// The render texture of the file (absolute), loaded the first time; nullptr when it can't be read (logged)
	static std::shared_ptr<EnvMapVulkanRenderTexture> Get(const std::string& filepath);
	// The render textures loaded, by file
	static std::vector<std::shared_ptr<EnvMapVulkanRenderTexture>> GetLoaded();
	// Forgets them (their textures are freed when nothing shows them any more): when the device is idle
	static void ClearCache();

	// The file as text, and back
	static std::string ToText(const RenderTextureSettings& settings);
	static bool FromText(const std::string& text, RenderTextureSettings& settings, std::string& error);
	// Writes a render texture file (a new one, or the settings of one in use: then Get's object takes them, SetSettings)
	static bool Save(const std::string& filepath, const RenderTextureSettings& settings, std::string& error);

	const std::string& GetFilePath() const { return m_FilePath; }
	const RenderTextureSettings& GetSettings() const { return m_Settings; }
	// Other settings: a new texture of the new size and format (the device must be idle: the old one may be in use)
	void SetSettings(const RenderTextureSettings& settings);
	// Changes with every new texture (SetSettings)
	uint64_t GetRevision() const { return m_Revision; }

	const H2M::RefH2M<H2M::Texture2D_H2M>& GetTexture() const { return m_Texture; }

private:
	void CreateTexture();

	std::string m_FilePath;
	RenderTextureSettings m_Settings;
	H2M::RefH2M<H2M::Texture2D_H2M> m_Texture;
	uint64_t m_Revision = 1;
};
