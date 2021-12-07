#include "DX11TextureCube.h"

#include "DX11Context.h"
#include "DX11Image.h"

#include <DirectXTex.h>


//////////////////////////////////////////////////////////////////////////////////
// TextureCube
//////////////////////////////////////////////////////////////////////////////////

DX11TextureCube::DX11TextureCube(H2M::ImageFormatH2M format, uint32_t width, uint32_t height, const void* data)
	: m_Format(format), m_Width(width), m_Height(height)
{
	if (data)
	{
		uint32_t size = width * height * 4 * 6; // six layers
		m_LocalStorage = H2M::BufferH2M::Copy(data, size);
	}

	//	H2M::RefH2M<DX11TextureCube> instance = this;
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
	ID3D11Device* dx11Device = DX11Context::Get()->GetDX11Device();
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
	ID3D11Device* dx11Device = DX11Context::Get()->GetDX11Device();

	// TODO
}
