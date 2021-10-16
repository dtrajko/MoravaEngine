#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"

#include "Hazel/Renderer/HazelImage.h"


struct DX11ImageInfo
{
	uint64_t Image;
};

class DX11Image2D : public Hazel::HazelImage2D
{
public:
	DX11Image2D(Hazel::ImageSpecification specification);
	virtual ~DX11Image2D();

	virtual void Invalidate() override;
	virtual void Release() override;

	virtual uint32_t GetWidth() const override { return m_Width; }
	virtual uint32_t GetHeight() const override { return m_Height; }
	virtual float GetAspectRatio() const override { return (float)m_Specification.Width / (float)m_Specification.Height; }

	virtual Hazel::ImageSpecification& GetSpecification() override { return m_Specification; }
	virtual const Hazel::ImageSpecification& GetSpecification() const override { return m_Specification; }

	void RT_Invalidate() {}

	DX11ImageInfo& GetImageInfo() { return m_Info; }
	const DX11ImageInfo& GetImageInfo() const { return m_Info; }

	virtual Hazel::Buffer GetBuffer() const override { return m_ImageData; }
	virtual Hazel::Buffer& GetBuffer() override { return m_ImageData; }

	virtual void CreatePerLayerImageViews() override {};

	virtual uint64_t GetHash() const override { return m_Info.Image; }

	void UpdateDescriptor();

private:
	Hazel::ImageSpecification m_Specification;

	Hazel::HazelImageFormat m_Format;
	uint32_t m_Width = 0, m_Height = 0;

	Hazel::Buffer m_ImageData;

	DX11ImageInfo m_Info;

};
