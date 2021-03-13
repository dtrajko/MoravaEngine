#pragma once

#include <cstdint>


class GBufferSSAO
{
public:
	GBufferSSAO();
	~GBufferSSAO();

	void Init();
	void GenerateConditional();
	void Update();
	void Release();
	void BindForWriting();
	void BindForReading();
	void SetReadBuffer(uint32_t attachmentIndex);

	inline uint32_t GetWidth() { return m_Width; }
	inline uint32_t GetHeight() { return m_Height; }

private:
	void Generate();
	void ResetHandlers();

public:
	unsigned int m_GBuffer;

	unsigned int m_GBufferPosition;
	unsigned int m_GBufferNormal;
	unsigned int m_GBufferAlbedo;
	unsigned int m_GBufferTexCoord;
	unsigned int m_RBO_Depth;

private:
	uint32_t m_Width;
	uint32_t m_Height;

	uint32_t m_WidthPrev;
	uint32_t m_HeightPrev;

};
