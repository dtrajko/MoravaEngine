#include "DX11TextureCube.h"

#include "DX11Context.h"
#include "DX11Image.h"

#include <DirectXTex.h>


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
