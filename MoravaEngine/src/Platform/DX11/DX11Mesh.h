#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#pragma once

#include "DX11.h"

#include "Hazel/Core/Ref.h"
#include "Hazel/Renderer/VertexBuffer.h"
#include "Hazel/Renderer/IndexBuffer.h"


class DX11Mesh : public Hazel::RefCounted
{
public:
	DX11Mesh(const wchar_t* fullPath);
	~DX11Mesh();

	const Hazel::Ref<Hazel::VertexBuffer>& GetVertexBuffer() { return m_VertexBuffer; };
	const Hazel::Ref<Hazel::IndexBuffer>& GetIndexBuffer() { return m_IndexBuffer; };

private:
	Hazel::Ref<Hazel::VertexBuffer> m_VertexBuffer;
	Hazel::Ref<Hazel::IndexBuffer> m_IndexBuffer;

};
