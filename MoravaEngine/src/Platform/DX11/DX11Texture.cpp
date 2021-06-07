#include "DX11Texture.h"

#include "DX11Context.h"
#include "DX11Image.h"

#include "../stb_image.h"


DX11Texture2D::DX11Texture2D(const std::string& path, bool srgb, Hazel::HazelTextureWrap wrap)
	: m_Path(path)
{
	int width, height, channels;
	stbi_set_flip_vertically_on_load(1);
	m_ImageData.Data = stbi_load(path.c_str(), &width, &height, &channels, 4);
	m_ImageData.Size = width * height * 4;
	HZ_CORE_ASSERT(m_ImageData.Data, "Failed to load image!");
	m_Width = width;
	m_Height = height;
	m_Channels = channels;

	if (channels != 4 && channels != 3) {
		return;
	}

	HZ_CORE_ASSERT(channels == 4 || channels == 3);
	// HZ_CORE_ASSERT(channels == 4);

	//	Ref<DX11Texture2D> instance = this;
	//	HazelRenderer::Submit([instance]() mutable
	//	{
	//		instance->Invalidate();
	//	});

	Invalidate();
}

DX11Texture2D::DX11Texture2D(Hazel::HazelImageFormat format, uint32_t width, uint32_t height, const void* data, Hazel::HazelTextureWrap wrap)
	: m_Format(format)
{
	m_Width = width;
	m_Height = height;

	// HZ_CORE_ASSERT(format == ImageFormat::RGBA);
	uint32_t size = width * height * 4;

	m_ImageData = Hazel::Buffer::Copy(data, size);
	memcpy(m_ImageData.Data, data, m_ImageData.Size);

	//	Ref<DX11Texture2D> instance = this;
	//	HazelRenderer::Submit([instance]() mutable
	//	{
	//		instance->Invalidate();
	//	});

	Invalidate();
}

DX11Texture2D::DX11Texture2D(Hazel::HazelImageFormat format, uint32_t width, uint32_t height, Hazel::HazelTextureWrap wrap)
{
	HZ_CORE_ASSERT(false);
}

DX11Texture2D::~DX11Texture2D()
{
	// auto DX11Device = DX11Context::GetCurrentDevice()->GetDX11Device();

	// TODO
}

void DX11Texture2D::Invalidate()
{
	auto device = DX11Context::GetCurrentDevice();
	// auto DX11Device = device->GetDX11Device();

	// TODO
}

void DX11Texture2D::Bind(uint32_t slot) const
{
}

void DX11Texture2D::Lock()
{
}

void DX11Texture2D::Unlock()
{
}

void DX11Texture2D::Resize(uint32_t width, uint32_t height)
{
}

Hazel::Buffer DX11Texture2D::GetWriteableBuffer()
{
	return m_ImageData;
}

bool DX11Texture2D::Loaded() const
{
	return true;
}

const std::string& DX11Texture2D::GetPath() const
{
	return m_Path;
}

Hazel::HazelImageFormat DX11Texture2D::GetFormat() const
{
	return Hazel::HazelImageFormat::None;
}

uint32_t DX11Texture2D::GetMipLevelCount() const
{
	return 0;
}

Hazel::RendererID DX11Texture2D::GetRendererID() const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////
// TextureCube
//////////////////////////////////////////////////////////////////////////////////

DX11TextureCube::DX11TextureCube(Hazel::HazelImageFormat format, uint32_t width, uint32_t height, const void* data)
	: m_Format(format), m_Width(width), m_Height(height)
{
	if (data)
	{
		uint32_t size = width * height * 4 * 6; // six layers
		m_LocalStorage = Hazel::Buffer::Copy(data, size);
	}

	//	Hazel::Ref<DX11TextureCube> instance = this;
	//	HazelRenderer::Submit([instance]() mutable
	//	{
	//		instance->Invalidate();
	//	});

	Invalidate();
}

DX11TextureCube::DX11TextureCube(const std::string& path)
{
}

DX11TextureCube::~DX11TextureCube()
{
}

void DX11TextureCube::Invalidate()
{
	auto device = DX11Context::GetCurrentDevice();
	// auto DX11Device = device->GetDX11Device();

	uint32_t mipCount = GetMipLevelCount();

	// TODO
}

uint32_t DX11TextureCube::GetMipLevelCount() const
{
	// TODO
	return uint32_t();
}

void DX11TextureCube::GenerateMips(bool readonly)
{
	auto device = DX11Context::GetCurrentDevice();
	// auto DX11Device = device->GetDX11Device();

	// TODO
}
