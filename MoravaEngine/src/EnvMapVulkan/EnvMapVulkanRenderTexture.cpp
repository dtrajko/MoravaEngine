#include "EnvMapVulkanRenderTexture.h"

#include "Core/Log.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>


static std::map<std::string, std::shared_ptr<EnvMapVulkanRenderTexture>> s_RenderTextures;

static std::string GetKey(const std::string& filepath)
{
	std::error_code error;
	return std::filesystem::weakly_canonical(std::filesystem::u8path(filepath), error).generic_u8string();
}

const char* RenderTextureSettings::GetFormatName(RenderTextureFormat format)
{
	return format == RenderTextureFormat::HDR ? "HDR" : "Color";
}

bool EnvMapVulkanRenderTexture::IsRenderTextureFile(const std::string& filepath)
{
	std::string extension = std::filesystem::u8path(filepath).extension().u8string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return extension == FileExtension;
}

std::shared_ptr<EnvMapVulkanRenderTexture> EnvMapVulkanRenderTexture::Get(const std::string& filepath)
{
	if (filepath.empty())
	{
		return nullptr;
	}
	const std::string key = GetKey(filepath);
	auto it = s_RenderTextures.find(key);
	if (it != s_RenderTextures.end())
	{
		return it->second;
	}

	std::ifstream file(std::filesystem::u8path(filepath));
	std::stringstream text;
	text << file.rdbuf();
	RenderTextureSettings settings;
	std::string error = file ? std::string() : "the file can't be read";
	if (!file || !FromText(text.str(), settings, error))
	{
		Log::GetLogger()->error("Render texture '{0}' could not be loaded: {1}", filepath, error);
		return nullptr; // tried again next time: the file may be written by then
	}
	auto renderTexture = std::make_shared<EnvMapVulkanRenderTexture>();
	renderTexture->m_FilePath = std::filesystem::u8path(filepath).lexically_normal().generic_u8string();
	renderTexture->m_Settings = settings;
	renderTexture->CreateTexture();
	s_RenderTextures[key] = renderTexture;
	Log::GetLogger()->info("Render texture '{0}': {1}x{2}, {3}", filepath, settings.Width, settings.Height,
		RenderTextureSettings::GetFormatName(settings.Format));
	return renderTexture;
}

std::vector<std::shared_ptr<EnvMapVulkanRenderTexture>> EnvMapVulkanRenderTexture::GetLoaded()
{
	std::vector<std::shared_ptr<EnvMapVulkanRenderTexture>> loaded;
	for (const auto& [key, renderTexture] : s_RenderTextures)
	{
		loaded.push_back(renderTexture);
	}
	return loaded;
}

void EnvMapVulkanRenderTexture::ClearCache()
{
	s_RenderTextures.clear();
}

std::string EnvMapVulkanRenderTexture::ToText(const RenderTextureSettings& settings)
{
	YAML::Emitter out;
	out << YAML::BeginMap;
	out << YAML::Key << "RenderTexture" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "Version" << YAML::Value << Version;
	out << YAML::Key << "Width" << YAML::Value << settings.Width;
	out << YAML::Key << "Height" << YAML::Value << settings.Height;
	out << YAML::Key << "Format" << YAML::Value << RenderTextureSettings::GetFormatName(settings.Format);
	out << YAML::EndMap;
	out << YAML::EndMap;
	return std::string(out.c_str()) + "\n";
}

bool EnvMapVulkanRenderTexture::FromText(const std::string& text, RenderTextureSettings& settings, std::string& error)
{
	try
	{
		const YAML::Node root = YAML::Load(text);
		const YAML::Node node = root["RenderTexture"];
		if (!node || !node.IsMap())
		{
			error = "not a render texture file (no RenderTexture section)";
			return false;
		}
		settings = RenderTextureSettings();
		if (node["Width"]) settings.Width = node["Width"].as<uint32_t>();
		if (node["Height"]) settings.Height = node["Height"].as<uint32_t>();
		if (node["Format"])
		{
			const std::string format = node["Format"].as<std::string>();
			if (format != "Color" && format != "HDR")
			{
				error = "unknown Format '" + format + "' (Color or HDR)";
				return false;
			}
			settings.Format = format == "HDR" ? RenderTextureFormat::HDR : RenderTextureFormat::Color;
		}
		settings.Width = std::clamp(settings.Width, RenderTextureSettings::MinSize, RenderTextureSettings::MaxSize);
		settings.Height = std::clamp(settings.Height, RenderTextureSettings::MinSize, RenderTextureSettings::MaxSize);
		return true;
	}
	catch (const YAML::Exception& exception)
	{
		error = exception.what();
		return false;
	}
}

bool EnvMapVulkanRenderTexture::Save(const std::string& filepath, const RenderTextureSettings& settings, std::string& error)
{
	std::ofstream file(std::filesystem::u8path(filepath));
	if (!file)
	{
		error = "'" + filepath + "' can't be written";
		return false;
	}
	file << ToText(settings);
	file.close();
	auto it = s_RenderTextures.find(GetKey(filepath));
	if (it != s_RenderTextures.end())
	{
		it->second->SetSettings(settings);
	}
	return true;
}

void EnvMapVulkanRenderTexture::SetSettings(const RenderTextureSettings& settings)
{
	if (settings == m_Settings)
	{
		return;
	}
	m_Settings = settings;
	CreateTexture();
	m_Revision++;
}

void EnvMapVulkanRenderTexture::CreateTexture()
{
	// The placeholder until a camera draws into it: dark diagonal stripes in a light frame, so a surface or a UI image
	// that shows a render texture nobody draws is plain to see (and its size and orientation too)
	const uint32_t width = m_Settings.Width, height = m_Settings.Height;
	const uint32_t stripe = std::max(std::max(width, height) / 16u, 2u);
	const uint32_t frame = std::max(std::min(width, height) / 64u, 1u);
	const float dark[3] = { 0.10f, 0.12f, 0.16f }, light[3] = { 0.20f, 0.23f, 0.30f }, edge[3] = { 0.55f, 0.60f, 0.70f };
	auto colorAt = [&](uint32_t x, uint32_t y) -> const float*
	{
		if (x < frame || y < frame || x >= width - frame || y >= height - frame)
		{
			return edge;
		}
		return ((x + y) / stripe) % 2 == 0 ? dark : light;
	};

	if (m_Settings.Format == RenderTextureFormat::HDR)
	{
		// Linear light, as the HDR picture will be
		std::vector<float> pixels((size_t)width * height * 4);
		for (uint32_t y = 0; y < height; y++)
		{
			for (uint32_t x = 0; x < width; x++)
			{
				const float* color = colorAt(x, y);
				float* pixel = &pixels[((size_t)y * width + x) * 4];
				for (int c = 0; c < 3; c++)
				{
					pixel[c] = color[c] * color[c]; // about sRGB to linear: the same look as the Color placeholder
				}
				pixel[3] = 1.0f;
			}
		}
		m_Texture = H2M::Texture2D_H2M::Create(H2M::ImageFormatH2M::RGBA32F, width, height, pixels.data());
	}
	else
	{
		std::vector<uint8_t> pixels((size_t)width * height * 4);
		for (uint32_t y = 0; y < height; y++)
		{
			for (uint32_t x = 0; x < width; x++)
			{
				const float* color = colorAt(x, y);
				uint8_t* pixel = &pixels[((size_t)y * width + x) * 4];
				for (int c = 0; c < 3; c++)
				{
					pixel[c] = (uint8_t)(color[c] * 255.0f + 0.5f);
				}
				pixel[3] = 255;
			}
		}
		m_Texture = H2M::Texture2D_H2M::Create(H2M::ImageFormatH2M::SRGB, width, height, pixels.data());
	}
	if (m_Texture)
	{
		m_Texture->SetPath(m_FilePath);
	}
}
