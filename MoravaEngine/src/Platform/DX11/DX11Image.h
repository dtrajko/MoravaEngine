/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"

#include "H2M/Renderer/ImageH2M.h"


struct DX11ImageInfo
{
	uint64_t Image;
};

class DX11Image2D : public H2M::Image2D_H2M
{
public:
	DX11Image2D(H2M::ImageSpecificationH2M specification);
	DX11Image2D(H2M::ImageFormatH2M format, uint32_t width, uint32_t height);
	virtual ~DX11Image2D();

	virtual void Invalidate() override;
	virtual void Release() override;

	virtual H2M::ImageFormatH2M GetFormat() const override { return m_Format; }
	virtual uint32_t GetWidth() const override { return m_Width; }
	virtual uint32_t GetHeight() const override { return m_Height; }

	virtual float GetAspectRatio() const override { return (float)m_Specification.Width / (float)m_Specification.Height; }

	void RT_Invalidate() {}

	DX11ImageInfo& GetImageInfo() { return m_Info; }
	const DX11ImageInfo& GetImageInfo() const { return m_Info; }

	virtual H2M::BufferH2M GetBuffer() const override { return m_ImageData; }
	virtual H2M::BufferH2M& GetBuffer() override { return m_ImageData; }

	virtual uint64_t GetHash() const override { return m_Info.Image; }

	void UpdateDescriptor();

private:
	H2M::ImageSpecificationH2M m_Specification;

	H2M::ImageFormatH2M m_Format;
	uint32_t m_Width = 0, m_Height = 0;

	H2M::BufferH2M m_ImageData;

	DX11ImageInfo m_Info = DX11ImageInfo{};

};
