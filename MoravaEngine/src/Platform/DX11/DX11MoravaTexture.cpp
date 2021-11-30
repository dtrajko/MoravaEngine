#include "DX11MoravaTexture.h"

#include "Core/Log.h"
#include "H2M/Core/Base.h"

#include <fstream>
#include <exception>
#include <string>


DX11MoravaTexture::DX11MoravaTexture()
{
	m_Spec.Width = 0;
	m_Spec.Height = 0;
	m_Spec.BitDepth = 0;
	m_Spec.InternalFormat = 0;
	m_Spec.Border = 0;
	m_Spec.Format = 0;
	// m_Spec.Type = GL_UNSIGNED_BYTE;
	m_Spec.FlipVertically = false;
	m_Spec.IsSampler = false;
	// m_Spec.Texture_Wrap_S = GL_REPEAT;
	// m_Spec.Texture_Wrap_T = GL_REPEAT;
	// m_Spec.Texture_Wrap_R = GL_REPEAT;
	// m_Spec.Texture_Min_Filter = GL_LINEAR;
	// m_Spec.Texture_Mag_Filter = GL_LINEAR;
	m_Spec.MipLevel = 0;

	m_Level = 0;
	m_ID = 0;
	m_FileLocation = "";
	m_Buffer = nullptr;
	m_Format = H2M::ImageFormatH2M::RGBA;
}

DX11MoravaTexture::DX11MoravaTexture(const char* fileLoc, bool flipVert, bool isSampler, int filter)
	: DX11MoravaTexture()
{
	m_FileLocation = fileLoc;

	m_Spec.Texture_Min_Filter = filter;
	m_Spec.Texture_Mag_Filter = filter;
	m_Spec.FlipVertically = flipVert;
	m_Spec.IsSampler = isSampler;

	try {
		Load(m_Spec.FlipVertically);
	}
	// catch (const std::exception& e) {
	catch (...) {
		Log::GetLogger()->error("Failed to load a texture '{0}'!", m_FileLocation);
		// throw std::runtime_error(e.what());
	}
}

DX11MoravaTexture::DX11MoravaTexture(const char* fileLoc, uint32_t width, uint32_t height, bool isSampler, int filter)
	: DX11MoravaTexture()
{
	m_FileLocation = fileLoc;
	m_Spec.Width = width;
	m_Spec.Height = height;
	m_Spec.IsSampler = isSampler;
	m_Spec.BitDepth = 4;
	m_Spec.Texture_Min_Filter = filter;
	m_Spec.Texture_Mag_Filter = filter;

	m_Buffer = new unsigned char[m_Spec.Width * m_Spec.Height * m_Spec.BitDepth];

	if (!m_Spec.IsSampler)
	{
		stbi_image_free(m_Buffer);
	}
}

/**
* Constructor for a fully customizable 2D texture
*/
DX11MoravaTexture::DX11MoravaTexture(const char* fileLoc, Specification spec)
	: DX11MoravaTexture()
{
	m_FileLocation = fileLoc;

	m_Spec.InternalFormat = spec.InternalFormat;
	m_Spec.Width = spec.Width;
	m_Spec.Height = spec.Height;
	m_Spec.Border = spec.Border;
	m_Spec.Format = spec.Format;
	m_Spec.Type = spec.Type;
	m_Spec.Texture_Wrap_S = spec.Texture_Wrap_S;
	m_Spec.Texture_Wrap_T = spec.Texture_Wrap_T;
	m_Spec.Texture_Wrap_R = spec.Texture_Wrap_R;
	m_Spec.Texture_Min_Filter = spec.Texture_Min_Filter;
	m_Spec.Texture_Mag_Filter = spec.Texture_Mag_Filter;
	m_Spec.MipLevel = spec.MipLevel;
	m_Spec.FlipVertically = spec.FlipVertically;
	m_Spec.BitDepth = spec.BitDepth;
	m_Spec.IsSampler = spec.IsSampler;
	m_Spec.Samples = spec.Samples;
	m_Spec.IsMultisample = spec.IsMultisample;
	m_Spec.IsSRGB = spec.IsSRGB;

	Load(m_Spec.FlipVertically);
}

bool DX11MoravaTexture::Load(bool flipVert)
{
	if (m_Buffer)
	{
		Log::GetLogger()->info("Texture '{0}' already loaded. Skipping...", m_FileLocation);
		return true;
	}

	stbi_set_flip_vertically_on_load(flipVert ? 1 : 0);

	if (stbi_is_hdr(m_FileLocation))
	{
		Log::GetLogger()->info("Loading an HDR texture '{0}'", m_FileLocation);
		m_Buffer = (byte*)stbi_loadf(m_FileLocation, (int*)&m_Spec.Width, (int*)&m_Spec.Height, &m_Spec.BitDepth, 0);
		m_Format = H2M::ImageFormatH2M::RGBA16F;
	}
	else
	{
		m_Buffer = stbi_load(m_FileLocation, (int*)&m_Spec.Width, (int*)&m_Spec.Height, &m_Spec.BitDepth, 0);
		m_Format = H2M::ImageFormatH2M::RGBA;
	}

	if (!m_Buffer)
	{
		std::string message = "ERROR: Texture failed to load '" + std::string(m_FileLocation) + "'";
		throw std::runtime_error(message.c_str());
	}

	CreateAPISpecific();

	float fileSize = GetFileSize(m_FileLocation) / (1024.0f * 1024.0f);
	Log::GetLogger()->info("Loading texture '{0}' [ID={1}, size={2} MB]", m_FileLocation, m_ID, fileSize);

	if (!m_Spec.IsSampler)
	{
		stbi_image_free(m_Buffer);
	}

	return true;
}

void DX11MoravaTexture::CreateAPISpecific()
{
	Log::GetLogger()->warn("DX11MoravaTexture::CreateAPISpecific method not yet implemented!");
}

void DX11MoravaTexture::Save()
{
	CreateAPISpecific();

	stbi_write_png(m_FileLocation, m_Spec.Width, m_Spec.Height, m_Spec.BitDepth, m_Buffer, m_Spec.Width * m_Spec.BitDepth);
}

int DX11MoravaTexture::GetRed(int x, int z)
{
	return (int)m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 0];
}

int DX11MoravaTexture::GetGreen(int x, int z)
{
	return (int)m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 1];
}

int DX11MoravaTexture::GetBlue(int x, int z)
{
	return (int)m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 2];

}

int DX11MoravaTexture::GetAlpha(int x, int z)
{
	return (int)m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 3];
}

void DX11MoravaTexture::SetPixel(int x, int z, glm::ivec4 pixel)
{
	SetRed(x, z, pixel.x);
	SetGreen(x, z, pixel.y);
	SetBlue(x, z, pixel.z);
	SetAlpha(x, z, pixel.w);
}

void DX11MoravaTexture::SetRed(int x, int z, int value)
{
	m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 0] = value;
}

void DX11MoravaTexture::SetGreen(int x, int z, int value)
{
	m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 1] = value;
}

void DX11MoravaTexture::SetBlue(int x, int z, int value)
{
	m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 2] = value;
}

void DX11MoravaTexture::SetAlpha(int x, int z, int value)
{
	m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 3] = value;
}

void DX11MoravaTexture::Bind(uint32_t textureSlot) const
{
	Log::GetLogger()->error("DX11MoravaTexture::Bind - method not yet implemented!");
}

void DX11MoravaTexture::Unbind()
{
	Log::GetLogger()->error("DX11MoravaTexture::Unbind - method not yet implemented!");
}

float DX11MoravaTexture::GetFileSize(const char* filename)
{
	struct stat stat_buf;
	int rc = stat(filename, &stat_buf);
	return rc == 0 ? (float)stat_buf.st_size : -1.0f;
}

void DX11MoravaTexture::Clear()
{
	if (m_Spec.IsSampler)
	{
		stbi_image_free(m_Buffer);
	}

	m_ID = 0;
	m_Spec.Width = 0;
	m_Spec.Height = 0;
	m_Spec.BitDepth = 0;
	m_FileLocation = "";
}

uint32_t DX11MoravaTexture::CalculateMipMapCount(uint32_t width, uint32_t height)
{
	uint32_t levels = 1;
	while ((width | height) >> levels)
	{
		levels++;
	}

	return levels;
}

uint32_t DX11MoravaTexture::GetMipLevelCount()
{
	return CalculateMipMapCount(m_Spec.Width, m_Spec.Height);
}

std::pair<uint32_t, uint32_t> DX11MoravaTexture::GetMipSize(uint32_t mip) const
{
	Log::GetLogger()->error("DX11MoravaTexture::GetMipSize({0}) - method not implemented!", mip);
	return std::pair<uint32_t, uint32_t>();
}

DX11MoravaTexture::~DX11MoravaTexture()
{
	Clear();
}
