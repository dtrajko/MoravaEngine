#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#pragma once

#include "DX11.h"

#include "DX11Material.h"

#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/VertexBufferH2M.h"
#include "H2M/Renderer/IndexBufferH2M.h"


struct DX11MaterialSlot
{
	size_t StartIndex = 0;
	size_t NumIndices = 0;
	size_t MaterialID = 0;
};


class DX11Mesh : public H2M::RefCountedH2M
{
public:
	DX11Mesh(const wchar_t* fullPath);
	~DX11Mesh();

	const H2M::RefH2M<H2M::VertexBufferH2M>& GetVertexBuffer() { return m_VertexBuffer; };
	const H2M::RefH2M<H2M::IndexBufferH2M>& GetIndexBuffer() { return m_IndexBuffer; };

	const DX11MaterialSlot GetMaterialSlot(uint32_t slot);
	size_t GetNumMaterialSlots();

private:
	void ComputeTangents(
		const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
		const glm::vec2& t0, const glm::vec2& t1, const glm::vec2& t2,
		glm::vec3& tangent, glm::vec3& binormal);

private:
	H2M::RefH2M<H2M::VertexBufferH2M> m_VertexBuffer;
	H2M::RefH2M<H2M::IndexBufferH2M> m_IndexBuffer;

	std::vector<DX11MaterialSlot> m_MaterialSlots;

};
