#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"

#include "H2M/Renderer/ImageH2M.h"


struct DX11ImageInfo
{
	uint64_t Image;
};

class DX11Image2D : public H2M::HazelImage2D
{
public:
	DX11Image2D(H2M::ImageSpecification specification);
	DX11Image2D(H2M::ImageFormatH2M format, uint32_t width, uint32_t height);
	virtual ~DX11Image2D();

	virtual void Invalidate() override;
	virtual void Release() override;

	virtual uint32_t GetWidth() const override { return m_Width; }
	virtual uint32_t GetHeight() const override { return m_Height; }

	virtual float GetAspectRatio() const override { return (float)m_Specification.Width / (float)m_Specification.Height; }

	virtual H2M::ImageSpecification& GetSpecification() override { return m_Specification; }
	virtual const H2M::ImageSpecification& GetSpecification() const override { return m_Specification; }

	void RT_Invalidate() {}

	DX11ImageInfo& GetImageInfo() { return m_Info; }
	const DX11ImageInfo& GetImageInfo() const { return m_Info; }

	virtual H2M::Buffer GetBuffer() const override { return m_ImageData; }
	virtual H2M::Buffer& GetBuffer() override { return m_ImageData; }

	virtual void CreatePerLayerImageViews() override {};
	virtual uint64_t GetHash() const override { return m_Info.Image; }

	void UpdateDescriptor();

private:
	H2M::ImageSpecification m_Specification;

	H2M::ImageFormatH2M m_Format;
	uint32_t m_Width = 0, m_Height = 0;

	H2M::Buffer m_ImageData;

	DX11ImageInfo m_Info = DX11ImageInfo{};

};
