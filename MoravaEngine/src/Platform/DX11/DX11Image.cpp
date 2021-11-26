#include "DX11Image.h"

#include "DX11Context.h"


DX11Image2D::DX11Image2D(Hazel::ImageSpecification specification)
	: m_Format(specification.Format), m_Width(specification.Width), m_Height(specification.Height)
{
}

DX11Image2D::DX11Image2D(Hazel::HazelImageFormat format, uint32_t width, uint32_t height)
	: m_Format(format), m_Width(width), m_Height(height)
{
}

DX11Image2D::~DX11Image2D()
{
}

void DX11Image2D::Invalidate()
{

}

void DX11Image2D::Release()
{
}

void DX11Image2D::UpdateDescriptor()
{
}
